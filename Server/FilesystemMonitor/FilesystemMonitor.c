/*++

Module Name:

    FilesystemMonitor.c

Abstract:

    This is the main module of the FilesystemMonitor miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <stdlib.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <ntifs.h>
#include "FilesystemMonitor.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
#pragma warning(disable: 4996)
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4047 )
#pragma warning ( disable : 4189 )

LARGE_INTEGER Timeout;
PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;
PFLT_PORT gServerPort;
PFLT_PORT gClientPort;
FilesystemMonitor_COMMAND gCommand = ENUM_BLOCK;
Rule rules[5];

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define MINISPY_PORT_NAME                                L"\\MiniPort"

ULONG gTraceFlags = 0;


char* forbiddenStr[3] = { "MONITORCLIENT","SYSTEM32","VMWARE" };


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

NTSTATUS
SendProcessInfo(OperationInfo* info);

NTSTATUS
FilesystemMonitorMiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID* ConnectionCookie
);

int CheckPermission(char* user, char* path);

NTSTATUS
NPMiniMessage(
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
);

VOID
FilesystemMonitorMiniDisconnect(
    __in_opt PVOID ConnectionCookie
);

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FilesystemMonitorInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
FilesystemMonitorInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
FilesystemMonitorInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
FilesystemMonitorUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
FilesystemMonitorInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FilesystemMonitorPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
FilesystemMonitorOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
FilesystemMonitorPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FilesystemMonitorPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

BOOLEAN
FilesystemMonitorDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );



FLT_PREOP_CALLBACK_STATUS
FilePreCreate(
    IN OUT PFLT_CALLBACK_DATA Data,
    IN     PCFLT_RELATED_OBJECTS FltObjects,
    OUT    PVOID* CompletionContext);

FLT_PREOP_CALLBACK_STATUS
FilePreWrite(
    IN OUT PFLT_CALLBACK_DATA Data,
    IN     PCFLT_RELATED_OBJECTS FltObjects,
    OUT    PVOID* CompletionContext);




VOID
FilesystemMonitorProcessNotify(
    IN HANDLE  ParentId,
    IN HANDLE  ProcessId,
    IN BOOLEAN  Create
);

VOID
FilesystemMonitorLoadImage(
    __in_opt PUNICODE_STRING  FullImageName,
    __in HANDLE  ProcessId,
    __in PIMAGE_INFO  ImageInfo
);


void MyCurentTimeStr(char* des)
{
    LARGE_INTEGER snow, now;
    TIME_FIELDS now_fields;
    static WCHAR time_str[32] = { 0 };

    //---获取标准时间
    KeQuerySystemTime(&snow);

    //---转换为当地时间
    ExSystemTimeToLocalTime(&snow, &now);

    //---整理出年，月，日，时，分，秒 这些元素  
    RtlTimeToTimeFields(&now, &now_fields);

    //---输出到字符串中
    RtlStringCchPrintfW(
        time_str,
        32 * 2,
        L"%4d-%2d-%2d %2d:%2d:%2d",
        now_fields.Year,
        now_fields.Month,
        now_fields.Day,
        now_fields.Hour,
        now_fields.Minute,
        now_fields.Second
    );

    //----

    sprintf(des, "%ws", time_str);
}


static NTSTATUS get_procees_user(int pid, PWCHAR* out_name);




EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FilesystemMonitorUnload)
#pragma alloc_text(PAGE, FilesystemMonitorInstanceQueryTeardown)
#pragma alloc_text(PAGE, FilesystemMonitorInstanceSetup)
#pragma alloc_text(PAGE, FilesystemMonitorInstanceTeardownStart)
#pragma alloc_text(PAGE, FilesystemMonitorInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      FilePreCreate,
      FilesystemMonitorPostOperation },

     { IRP_MJ_WRITE,
      0,
      FilePreWrite,
      FilesystemMonitorPostOperation },

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_CLOSE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_READ,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_WRITE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SET_EA,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FilesystemMonitorPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_PNP,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FilesystemMonitorPreOperation,
      FilesystemMonitorPostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    FilesystemMonitorUnload,                           //  MiniFilterUnload

    FilesystemMonitorInstanceSetup,                    //  InstanceSetup
    FilesystemMonitorInstanceQueryTeardown,            //  InstanceQueryTeardown
    FilesystemMonitorInstanceTeardownStart,            //  InstanceTeardownStart
    FilesystemMonitorInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
FilesystemMonitorInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
FilesystemMonitorInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
FilesystemMonitorInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorInstanceTeardownStart: Entered\n") );
}


VOID
FilesystemMonitorInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;
    PSECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING uniString;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

            FltUnregisterFilter( gFilterHandle );
        }
    }
    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status)) {
        goto final;
    }
    RtlInitUnicodeString(&uniString, MINISPY_PORT_NAME);
    InitializeObjectAttributes(&oa, &uniString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);

    status = FltCreateCommunicationPort(gFilterHandle,
        &gServerPort,
        &oa, NULL,
        FilesystemMonitorMiniConnect,
        FilesystemMonitorMiniDisconnect,
        NPMiniMessage,
        1);

    final :
    if (!NT_SUCCESS(status)) {

        if (NULL != gServerPort) {
            FltCloseCommunicationPort(gServerPort);
        }

        if (NULL != gFilterHandle) {
            FltUnregisterFilter(gFilterHandle);
        }
    }
    return status;
}

NTSTATUS
FilesystemMonitorUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorUnload: Entered\n") );
    

    PsSetCreateProcessNotifyRoutine(FilesystemMonitorProcessNotify, TRUE);
    PsRemoveLoadImageNotifyRoutine(FilesystemMonitorLoadImage);



    FltCloseCommunicationPort(gServerPort);

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
FilesystemMonitorPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    //DbgPrint("************[CreatePreOperation]hi!!!!!FilesystemMonitorPreOperation!!");
    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
        ("minifilter!minifilterPreOperation: Entered\n"));





    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (FilesystemMonitorDoRequestOperationStatus(Data)) {

        status = FltRequestOperationStatusCallback(Data,
            FilesystemMonitorOperationStatusCallback,
            (PVOID)(++OperationStatusCtx));
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS,
                ("FilesystemMonitor!minifilterPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                    status));
        }
    }

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.


    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}





FLT_PREOP_CALLBACK_STATUS FilePreCreate(
    IN OUT PFLT_CALLBACK_DATA Data,
    IN     PCFLT_RELATED_OBJECTS FltObjects,
    OUT    PVOID* CompletionContext)
{
    NTSTATUS status;
    OperationInfo message;
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PFLT_FILE_NAME_INFORMATION nameInfo;


    MyCurentTimeStr(message.time);
    //DbgPrint("CreateTime: %s\r\n", message.time);

    PAGED_CODE();
    __try {
        message.operation_type = 1;
        if (Data && Data->Thread)
        {
            ULONG ProcessId = FltGetRequestorProcessId(Data);
            ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);
            //DbgPrint("CreateRequest, pid: %d, tid: %d\n", ProcessId, ThreadId);

            message.process_id = ProcessId;
            PWCHAR out_name;
            get_procees_user(ProcessId, &out_name);
            sprintf(message.user, "%ws", out_name);
            //DbgPrint("CreateRequest, user: %x | %s\n", message.user, message.user);
        }       

               

        status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
        if (NT_SUCCESS(status)) {
            //DbgPrint("CreateFile");
            FltParseFileNameInformation(nameInfo);

            //输出文件名及相对路径
            ANSI_STRING tmp_process;
            RtlUnicodeStringToAnsiString(&tmp_process, &(nameInfo->Name), TRUE);
            strcpy(message.path, tmp_process.Buffer);
            DbgPrint("CreateDirectory: %s\r\n", message.path);

       
        }
        int permission = CheckPermission(message.user, message.path);
        if (!permission)
        {
            DbgPrint("Permission Denied");
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            Data->IoStatus.Information = 0;
            FltReleaseFileNameInformation(nameInfo);
            return FLT_PREOP_COMPLETE;
        }
        if (gCommand == ENUM_START) {
            SendProcessInfo(&message);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DbgPrint("FilePreCreate EXCEPTION_EXECUTE_HANDLER\n");
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS FilePreWrite(
    IN OUT PFLT_CALLBACK_DATA Data,
    IN     PCFLT_RELATED_OBJECTS FltObjects,
    OUT    PVOID* CompletionContext)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);
    OperationInfo message;
    message.operation_type = 2;
    
    if (Data && Data->Thread)
    {
        ULONG ProcessId = FltGetRequestorProcessId(Data);
        ULONG ThreadId = (ULONG)PsGetThreadId(Data->Thread);

        //DbgPrint("WriteRequest, pid: %d, tid: %d\n", ProcessId, ThreadId);

        message.process_id = ProcessId;
        PWCHAR out_name;
        get_procees_user(ProcessId, &out_name);
        sprintf(message.user, "%ws", out_name);
        //DbgPrint("WriteRequest, user: %s\n", message.user);
    }

    MyCurentTimeStr(message.time);
    //DbgPrint("WriteTime: %s\r\n", message.time);



    PAGED_CODE();
    {
        PFLT_FILE_NAME_INFORMATION nameInfo;
        //直接获得文件名并检查
        if (NT_SUCCESS(FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo)))
        {
            if (NT_SUCCESS(FltParseFileNameInformation(nameInfo)))
            {
                WCHAR pTempBuf[512] = { 0 };
                WCHAR* pNonPageBuf = NULL, * pTemp = pTempBuf;
                if (nameInfo->Name.MaximumLength > 512)
                {
                    pNonPageBuf = ExAllocatePool(NonPagedPool, nameInfo->Name.MaximumLength);
                    pTemp = pNonPageBuf;
                }
                RtlCopyMemory(pTemp, nameInfo->Name.Buffer, nameInfo->Name.MaximumLength);
                //DbgPrint("WriteFile");
                FltParseFileNameInformation(nameInfo);

                //输出文件名及相对路径
                ANSI_STRING tmp_process;
                RtlUnicodeStringToAnsiString(&tmp_process, &(nameInfo->Name), TRUE);
                strcpy(message.path, tmp_process.Buffer);
                DbgPrint("WriteDirectory: %s\r\n", message.path);
           

            }
            FltReleaseFileNameInformation(nameInfo);
        }
    
        if (gCommand == ENUM_START) {
             SendProcessInfo(&message);
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}



VOID
FilesystemMonitorOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("FilesystemMonitor!FilesystemMonitorOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}


FLT_POSTOP_CALLBACK_STATUS
FilesystemMonitorPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorPostOperation: Entered\n") );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FilesystemMonitorPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FilesystemMonitor!FilesystemMonitorPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
FilesystemMonitorDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}

NTSTATUS
FilesystemMonitorMiniConnect(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID* ConnectionCookie
)
{
    DbgPrint("[mini-filter] NPMiniConnect");
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionCookie);

    ASSERT(gClientPort == NULL);
    gClientPort = ClientPort;
    return STATUS_SUCCESS;
}

VOID
FilesystemMonitorMiniDisconnect(
    __in_opt PVOID ConnectionCookie
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(ConnectionCookie);
    DbgPrint("[mini-filter] NPMiniDisconnect");

    //  Close our handle
    FltCloseClientPort(gFilterHandle, &gClientPort);
}

NTSTATUS
NPMiniMessage(
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
)
{

    NTSTATUS status = STATUS_SUCCESS;
    FilesystemMonitor_COMMAND command;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(ConnectionCookie);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

    DbgPrint("[mini-filter] NPMiniMessage\n");
    if ((InputBuffer != NULL) &&
        (InputBufferSize >= (FIELD_OFFSET(COMMAND_MESSAGE, Command) +
            sizeof(COMMAND_MESSAGE)))) {
        try {
            //  Probe and capture input message: the message is raw user mode
            //  buffer, so need to protect with exception handler
            command = ((PCOMMAND_MESSAGE)InputBuffer)->Command;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();
        }
        switch (command) {
        case ENUM_START:
        {
            DbgPrint("[mini-filter] ENUM_PASS");
            gCommand = ENUM_START;
            status = STATUS_SUCCESS;
            break;
        }
        case ENUM_BLOCK:
        {
            DbgPrint("[mini-filter] ENUM_BLOCK");
            gCommand = ENUM_BLOCK;
            status = STATUS_SUCCESS;
            break;
        }
        case ENUM_RULE:
        {
            DbgPrint("[mini-filter] ENUM_RULE");
            Rule* recv_rules = ((PCOMMAND_MESSAGE)InputBuffer)->rules;
            for (int i = 0; i < 5; i++)
            {
                strcpy(rules[i].path, recv_rules[i].path);
                strcpy(rules[i].user, recv_rules[i].user);
                //DbgPrint("user:%s",rules[i].user);
                //DbgPrint("path:%s", rules[i].path);
            }
            break;
        }

        default:
            DbgPrint("[mini-filter] default");
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }
    else {

        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
SendProcessInfo(OperationInfo* info)
{
    NTSTATUS status;
    Timeout.QuadPart = (LONGLONG)(-1) * 500;
    status = FltSendMessage(gFilterHandle, &gClientPort, info, sizeof(OperationInfo), NULL, NULL,&Timeout);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Send Fail,Status:%d",status);
    }
    else {
        //DbgPrint("Send successful");
    }
    return status;
}




VOID
FilesystemMonitorProcessNotify(
    IN HANDLE  ParentId,
    IN HANDLE  ProcessId,
    IN BOOLEAN  Create
)
{
    DbgPrint("FilesystemMonitorProcessNotify, pid: %d, tid: %d, create: %d\n", ParentId, ProcessId, Create);
}

VOID
FilesystemMonitorLoadImage(__in_opt PUNICODE_STRING FullImageName, __in HANDLE ProcessId, __in PIMAGE_INFO ImageInfo)
{
    UNREFERENCED_PARAMETER(ImageInfo);

    if (FullImageName)
    {
        DbgPrint("FilesystemMonitorLoadImage, image name: %wZ, pid: %d\n", FullImageName, ProcessId);
    }
    else
        DbgPrint("FilesystemMonitorLoadImage, image name: null, pid: %d\n", ProcessId);
}



static NTSTATUS get_procees_user(int pid, PWCHAR* out_name)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    *out_name = NULL;
    PEPROCESS pProcess = NULL;
    PACCESS_TOKEN Token;
    LUID luid;
    PSecurityUserData userInformation = NULL;
    PWCHAR name = NULL;
    ///
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        //DPT("IRQL Must PASSIVE_LEVEL.\n");
        return status;
    }

    status = PsLookupProcessByProcessId((HANDLE)pid, &pProcess);
    if (!NT_SUCCESS(status)) {
        //DPT("Not Find PID=%d\n", pid);
        return status;
    }

    Token = PsReferencePrimaryToken(pProcess);

    status = SeQueryAuthenticationIdToken(Token, &luid);
    if (!NT_SUCCESS(status)) {
        //DPT("SeQueryAuthenticationIdToken Error<%u>\n", status);
        goto E;
    }

    status = GetSecurityUserInfo(&luid, UNDERSTANDS_LONG_NAMES, &userInformation);
    if (!NT_SUCCESS(status)) {
        //DPT("GetSecurityUserInfo Error<%u>\n", status);
        goto E;
    }

    name = (PWCHAR)ExAllocatePool(NonPagedPool, userInformation->UserName.Length + sizeof(WCHAR));
    if (!name) {
        status = STATUS_NO_MEMORY;
        goto E;
    }
    RtlZeroMemory(name, userInformation->UserName.Length + sizeof(WCHAR));
    RtlCopyMemory(name, userInformation->UserName.Buffer, userInformation->UserName.Length);
    *out_name = name;

E:
    ObDereferenceObject(pProcess);
    ObDereferenceObject(Token);
    if (userInformation)
        LsaFreeReturnBuffer(userInformation);
    return status;
}

int CheckPermission(char* user, char* path){
    for (int i = 0; i < 5; i++){
        if (strcmp(user, rules[i].user)==0 && strcmp(path, rules[i].path)==0){
            return 0;
        }
    }
    return 1;
}





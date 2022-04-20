#pragma once

//许岽结构体
typedef struct
{
    char time[32];
    short operation_type;
    int process_id;
    char user[256];
    char path[256];
} OperationInfo;

typedef struct Rule
{
    char user[256];
    char path[256];
}Rule;

typedef enum _FilesystemMonitor_MINI_COMMAND {
    ENUM_START = 1,
    ENUM_BLOCK = 0,
    ENUM_RULE = 2
}FilesystemMonitor_COMMAND;

typedef struct _COMMAND_MESSAGE {
    FilesystemMonitor_COMMAND Command;
    Rule rules[5];
} COMMAND_MESSAGE, * PCOMMAND_MESSAGE;
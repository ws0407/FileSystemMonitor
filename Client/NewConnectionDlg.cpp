// NewConnectionDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "NewConnectionDlg.h"
#include "afxdialogex.h"
#include "RemoteMonitorDlg.h"
#include <thread>
#include <Windows.h>


extern CRemoteMonitorDlg* pLogin; //全局变量



// NewConnectionDlg 对话框

IMPLEMENT_DYNAMIC(NewConnectionDlg, CDialogEx)

NewConnectionDlg::NewConnectionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONNECT, pParent)
{

}

NewConnectionDlg::~NewConnectionDlg()
{
}

void NewConnectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(NewConnectionDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &NewConnectionDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &NewConnectionDlg::OnBnClickedCancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// NewConnectionDlg 消息处理程序


BOOL NewConnectionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	GetDlgItem(IDC_STATIC_CONNECT_TEXT)->SetWindowText("\n\n初始化成功，等待客户端连接...\n倒计时：9秒");
	font.CreatePointFont(100, "黑体");
	GetDlgItem(IDC_STATIC_CONNECT_TEXT)->SetFont(&font);

	SetTimer(1, 1000, 0);	//接受连接
	SetTimer(2, 1000, 0);	//倒计时


	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void NewConnectionDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	GetDlgItem(IDC_STATIC_CONNECT_TEXT)->SetWindowText("\n\n初始化成功，等待客户端连接...\n倒计时：9秒");
	SetTimer(1, 1000, 0);	//重新连接
	SetTimer(2, 1000, 0);	//倒计时

	//CDialogEx::OnOK();
}

void NewConnectionDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	//closesocket(pLogin->connSocket);
	CDialogEx::OnCancel();
}

void thread01() {
	pLogin->connSocket = 0;

	pLogin->clientSocket_vec.push_back(pLogin->connSocket);
	int index = pLogin->clientSocket_vec.size() - 1;

	pLogin->len = sizeof(SOCKADDR);
	pLogin->clientSocket_vec[index] = accept(pLogin->serverSocket, (SOCKADDR*)&(pLogin->clientAddr), &(pLogin->len));
	if (pLogin->host_index != -1) {
		while (1) {
			CString ip = pLogin->ip_addr_vec[pLogin->host_index - 1];
			if (ip == inet_ntoa(pLogin->clientAddr.sin_addr))
				break;
			pLogin->clientSocket_vec[index] = 0;
			closesocket(pLogin->clientSocket_vec[index]);
			pLogin->clientSocket_vec[index] = accept(pLogin->serverSocket, (SOCKADDR*)&(pLogin->clientAddr), &(pLogin->len));
		}
	}

	if (pLogin->clientSocket_vec[index] == INVALID_SOCKET)
	{
		//连接失败
		MessageBoxA(NULL,"连接失败，请重试", "提示", MB_ICONWARNING | MB_OK);
		//SetTimer(1, 1000, 0);
	}
	else {
		CString temp_str;

		HWND hTest = ::FindWindow(NULL, _T("新建连接"));
		KillTimer(hTest,2);

		temp_str.Format("连接成功，主机信息如下：\nip：%s\nport：%d", inet_ntoa(pLogin->clientAddr.sin_addr), (int)(pLogin->clientAddr.sin_port));
		if (MessageBox(NULL, temp_str, "提示", MB_ICONWARNING | MB_OKCANCEL) == IDOK) {
			/*
			if (pLogin->host_index != -1) {
				pLogin->connect_status_vec[index] = true;
				temp_str.Format("%d", pLogin->clientAddr.sin_port);
				pLogin->port_vec[index] = temp_str;

				HWND hTest = ::FindWindow(NULL, _T("新建连接"));	//关闭当前窗口
				::SendMessage(hTest, WM_CLOSE, NULL, NULL);

				pLogin->RefreshHostList();
				return;
			}*/

			int i = 0;
			for (vector<CString>::iterator it = pLogin->ip_addr_vec.begin(); it != pLogin->ip_addr_vec.end(); it++) {
				if (*it == inet_ntoa(pLogin->clientAddr.sin_addr)) {
					pLogin->ip_addr_vec.erase(pLogin->ip_addr_vec.begin() + i);
					pLogin->port_vec.erase(pLogin->port_vec.begin() + i);
					pLogin->connect_status_vec.erase(pLogin->connect_status_vec.begin() + i);
					pLogin->sendLen_vec.erase(pLogin->sendLen_vec.begin() + i);
					pLogin->clientSocket_vec.erase(pLogin->clientSocket_vec.begin() + i);
					break;
				}
				i++;
			}

			//创建日志文件夹
			CString Path;
			Path.Format("log\\%s",inet_ntoa(pLogin->clientAddr.sin_addr));
			BOOL rec = PathFileExists(Path);
			if (!rec) {
				system("mkdir "+Path);
			}

			CStdioFile file;
			if (!PathFileExists(Path + "\\KeyboardRecord.txt")) {
				file.Open(Path + "\\KeyboardRecord.txt", CFile::modeWrite | CFile::modeCreate); //如果不存在就创建
				file.Close();//关闭文件流
			}
			if (!PathFileExists(Path + "\\FileOperationRecord.txt")) {
				file.Open(Path + "\\FileOperationRecord.txt", CFile::modeWrite | CFile::modeCreate); //如果不存在就创建
				file.Close();
			}
			pLogin->ip_addr_vec.push_back(inet_ntoa(pLogin->clientAddr.sin_addr));
			temp_str.Format("%d", pLogin->clientAddr.sin_port);
			pLogin->port_vec.push_back(temp_str);
			pLogin->connect_status_vec.push_back(true);
			pLogin->sendLen_vec.push_back(0);

			HWND hTest = ::FindWindow(NULL, _T("新建连接"));	//关闭当前窗口
			::SendMessage(hTest, WM_CLOSE, NULL, NULL);

			pLogin->RefreshHostList();
		}
		else {
			closesocket(pLogin->clientSocket_vec[index]);
			pLogin->clientSocket_vec.pop_back();
			//SetTimer(1, 1000, 0);
		}
		
	}
	//closesocket(pLogin->serverSocket);
}

void thread02() {
	Sleep(9000);
	HWND hTest = ::FindWindow(NULL, _T("新建连接"));
	int index = pLogin->clientSocket_vec.size() - 1;

	if (pLogin->clientSocket_vec[index] == 0 && hTest != NULL && pLogin->ip_addr_vec.size() != pLogin->clientSocket_vec.size()) {
		MessageBoxA(hTest, "连接超时，请重试", "提示", MB_ICONWARNING | MB_OK);
		closesocket(pLogin->clientSocket_vec[index]);
		pLogin->clientSocket_vec.pop_back();
	}
}

void NewConnectionDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent) {
	case 1:
	{
		KillTimer(1);

		thread task01(thread01);
		task01.detach();

		thread task02(thread02);
		task02.detach();

		break;
	}
	case 2:
	{
		CString text;
		GetDlgItem(IDC_STATIC_CONNECT_TEXT)->GetWindowText(text);
		int time = _ttoi(text.Mid(40, 1));
		if (time == 0) {
			KillTimer(2);
			break;
		}
		text.Format("\n\n初始化成功，等待客户端连接...\n倒计时：%d秒", time - 1);

		GetDlgItem(IDC_STATIC_CONNECT_TEXT)->SetWindowText(text);
		break;
	}
	}
	

	CDialogEx::OnTimer(nIDEvent);
}

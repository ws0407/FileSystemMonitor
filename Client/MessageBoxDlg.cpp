// MessageBoxDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "MessageBoxDlg.h"
#include "afxdialogex.h"
#include "RemoteMonitorDlg.h"


// MessageBoxDlg 对话框
extern CRemoteMonitorDlg* pLogin;

IMPLEMENT_DYNAMIC(MessageBoxDlg, CDialogEx)

MessageBoxDlg::MessageBoxDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{

}

MessageBoxDlg::~MessageBoxDlg()
{
}

void MessageBoxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_edit_message);
}


BEGIN_MESSAGE_MAP(MessageBoxDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &MessageBoxDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &MessageBoxDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// MessageBoxDlg 消息处理程序


BOOL MessageBoxDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void MessageBoxDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}

string _mb2u8_(const char* cont)
{
	if (NULL == cont)
	{
		return "";
	}
	int num = MultiByteToWideChar(CP_ACP, NULL, cont, -1, NULL, NULL);
	if (num <= 0)
	{
		return "";
	}
	wchar_t* buffw = new (std::nothrow) wchar_t[num];
	if (NULL == buffw)
	{
		return "";
	}
	MultiByteToWideChar(CP_ACP, NULL, cont, -1, buffw, num);
	int len = WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
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
	WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	std::string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

void MessageBoxDlg::OnBnClickedOk()
{
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "messagebox", sizeof("messagebox"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send(messagebox) fail");
		return;
	}
	Sleep(500);
	CString message;
	GetDlgItem(IDC_EDIT_MESSAGE)->GetWindowText(message);

	string message_str = _mb2u8_(message);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], message_str.c_str(), message_str.length(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send(messagebox) fail");
		return;
	}

	MessageBox("发送弹窗成功", "提示", MB_ICONINFORMATION | MB_OK);
}

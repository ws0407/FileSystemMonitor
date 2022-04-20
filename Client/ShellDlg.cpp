// ShellDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "ShellDlg.h"
#include "RemoteMonitorDlg.h"
#include "afxdialogex.h"
#include <json.h>


extern CRemoteMonitorDlg* pLogin; //全局变量


IMPLEMENT_DYNAMIC(ShellDlg, CDialogEx)

ShellDlg::ShellDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SHELL, pParent)
{

}

ShellDlg::~ShellDlg()
{
}

void ShellDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SHELL, m_edit_shell);
}


BEGIN_MESSAGE_MAP(ShellDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &ShellDlg::OnBnClickedCancel)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// ShellDlg 消息处理程序
string u82mb(const char* cont)
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

BOOL ShellDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化
	CString title;
	title.Format("主机%s:%s的终端", pLogin->ip_addr_vec[(pLogin->host_index) - 1], pLogin->port_vec[(pLogin->host_index) - 1]);
	this->SetWindowText(title);

	brush.CreateSolidBrush(RGB(0, 0, 0));

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "shell_start", sizeof("shell_start"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(shell start) fail");
		return TRUE;
	}
	char shell_tip_[1024] = {};

	int num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], shell_tip_, Bufsize, 0);
	//MessageBox(file_list_);
	if (num == SOCKET_ERROR) {
		MessageBox("recv tip error");
		return TRUE;
	}

	shell_tip = shell_tip_;

	num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], shell_tip_, Bufsize, 0);
	//MessageBox(file_list_);
	if (num == SOCKET_ERROR) {
		MessageBox("recv tip error");
		return TRUE;
	}
	string temp_shell_tip = u82mb(shell_tip_);

	shell_log = shell_tip + "\r\n" + temp_shell_tip.c_str() + ">";

	shell_tip == temp_shell_tip.c_str();
	GetDlgItem(IDC_EDIT_SHELL)->SetWindowTextA(shell_log);

	m_edit_shell.SetSel(shell_log.GetLength(),shell_log.GetLength(),false);
	m_edit_shell.SetFocus();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void ShellDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


BOOL ShellDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN && GetFocus() == GetDlgItem(IDC_EDIT_SHELL)) {
		DWORD selectedRegion = ((CEdit*)GetDlgItem(IDC_EDIT_SHELL))->GetSel();
		int selectedStart = LOWORD(selectedRegion);
		int selectedEnd = HIWORD(selectedRegion);
		if (selectedStart != selectedEnd && selectedStart < shell_log.GetLength())
			return true;

		if (pMsg->wParam == VK_BACK && selectedStart <= shell_log.GetLength()) // 阻止删除之前的文字
			return true;

		if (pMsg->wParam == VK_RETURN) // 当输入回车时发送消息
			this->SendCommand();
	}


	return CDialogEx::PreTranslateMessage(pMsg);
}

void  ShellDlg::SendCommand() {
	CString text;
	GetDlgItem(IDC_EDIT_SHELL)->GetWindowTextA(text);
	shell_cmd = text.Right(text.GetLength() - shell_log.GetLength());
	//处理shell_cmd
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], shell_cmd, shell_cmd.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(cmd) fail");
		return;
	}

	char _tip_[Bufsize];
	char _rtn_[ShellRtn];
	memset(_tip_, 0, Bufsize);
	memset(_rtn_, 0, ShellRtn);

	if (shell_cmd == "exit") {
		int num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], _rtn_, ShellRtn, 0);
		CString shell_rtn = _rtn_;
		if (num == SOCKET_ERROR) {
			MessageBox("recv rtn error");
			return;
		}

		shell_log = text + "\r\n" + shell_rtn +"\r\n";
		GetDlgItem(IDC_EDIT_SHELL)->SetWindowTextA(shell_log);
		m_edit_shell.SetSel(shell_log.GetLength(), shell_log.GetLength(), false);

		return;
	}

	int num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], _rtn_, ShellRtn, 0);
	if (num == SOCKET_ERROR) {
		MessageBox("recv rtn error");
		return;
	}

	string mb_rtn = u82mb(_rtn_);

	num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], _tip_, Bufsize, 0);
	if (num == SOCKET_ERROR) {
		MessageBox("recv tip error");
		return;
	}
	string mb_tip = u82mb(_tip_);

	shell_tip = mb_tip.c_str();
	CString shell_rtn = mb_rtn.c_str();

	shell_log = text + "\r\n" + shell_rtn + "\r\n" + shell_tip +">";

	GetDlgItem(IDC_EDIT_SHELL)->SetWindowTextA(shell_log);
	m_edit_shell.SetSel(shell_log.GetLength() , shell_log.GetLength(), false);
}

HBRUSH ShellDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if (IDC_EDIT_SHELL == pWnd->GetDlgCtrlID())//判断发出消息的空间是否是该静态文本框
	{
		//pDC->SetTextColor(RGB(255, 255, 255));//设置文本颜色为白色
		//pDC->SetBkMode(TRANSPARENT);
	}
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

bool UTF8ToMB(vector<char>& pmb, const char* pu8, int utf8Len)
{
	// convert an UTF8 string to widechar   
	int nLen = MultiByteToWideChar(CP_UTF8, 0, pu8, utf8Len, NULL, 0);

	WCHAR* lpszW = NULL;
	try
	{
		lpszW = new WCHAR[nLen];
	}
	catch (bad_alloc& memExp)
	{
		return false;
	}

	int nRtn = MultiByteToWideChar(CP_UTF8, 0, pu8, utf8Len, lpszW, nLen);

	if (nRtn != nLen)
	{
		delete[] lpszW;
		return false;
	}

	// convert an widechar string to Multibyte   
	int MBLen = WideCharToMultiByte(CP_ACP, 0, lpszW, nLen, NULL, 0, NULL, NULL);
	if (MBLen <= 0)
	{
		return false;
	}
	pmb.resize(MBLen);
	nRtn = WideCharToMultiByte(CP_ACP, 0, lpszW, nLen, &*pmb.begin(), MBLen, NULL, NULL);
	delete[] lpszW;

	if (nRtn != MBLen)
	{
		pmb.clear();
		return false;
	}
	return true;
}


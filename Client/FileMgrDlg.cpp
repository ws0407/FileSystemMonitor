// FileMgrDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteMonitor.h"
#include "FileMgrDlg.h"
#include "afxdialogex.h"
#include "RemoteMonitorDlg.h"
#include <json.h>

using namespace Json;

extern CRemoteMonitorDlg* pLogin; //全局变量

// FileMgrDlg 对话框

IMPLEMENT_DYNAMIC(FileMgrDlg, CDialogEx)

FileMgrDlg::FileMgrDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_FILE, pParent)
{

}

FileMgrDlg::~FileMgrDlg()
{
}

void FileMgrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FILE, file_list);
	DDX_Control(pDX, IDC_EDIT_FILE, file_addr);
}


BEGIN_MESSAGE_MAP(FileMgrDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &FileMgrDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_TURNTO, &FileMgrDlg::OnBnClickedButtonTurnto)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &FileMgrDlg::OnNMRClickListFile)
	ON_COMMAND(ID_LINK_DETAIL, &FileMgrDlg::OnLinkDetail)
	ON_COMMAND(ID_LINK_DECODING, &FileMgrDlg::OnLinkDecoding)
	ON_COMMAND(ID_LINK_ENCODING, &FileMgrDlg::OnLinkEncoding)
	ON_COMMAND(ID_LINK_DELETE, &FileMgrDlg::OnLinkDelete)
	ON_COMMAND(ID_LINK_DETELE_ALL, &FileMgrDlg::OnLinkDeteleAll)
	ON_COMMAND(ID_LINK_OPEN_FOLDER, &FileMgrDlg::OnLinkOpenFolder)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FILE, &FileMgrDlg::OnNMDblclkListFile)
	ON_BN_CLICKED(IDC_BUTTON_FILE_RETURN, &FileMgrDlg::OnBnClickedButtonFileReturn)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_FILE, &FileMgrDlg::OnBnClickedButtonSelectFile)
	ON_BN_CLICKED(IDC_BUTTON_UPLOAD_FILE, &FileMgrDlg::OnBnClickedButtonUploadFile)
	ON_COMMAND(ID_LINK_DOWNLOAD, &FileMgrDlg::OnLinkDownload)
END_MESSAGE_MAP()

// FileMgrDlg 消息处理程序

void FileMgrDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}

string u82mb_(const char* cont)
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

string mb2u8_(const char* cont)
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

BOOL FileMgrDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString ip_str = pLogin->ip_addr_vec[pLogin->host_index - 1];
	CString port_str = pLogin->port_vec[pLogin->host_index - 1];

	GetDlgItem(IDC_STATIC_HOST_INFOMATION)->SetWindowText("当前主机地址：" + ip_str + ":" + port_str);
	//添加列表信息
	CRect rt;
	GetClientRect(&rt);
	m_IconList.Create(16, 16, 255, 255, 255);
	m_IconList.Add(AfxGetApp()->LoadIcon(IDI_ICON_FOLDER));
	m_IconList.Add(AfxGetApp()->LoadIcon(IDI_ICON_FILE));

	file_list.SetImageList(&m_IconList, LVSIL_SMALL);
	file_list.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT |
		LVS_EDITLABELS | LVS_EX_SUBITEMIMAGES);

	file_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	file_list.InsertColumn(0, _T("名称"), LVCFMT_LEFT, 400, 1);	//括号里面的信息分别是 第几列，内容，居中/靠左/..  表格宽度  子框（没用）
	file_list.InsertColumn(1, _T("类型"), LVCFMT_CENTER, 100, 1);
	file_list.InsertColumn(2, _T("加密状态"), LVCFMT_CENTER, 100, 1);
	file_list.InsertColumn(3, _T("修改时间"), LVCFMT_CENTER, 235, 1);

	CWnd* pWnd = GetDlgItem(IDC_EDIT_FILE);
	pWnd->SetWindowText("root\\");

	OnBnClickedButtonTurnto();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

void FileMgrDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	int nItem = file_list.GetSelectionMark();
	if (nItem >= file_list.GetItemCount()) return;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->iItem != -1)
	{
		CPoint pt;
		GetCursorPos(&pt);
		CMenu menu;
		CMenu* popmenu;
		if (file_list.GetItemText(pNMListView->iItem, 1) == _T("文件夹")) {
			menu.LoadMenu(IDR_MENU_FOLDER);
			popmenu = menu.GetSubMenu(0);
			popmenu->TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
		}
		else {
			menu.LoadMenu(IDR_MENU_FILE);
			if (file_list.GetItemText(pNMListView->iItem, 2) == _T("未加密")) {
				menu.EnableMenuItem(ID_LINK_DECODING, MF_DISABLED);
			}
			else {
				menu.EnableMenuItem(ID_LINK_ENCODING, MF_DISABLED);
			}
			popmenu = menu.GetSubMenu(0);
			popmenu->TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
		}
	}

	*pResult = 0;
}

void FileMgrDlg::OnLinkDetail()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = file_list.GetSelectionMark();
	CString file_name = file_list.GetItemText(nItem, 0);
	CString file_type = file_list.GetItemText(nItem, 1);
	CString file_status = file_list.GetItemText(nItem, 2);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "file_information", sizeof("file_information"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send cmd file_information error");

	CString file_remote_path;
	file_remote_path = GetRemotePath();

	Sleep(500);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], file_remote_path, file_remote_path.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send file path error");

	char file_info[ShellRtn];
	memset(file_info, 0, ShellRtn);
	int num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], file_info, ShellRtn, 0);
	string _file_info_ = u82mb_(file_info);
	MessageBox(_file_info_.c_str());
	if (num == SOCKET_ERROR)
	{
		MessageBox("recv file info error");
		return;
	}

	Json::Reader reader;
	Json::Value value;
	if (reader.parse(_file_info_, value))
	{
		int _file_size_ = value["file_size"].asInt() /1024;
		CString last_modification_time = value["last_modification_time"].asCString();
		char szInfo[100];
		wsprintf(szInfo, "文件名：%s\n文件类型：%s\n文件大小：%d KB\n加密状态：%s\n修改日期：%s\n", file_name, file_type, _file_size_, file_status, last_modification_time);
		MessageBox(szInfo, "文件信息");
	}
}

void FileMgrDlg::OnLinkDecoding()
{
	CString file_remote_path;
	file_remote_path = GetRemotePath();

	int nItem = file_list.GetSelectionMark();
	CString file_type = file_list.GetItemText(nItem, 1);
	CString file_status = file_list.GetItemText(nItem, 2);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "file_decrypt", sizeof("file_decrypt"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(encrypt) fail");
		return;
	}

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], file_remote_path, file_remote_path.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(file_path) fail");
		return;
	}

	if (file_status == "未加密")
		MessageBox("该文件未被加密，无需解密", "警告", MB_ICONWARNING | MB_OK);
	else {
		CTime tm;
		tm = CTime::GetCurrentTime();
		//设置当前时间
		CString time_str = tm.Format("%Y年%m月%d日%X");

		FILE* send_fopen = NULL;
		CString file_path = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\FileOperationRecord.txt";
		errno_t err = fopen_s(&send_fopen, file_path, "a+");
		if (send_fopen == NULL) {
			MessageBox("file fail");
			return;
		}
		string tmp = u82mb_(file_remote_path);
		CString log_str = time_str + "\t||\t" + tmp.c_str() + "\t||\tdecrypt\n";
		fwrite(log_str, log_str.GetLength(), 1, send_fopen);
		fclose(send_fopen);
		//返回解密成功消息
		file_list.SetItemText(nItem, 2, "未加密");
		MessageBox("解密成功", "提示", MB_ICONINFORMATION | MB_OK);
	}
}

void FileMgrDlg::OnLinkEncoding()
{
	CString file_remote_path;
	file_remote_path = GetRemotePath();

	int nItem = file_list.GetSelectionMark();
	CString file_type = file_list.GetItemText(nItem, 1);
	CString file_status = file_list.GetItemText(nItem, 2);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "file_encrypt", sizeof("file_encrypt"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(encrypt) fail");
		return;
	}

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], file_remote_path, file_remote_path.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
		MessageBox("send(file_path) fail");
		return;
	}

	if (file_status == "已加密")
		MessageBox("该文件已加密，无需加密", "警告", MB_ICONWARNING | MB_OK);
	else {
		CTime tm;
		tm = CTime::GetCurrentTime();
		//设置当前时间
		CString time_str = tm.Format("%Y:%m:%d:%X");

		FILE* send_fopen = NULL;
		CString file_path = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\FileOperationRecord.txt";
		errno_t err = fopen_s(&send_fopen, file_path, "a+");
		if (send_fopen == NULL) {
			MessageBox("file fail");
			return;
		}
		string tmp = u82mb_(file_remote_path);
		CString log_str = time_str + "\t||\t" + tmp.c_str() + "\t||\tencrypt\n";
		fwrite(log_str, log_str.GetLength(), 1, send_fopen);
		fclose(send_fopen);

		//返回加密成功消息
		file_list.SetItemText(nItem, 2, "已加密");
		MessageBox("加密成功", "提示", MB_ICONINFORMATION | MB_OK);
	}
}

void FileMgrDlg::OnLinkDelete()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = file_list.GetSelectionMark();
	CString file_name = file_list.GetItemText(nItem, 0);
	CString file_type = file_list.GetItemText(nItem, 1);
	char szInfo[100];
	wsprintf(szInfo, "确认删除文件:\n%s.%s\n该操作不可恢复！！", file_name, file_type);
	if (MessageBox(szInfo, TEXT("警告"), MB_ICONWARNING | MB_YESNO) == IDNO) {
		return;
	}
	//发送返回删除消息

	//
	file_list.DeleteItem(nItem);
	MessageBox("删除成功", "提示", MB_ICONINFORMATION | MB_OK);
}

void FileMgrDlg::OnLinkDeteleAll()
{
	// TODO: 在此添加命令处理程序代码
	// TODO: 在此添加命令处理程序代码
	int nItem = file_list.GetSelectionMark();
	CString file_name = file_list.GetItemText(nItem, 0);
	char szInfo[100];
	wsprintf(szInfo, "确认删除文件夹: %s 及内所有子文件？\n该操作不可恢复！！！", file_name);
	if (MessageBox(szInfo, TEXT("警告"), MB_ICONWARNING | MB_YESNO) == IDNO) {
		return;
	}
	//发送返回删除消息

	//
	file_list.DeleteItem(nItem);
	MessageBox("删除成功", "提示", MB_ICONINFORMATION | MB_OK);

}

void FileMgrDlg::OnLinkOpenFolder()
{
	// TODO: 在此添加命令处理程序代码
	CString directory;
	GetDlgItem(IDC_EDIT_FILE)->GetWindowText(directory);
	int nItem = file_list.GetSelectionMark();
	CString folder = file_list.GetItemText(nItem, 0);

	CWnd* pWnd = GetDlgItem(IDC_EDIT_FILE);
	if (directory[directory.GetLength() - 2] == ':') {
		pWnd->SetWindowText(directory + folder);
	}
	else
		pWnd->SetWindowText(directory + "\\" + folder);

	OnBnClickedButtonTurnto();

}

void FileMgrDlg::OnNMDblclkListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	int nItem = file_list.GetSelectionMark();
	if (nItem >= file_list.GetItemCount()) return;

	CString file_type = file_list.GetItemText(nItem, 1);

	if (file_type == "文件夹")
		FileMgrDlg::OnLinkOpenFolder();
	else {
		//查看文件详情
		FileMgrDlg::OnLinkDetail();
	}
	*pResult = 0;
}

void FileMgrDlg::OnBnClickedButtonTurnto()
{
	// TODO: 在此添加控件通知处理程序代码
	CString directory;
	GetDlgItem(IDC_EDIT_FILE)->GetWindowText(directory);

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "get_file_list", sizeof("get_file_list"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send cmd error");
	string tmp = mb2u8_(directory);
	directory = tmp.c_str();
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], directory, directory.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
		MessageBox("send path error");

	char file_list_[ShellRtn];
	memset(file_list_, 0, ShellRtn);
	int num = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], file_list_, ShellRtn, 0);
	string _file_list_ = u82mb_(file_list_);
	if (num == SOCKET_ERROR)
	{
		MessageBox("recv error");
		return;
	}

	file_list.DeleteAllItems();

	Json::Reader reader;
	Json::Value value;
	if (reader.parse(_file_list_, value))
	{
		CString strFileName = "log\\" + pLogin->ip_addr_vec[pLogin->host_index - 1] + "\\FileOperationRecord.txt";
		if (!PathFileExists(strFileName)) return;
		CStdioFile file;
		if (!file.Open(strFileName, CFile::modeRead)) return;
		std::vector<CString> vecResult;
		CString strValue = _T("");
		while (file.ReadString(strValue))
		{
			vecResult.push_back(strValue);
		}
		file.Close();

		CString _front_;
		GetDlgItem(IDC_EDIT_FILE)->GetWindowText(_front_);
		CString _dir_;
		if (_front_[_front_.GetLength() - 1] == '\\' && _front_.GetLength() > 6)
			_front_ = _front_.Left(_front_.GetLength() - 1);
		if (_front_.GetLength() >= 6)
			_front_ = _front_.Right(_front_.GetLength() - 6);

		for (unsigned int i = 0; i < value["files"].size(); i++)
		{
			string tmp = u82mb_(value["files"][i].asString().c_str());
			CString file = tmp.c_str();
			int type_index = file.ReverseFind('.');

			char szNum[100];
			wsprintf(szNum, "%s", type_index == -1 ? file : file.Left(type_index));

			LV_ITEM lvitem;
			memset((char*)&lvitem, '\0', sizeof(LV_ITEM));
			lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
			lvitem.iItem = i;
			lvitem.iImage = 1;
			lvitem.iSubItem = 0;
			lvitem.stateMask = 0;
			lvitem.pszText = szNum;
			file_list.InsertItem(&lvitem);

			//file_list.SetItemText(i, 0, type_index == -1 ? file : file.Left(type_index));
			file_list.SetItemText(i, 1, type_index == -1 ? "" : file.Right(file.GetLength() - type_index - 1));

			_dir_ = _front_ + "\\" + file;
			bool flag = true;
			for (vector<CString>::iterator v = vecResult.end() - 1; vecResult.size() > 0; v--) {
				if ((*v).Find(_dir_) != -1) {
					if ((*v).Find("encrypt") != -1) {
						file_list.SetItemText(i, 2, "已加密");
						flag = false;
						break;
					}
					else if ((*v).Find("decrypt") != -1) {
						break;
					}

				}
				if (v == vecResult.begin()) break;
			}
			if (flag) file_list.SetItemText(i, 2, "未加密");
			CTime tm;
			tm = CTime::GetCurrentTime();
			//设置当前时间
			CString time_str = tm.Format("%Y年%m月%d日%X");
			
			file_list.SetItemText(i, 3, time_str);
		}

		string dirname = value["dirname"].asString();
		for (unsigned int i = 0; i < value["child_dirs"].size(); i++)
		{
			string tmp = u82mb_(value["child_dirs"][i].asString().c_str());
			CString child = tmp.c_str();

			char szNum[100];
			wsprintf(szNum, "%s", child);

			LV_ITEM lvitem;
			memset((char*)&lvitem, '\0', sizeof(LV_ITEM));
			lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
			lvitem.iItem = i;
			lvitem.iImage = 0;
			lvitem.iSubItem = 0;
			lvitem.stateMask = 0;
			lvitem.pszText = szNum;

			file_list.InsertItem(&lvitem);
			file_list.SetItemText(i, 1, "文件夹");
			file_list.SetItemText(i, 2, "/");
			file_list.SetItemText(i, 3, _T("2020年08月02日12:33"));
		}

	}
}

void FileMgrDlg::OnBnClickedButtonFileReturn()
{
	// TODO: 在此添加控件通知处理程序代码
	CString directory, back_directory;
	GetDlgItem(IDC_EDIT_FILE)->GetWindowText(directory);

	if (directory == "root\\" || directory == "root") {
		return;
	}
	if (directory[directory.GetLength() - 1] == '\\')
		directory = directory.Left(directory.GetLength() - 1);

	int index = directory.ReverseFind('\\');

	back_directory = directory.Left(index);

	if (back_directory[back_directory.GetLength() - 1] == ':')
		back_directory += "\\";

	CWnd* pWnd = GetDlgItem(IDC_EDIT_FILE);
	pWnd->SetWindowText(back_directory);

	OnBnClickedButtonTurnto();

}

void FileMgrDlg::OnBnClickedButtonSelectFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE, NULL, NULL,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, _T("All Files (*.*)|*.*||"),
		this);
	dlg.m_ofn.lpstrTitle = "请选择要上传的文件";
	if (dlg.DoModal() == IDOK)
	{
		CString file_local_path = dlg.GetPathName();
		SetDlgItemText(IDC_STATIC_FILE_SELECT, file_local_path);
	}
}

void FileMgrDlg::OnBnClickedButtonUploadFile()
{
	// TODO: 在此添加控件通知处理程序代码

	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "file_upload", sizeof("file_upload"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send() fail");
		return;
	}

	Sleep(500);

	SetDlgItemText(IDC_BUTTON_UPLOAD_FILE, "正在上传...");
	GetDlgItem(IDC_BUTTON_UPLOAD_FILE)->EnableWindow(false);
	GetDlgItem(IDC_BUTTON_SELECT_FILE)->EnableWindow(false);

	CString file_path;
	GetDlgItemText(IDC_STATIC_FILE_SELECT, file_path);
	//上传代码
	CString file_remote_path, file_local_path;
	GetDlgItem(IDC_EDIT_FILE)->GetWindowText(file_remote_path);
	GetDlgItem(IDC_STATIC_FILE_SELECT)->GetWindowText(file_local_path);

	char recvBuf[Bufsize];
	char sendBuf[Bufsize];
	memset(recvBuf, 0, sizeof(sendBuf));
	memset(sendBuf, 0, sizeof(sendBuf));
	CString tmp = file_remote_path.Right(file_remote_path.GetLength() - 6);
	int index = file_local_path.ReverseFind('\\');
	if (tmp[tmp.GetLength() - 1] == '\\')
		tmp = tmp.Left(tmp.GetLength() - 1);
	tmp = tmp + "\\" + file_local_path.Right(file_local_path.GetLength() - index - 1);
	string tmp_str = mb2u8_(tmp);
	tmp = tmp_str.c_str();
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], tmp, tmp.GetLength(), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send() fail");
		return;
	}

	Sleep(500);

	FILE* server_file;
	errno_t err = fopen_s(&server_file, file_local_path, "rb");
	if (server_file == NULL)
	{
		MessageBox("open file fail");
		return;
	}
	while (pLogin->sendLen_vec[pLogin->host_index - 1] = fread(sendBuf, 1, Bufsize, server_file) > 0)
	{
		send(pLogin->clientSocket_vec[pLogin->host_index - 1], sendBuf, Bufsize, 0);
	}
	fclose(server_file);
	Sleep(3000);
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "end", sizeof("end"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send(end) fail");
		return;
	}

	SetDlgItemText(IDC_BUTTON_UPLOAD_FILE, "上传");
	GetDlgItem(IDC_BUTTON_UPLOAD_FILE)->EnableWindow(true);
	GetDlgItem(IDC_BUTTON_SELECT_FILE)->EnableWindow(true);

	MessageBox("上传成功", "提示", MB_ICONINFORMATION | MB_OK);
	//再重新刷新列表
	OnBnClickedButtonTurnto();
}

void FileMgrDlg::OnLinkDownload()
{
	// TODO: 在此添加命令处理程序代码
	pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], "file_download", sizeof("file_download"), 0);
	if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR)
	{
		MessageBox("send() fail");
		return;
	}

	CString file_remote_path, file_local_path;
	file_remote_path = GetRemotePath();

	int nItem = file_list.GetSelectionMark();
	CString file_type = file_list.GetItemText(nItem, 1);

	// 获取特定文件夹的LPITEMIDLIST，可以将之理解为HANDLE 所谓的特定文件夹,可以用CSIDL_XXX来检索之。
	LPITEMIDLIST rootLoation;
	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &rootLoation);
	if (rootLoation == NULL) {
		// unkown error
		return;
	}
	// 配置对话框
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.pidlRoot = rootLoation; // 文件夹对话框之根目录，不指定的话则为我的电脑
	bi.lpszTitle = _T("对话框抬头"); // 可以不指定
	// bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS;

	// 打开对话框, 有点像DoModal
	LPITEMIDLIST targetLocation = SHBrowseForFolder(&bi);
	if (targetLocation != NULL) {
		TCHAR targetPath[MAX_PATH];
		SHGetPathFromIDList(targetLocation, targetPath);
		if (file_type.GetLength() != 0)
			file_local_path.Format("%s\\%s.%s", targetPath, file_list.GetItemText(nItem, 0), file_type);
		else
			file_local_path.Format("%s\\%s", targetPath, file_list.GetItemText(nItem, 0));

		pLogin->sendLen_vec[pLogin->host_index - 1] = send(pLogin->clientSocket_vec[pLogin->host_index - 1], file_remote_path, file_remote_path.GetLength(), 0);
		if (pLogin->sendLen_vec[pLogin->host_index - 1] == SOCKET_ERROR) {
			MessageBox("send() fail");
			return;
		}

		FILE* send_fopen = NULL;
		errno_t err = fopen_s(&send_fopen, file_local_path, "wb");
		if (send_fopen == NULL) {
			MessageBox("file fail");
			return;
		}
		int file_flag = 1;
		char recvBuf[Bufsize];
		char sendBuf[Bufsize];
		memset(recvBuf, 0, sizeof(sendBuf));
		memset(sendBuf, 0, sizeof(sendBuf));
		while (file_flag) {
			int length = 0;
			//接受到客户端发来的文件数据
			length = recv(pLogin->clientSocket_vec[pLogin->host_index - 1], recvBuf, sizeof(recvBuf), 0);
			//写入
			//cout << recvBuf << endl;
			if (!strncmp(recvBuf, "end", sizeof("end")))
			{
				file_flag = 0;
				break;
			}
			fwrite(recvBuf, length, 1, send_fopen);
			memset(recvBuf, 0, sizeof(sendBuf));
		}
		fclose(send_fopen);

		CString text;
		text.Format("文件已成功下载到：%s", file_local_path);
		MessageBox(text, "提示", MB_ICONWARNING | MB_OK);
	}
	else {
		return;
	}
}

CString FileMgrDlg::GetRemotePath() {
	CString file_remote_path;
	GetDlgItem(IDC_EDIT_FILE)->GetWindowText(file_remote_path);

	int nItem = file_list.GetSelectionMark();
	CString file_type = file_list.GetItemText(nItem, 1);

	CString tmp = file_remote_path.Right(file_remote_path.GetLength() - 6);
	if (tmp[tmp.GetLength() - 1] == '\\')
		tmp = tmp.Left(tmp.GetLength() - 1);

	if (file_type.GetLength() != 0) {
		file_remote_path.Format("%s\\%s.%s", tmp, file_list.GetItemText(nItem, 0), file_type);
	}
	else
		file_remote_path.Format("%s\\%s", tmp, file_list.GetItemText(nItem, 0));

	string path_str = mb2u8_(file_remote_path);
	file_remote_path = path_str.c_str();

	return file_remote_path;
}
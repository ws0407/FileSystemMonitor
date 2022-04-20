
// RemoteMonitorDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteMonitor.h"
#include "RemoteMonitorDlg.h"
#include "afxdialogex.h"
#include "FileMgrDlg.h"
#include "HostDetailDlg.h"
#include "NewConnectionDlg.h"
#include "ShellDlg.h"
#include "MessageBoxDlg.h"
#include<json.h>
#include <thread>
#include <Windows.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;
using namespace Json;

CRemoteMonitorDlg* pLogin;	//定义一个全局变量，用于向另外的对话框传递信息

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteMonitorDlg 对话框



CRemoteMonitorDlg::CRemoteMonitorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTEMONITOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_RADAR);
}

void CRemoteMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_HOST_LIST, host_list);
}

BEGIN_MESSAGE_MAP(CRemoteMonitorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CRemoteMonitorDlg::OnBnClickedButton1)
	ON_NOTIFY(NM_RCLICK, IDC_HOST_LIST, &CRemoteMonitorDlg::OnNMRClickHostList)
	ON_BN_CLICKED(IDC_BUTTON_HOST_DETAIL, &CRemoteMonitorDlg::OnBnClickedButtonHostDetail)
	ON_COMMAND(ID_LINK_CONNECT, &CRemoteMonitorDlg::OnLinkConnect)
	ON_COMMAND(ID_LINK_DISCONNECT, &CRemoteMonitorDlg::OnLinkDisconnect)
	ON_COMMAND(ID_LINK_FILEMGR, &CRemoteMonitorDlg::OnLinkFilemgr)
	ON_COMMAND(ID_LINK_HOST_DETAIL, &CRemoteMonitorDlg::OnLinkHostDetail)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_DBLCLK, IDC_HOST_LIST, &CRemoteMonitorDlg::OnNMDblclkHostList)
	ON_COMMAND(ID_CONNECT, &CRemoteMonitorDlg::OnConnect)
	ON_COMMAND(ID_LINK_TERMINAL, &CRemoteMonitorDlg::OnLinkTerminal)
	ON_COMMAND(ID_LINK_DELETE_HOST, &CRemoteMonitorDlg::OnLinkDeleteHost)
	ON_COMMAND(ID_LINK_MESSAGEBOX, &CRemoteMonitorDlg::OnLinkMessagebox)
END_MESSAGE_MAP()


// CRemoteMonitorDlg 消息处理程序

BOOL CRemoteMonitorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	pLogin = this;

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	// 将“关于...”菜单项添加到系统菜单中。
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	font.CreatePointFont(200, "华文行楷");
	brush.CreateSolidBrush(RGB(0, 0, 255));

	GetDlgItem(IDC_STATIC_HOME_CAPTION)->SetFont(&font);

	//添加列表信息
	CRect rt;
	GetClientRect(&rt);
	m_IconList.Create(16, 16, 255, 255, 255);
	m_IconList.Add(AfxGetApp()->LoadIcon(IDI_ICON_WRONG));
	m_IconList.Add(AfxGetApp()->LoadIcon(IDI_ICON_YES));

	host_list.SetImageList(&m_IconList, LVSIL_SMALL);
	host_list.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT |
		LVS_EDITLABELS | LVS_EX_SUBITEMIMAGES);

	host_list.InsertColumn(0, _T("序号"), LVCFMT_CENTER, rt.Width() / 6, 1);	//括号里面的信息分别是 第几列，内容，居中/靠左/..  表格宽度  子框（没用）
	host_list.InsertColumn(1, _T("地址"), LVCFMT_CENTER, rt.Width() / 3.05, 1);
	host_list.InsertColumn(2, _T("端口号"), LVCFMT_CENTER, rt.Width() / 8, 1);
	host_list.InsertColumn(3, _T("连接状态"), LVCFMT_CENTER, rt.Width() / 3, 1);
	
	//创建日志文件夹
	CString Path("log");
	BOOL rec = PathFileExists(Path);
	if (!rec) {
		system("mkdir log");
	}

	//加载套接字库
	WSADATA wsaData;
	iRet = 0;
	iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iRet != 0)
	{
		cout << "WSAStartup(MAKEWORD(2, 2), &wsaData) execute failed!" << endl;;
		return -1;
	}
	if (2 != LOBYTE(wsaData.wVersion) || 2 != HIBYTE(wsaData.wVersion))
	{
		WSACleanup();
		cout << "WSADATA version is not correct!" << endl;
		return -1;
	}

	//创建套接字
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		cout << "serverSocket = socket(AF_INET, SOCK_STREAM, 0) execute failed!" << endl;
		return -1;
	}

	//初始化服务器地址族变量
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);

	//绑定
	iRet = bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
	if (iRet == SOCKET_ERROR)
	{
		cout << "bind(serverSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) execute failed!" << endl;
		return -1;
	}


	//监听
	iRet = listen(serverSocket, CONNECT_NUM_MAX);
	if (iRet == SOCKET_ERROR)
	{
		cout << "listen(serverSocket, 10) execute failed!" << endl;
		return -1;
	}

	//cout << ("server is start,waiting for client.") << endl;
	CFileFind finder;
	CString strPath;
	BOOL bWorking = finder.FindFile("log//*.*");
	connSocket = 0;
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		strPath = finder.GetFilePath();

		//strPath就是所要获取Test目录下的文件夹和文件（包括路径）
		int index = strPath.ReverseFind('\\');
		CString ip_addr = strPath.Right(strPath.GetLength() - index - 1);
		if (ip_addr.GetLength() < 7)
			continue;
		pLogin->ip_addr_vec.push_back(ip_addr);
		pLogin->port_vec.push_back("/");
		pLogin->connect_status_vec.push_back(false);
		pLogin->sendLen_vec.push_back(0);
		pLogin->clientSocket_vec.push_back(connSocket);
	}

	char szNum[20];
	int i = 0;
	for (vector<CString>::iterator v = ip_addr_vec.begin(); v != ip_addr_vec.end(); v++) {
		wsprintf(szNum, "     %u", i + 1);		// int 转成字符串
		
		LV_ITEM lvitem;
		memset((char*)&lvitem, '\0', sizeof(LV_ITEM));
		lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvitem.iItem = i;
		lvitem.iImage = 0;
		lvitem.iSubItem = 0;
		lvitem.stateMask = 0;
		lvitem.pszText = szNum;
		host_list.InsertItem(&lvitem);
		
		//host_list.SetItemText(i, 0, szNum);	// 设置第1列(序号)
		host_list.SetItemText(i, 1, ip_addr_vec[i]);	// 设置第2列(地址)
		host_list.SetItemText(i, 2, port_vec[i]);	// 设置第3列()
		host_list.SetItemText(i, 3, connect_status_vec[i] == true ? "已连接" : "未连接");	// 设置第4列()
		i++;
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteMonitorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteMonitorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteMonitorDlg::OnBnClickedButton1()
{
	INT_PTR nRes;
	FileMgrDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::OnNMRClickHostList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	if (pNMListView->iItem != -1)
	{
		CPoint pt;
		GetCursorPos(&pt);
		CMenu menu;
		menu.LoadMenu(IDR_MENU2);

		int nItem = host_list.GetSelectionMark();
		
		CString connect_status = host_list.GetItemText(nItem, 3);
		if (connect_status == "") {
			return;
		}
		else if (connect_status == "未连接") {
			menu.EnableMenuItem(ID_LINK_DISCONNECT, MF_DISABLED);
			menu.EnableMenuItem(ID_LINK_HOST_DETAIL, MF_DISABLED);
			menu.EnableMenuItem(ID_LINK_FILEMGR, MF_DISABLED);
			menu.EnableMenuItem(ID_LINK_TERMINAL, MF_DISABLED);
			menu.EnableMenuItem(ID_LINK_MESSAGEBOX, MF_DISABLED);
		}
		else {
			menu.EnableMenuItem(ID_LINK_CONNECT, MF_DISABLED);
		}

		CMenu* popmenu;
		popmenu = menu.GetSubMenu(0);
		popmenu->TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
	}


	*pResult = 0;
}

void CRemoteMonitorDlg::OnBnClickedButtonHostDetail()
{
	// TODO: 在此添加控件通知处理程序代码
	INT_PTR nRes;
	HostDetailDlg myDlg;       // 构造对话框类RegisterDlg的实例 
	nRes = myDlg.DoModal();  // 弹出对话框

}

HBRUSH CRemoteMonitorDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if (IDC_STATIC_HOME_CAPTION == pWnd->GetDlgCtrlID())//判断发出消息的空间是否是该静态文本框
	{
		pDC->SetTextColor(RGB(0, 0, 255));//设置文本颜色为红色

	}
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

void CRemoteMonitorDlg::OnNMDblclkHostList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	int nItem = host_list.GetSelectionMark();
	if (nItem >= connect_status_vec.size())
		return;

	host_index = nItem + 1;

	if (connect_status_vec[host_index - 1]) {
		INT_PTR nRes;
		HostDetailDlg myDlg;
		nRes = myDlg.DoModal();
	}
	else {
		MessageBox("该主机尚未连接，无法查看");
	}

	*pResult = 0;
}

void CRemoteMonitorDlg::OnConnect()
{
	// TODO: 在此添加命令处理程序代码
	host_index = -1;

	INT_PTR nRes;
	NewConnectionDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::RefreshHostList()
{
	// TODO: 在此添加命令处理程序代码
	host_list.DeleteAllItems();

	char szNum[10];
	int i = 0;
	for (vector<CString>::iterator v = ip_addr_vec.begin(); v != ip_addr_vec.end(); v++) {
		wsprintf(szNum, "     %u", i + 1);		// int 转成字符串

		LV_ITEM lvitem;
		memset((char*)&lvitem, '\0', sizeof(LV_ITEM));
		lvitem.mask = LVIF_TEXT | LVIF_IMAGE;
		lvitem.iItem = i;
		lvitem.iImage = connect_status_vec[i] == true?1:0;
		lvitem.iSubItem = 0;
		lvitem.stateMask = 0;
		lvitem.pszText = szNum;
		host_list.InsertItem(&lvitem);

		host_list.SetItemText(i, 0, szNum);	// 设置第1列(序号)
		host_list.SetItemText(i, 1, ip_addr_vec[i]);	// 设置第2列(地址)
		host_list.SetItemText(i, 2, port_vec[i]);	// 设置第3列()
		host_list.SetItemText(i, 3, connect_status_vec[i] == true ? "已连接" : "未连接");	// 设置第4列()
		i++;
	}

}

void CRemoteMonitorDlg::OnLinkConnect()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = host_list.GetSelectionMark();
	host_index = nItem + 1;

	INT_PTR nRes;
	NewConnectionDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::OnLinkDisconnect()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = host_list.GetSelectionMark();
	host_index = nItem + 1;

	CString host_ip_addr = ip_addr_vec[host_index - 1];
	CString host_port = port_vec[host_index - 1];
	//socket断开连接的代码
	bool disconnect_return = true;

	sendLen_vec[host_index - 1] = send(clientSocket_vec[host_index - 1], "exit", sizeof("exit"), 0);
	if (sendLen_vec[host_index - 1] == SOCKET_ERROR)
	{
		disconnect_return = false;
		MessageBox("send exit error");
	}

	closesocket(clientSocket_vec[host_index - 1]);

	//返回成功或者失败

	if (disconnect_return) {
		connect_status_vec[host_index - 1] = false;
		host_list.SetItemText(nItem, 3, "未连接");
		MessageBox("成功断开连接", "提示", MB_ICONINFORMATION | MB_OK);
		RefreshHostList();
	}
	else {
		MessageBox("断开连接失败", "提示", MB_ICONWARNING | MB_OK);
	}
}

void CRemoteMonitorDlg::OnLinkFilemgr()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = host_list.GetSelectionMark();
	host_index = nItem+1;

	INT_PTR nRes;
	FileMgrDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::OnLinkHostDetail()
{
	int nItem = host_list.GetSelectionMark();
	host_index = nItem + 1;

	INT_PTR nRes;
	HostDetailDlg myDlg;       // 构造对话框类RegisterDlg的实例 
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::OnLinkTerminal()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = host_list.GetSelectionMark();
	host_index = nItem + 1;

	INT_PTR nRes;
	ShellDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

void CRemoteMonitorDlg::OnLinkDeleteHost()
{
	// TODO: 在此添加命令处理程序代码
	int nItem = host_list.GetSelectionMark();
	if (host_list.GetItemText(nItem, 3) == "已连接") {
		MessageBox("请先关闭连接", "提示", MB_ICONWARNING | MB_OK);
		return;
	}
	if (MessageBox("确定删除改主机所有信息吗？", "提示", MB_ICONWARNING | MB_OKCANCEL) == IDOK) {
		ip_addr_vec.erase(ip_addr_vec.begin() + nItem);
		port_vec.erase(port_vec.begin() + nItem);
		connect_status_vec.erase(connect_status_vec.begin() + nItem);
		sendLen_vec.erase(sendLen_vec.begin() + nItem);
		clientSocket_vec.erase(clientSocket_vec.begin() + nItem);
		MessageBox("删除成功", "提示", MB_ICONINFORMATION | MB_OK);
		RefreshHostList();
	}
	else {
		return;
	}
}

void CRemoteMonitorDlg::OnLinkMessagebox()
{
	int nItem = host_list.GetSelectionMark();
	host_index = nItem + 1;

	INT_PTR nRes;
	MessageBoxDlg myDlg;
	nRes = myDlg.DoModal();  // 弹出对话框
}

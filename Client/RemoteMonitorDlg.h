
// RemoteMonitorDlg.h: 头文件
//
#include <vector>
#include <stdarg.h>
#include <time.h>
#include <string>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#pragma once

#define CONNECT_NUM_MAX 10
#define FILE_NAME_MAX_SIZE 100
#define Bufsize 1024
#define ShellRtn 4096

using namespace std;

// CRemoteMonitorDlg 对话框
class CRemoteMonitorDlg : public CDialogEx
{
// 构造
public:
	CRemoteMonitorDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTEMONITOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	CFont font;
	CBrush brush;
	CImageList m_IconList;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl host_list;	//
	vector<CString> ip_addr_vec;
	vector<CString> port_vec;
	vector<bool> connect_status_vec;
	int host_index;	//
	vector<int> sendLen_vec; //send标识
	vector<SOCKET> clientSocket_vec;	//主机标识

	SOCKET serverSocket;
	SOCKADDR_IN addrSrv;
	int iRet;

	SOCKADDR_IN clientAddr;
	int len;
	SOCKET connSocket;

	afx_msg void OnBnClickedButton1();
	afx_msg void OnNMRClickHostList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonHostDetail();
	afx_msg void OnLinkConnect();
	afx_msg void OnLinkDisconnect();
	afx_msg void OnLinkFilemgr();
	afx_msg void OnLinkHostDetail();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnNMDblclkHostList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnConnect();
	void RefreshHostList();
	afx_msg void OnLinkTerminal();
	afx_msg void OnLinkDeleteHost();
	afx_msg void OnLinkMessagebox();
};

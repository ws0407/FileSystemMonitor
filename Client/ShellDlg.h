#pragma once


// ShellDlg 对话框

class ShellDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ShellDlg)

public:
	ShellDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~ShellDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_SHELL };
#endif

protected:

	CFont font;
	CBrush brush;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString shell_log;
	CString shell_tip;
	CString shell_cmd;

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CEdit m_edit_shell;
	void SendCommand();
	//string u82mb(const char* cont);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
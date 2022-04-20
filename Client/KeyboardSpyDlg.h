#pragma once


// KeyboardSpyDlg 对话框

class KeyboardSpyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(KeyboardSpyDlg)

public:
	KeyboardSpyDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~KeyboardSpyDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_KEYBOARD_SPY };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl KeyBoardControlList;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonStop();
};

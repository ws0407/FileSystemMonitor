#pragma once


// MessageBoxDlg 对话框

class MessageBoxDlg : public CDialogEx
{
	DECLARE_DYNAMIC(MessageBoxDlg)

public:
	MessageBoxDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~MessageBoxDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CEdit m_edit_message;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
};

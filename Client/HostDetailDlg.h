#pragma once


// HostDetailDlg 对话框

class HostDetailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(HostDetailDlg)

public:
	void ShowPicture(CString pic_path);
	HostDetailDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~HostDetailDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_HOST_DETAIL };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	CFont hostDetailCaptionFont;

	afx_msg void OnBnClickedButtonReturn();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonKeyboardSpy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonScreenShot();
	afx_msg void OnBnClickedButtonScreenRecording();
	afx_msg void OnBnClickedButtonDisconnect();
};

#pragma once


// FileMgrDlg 对话框

class FileMgrDlg : public CDialogEx
{
	DECLARE_DYNAMIC(FileMgrDlg)

public:
	FileMgrDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~FileMgrDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_FILE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	CImageList m_IconList;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	CListCtrl file_list;
	afx_msg void OnBnClickedButtonTurnto();
	CEdit file_addr;
	virtual BOOL OnInitDialog();
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLinkDetail();
	afx_msg void OnLinkDecoding();
	afx_msg void OnLinkEncoding();
	afx_msg void OnLinkDelete();
	afx_msg void OnLinkDeteleAll();
	afx_msg void OnLinkOpenFolder();
	afx_msg void OnNMDblclkListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonFileReturn();
	afx_msg void OnBnClickedButtonSelectFile();
	afx_msg void OnBnClickedButtonUploadFile();
	afx_msg void OnLinkDownload();
	CString GetRemotePath();
};

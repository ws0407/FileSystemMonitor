﻿#pragma once


// NewConnectionDlg 对话框

class NewConnectionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(NewConnectionDlg)

public:
	NewConnectionDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~NewConnectionDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CONNECT };
#endif

protected:
	CFont font;
	CBrush brush;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};

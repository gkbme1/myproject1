#pragma once
#include "afxdialogex.h"


// CRuleManageDlg 对话框

class CRuleManageDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRuleManageDlg)

public:
	CRuleManageDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CRuleManageDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CRuleManageDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CButton m_work_5_day;
	CButton m_Night_MorningSwitch;
	CButton m_OneRound_ClassNum;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};

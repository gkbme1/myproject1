#pragma once
#include "afxdialogex.h"
#include "CDlgRequstRestDay.h"

// CDlgRestDayManagement 对话框

class CDlgRestDayManagement : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgRestDayManagement)

public:
	CDlgRestDayManagement(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgRestDayManagement();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CDlgRestDayManagement};
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListCtrl m_ListCtrl_WorkRestList;

public:
	void ShowAllPersonRestWorkInfo();

	afx_msg void OnAddRequestRestDay();

	CDlgRequstRestDay m_DlgRequstRestDay; // 休息日申请对话框

	afx_msg void OnAdjustRestAndNeeWorkPerson();
	afx_msg void OnAdjustRestAndNeeWorkPerson_EX();
	afx_msg void OnBnClickedOk();
};

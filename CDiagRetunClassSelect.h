#pragma once
#include "afxdialogex.h"
#include "SchedulCommonStruct.h"

// CDiagRetunClassSelect 对话框

class CDiagRetunClassSelect : public CDialogEx
{
	DECLARE_DYNAMIC(CDiagRetunClassSelect)

public:
	CDiagRetunClassSelect(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDiagRetunClassSelect();

	void InitData(vector<ReturnClassStruct> &theReturnClassInfo);

	vector<ReturnClassStruct> m_theReturnClassInfo;
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CDiagRetunClassSelect };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

	int m_iCurrentSelectIndex;
	afx_msg void OnBnClickedButtonLastPlan();
	afx_msg void OnBnClickedButtonNextPlan();

	void ShowReturnClassInfo();

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
};

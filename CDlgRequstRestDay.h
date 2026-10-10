#pragma once
#include "afxdialogex.h"


// CDlgRequstRestDay 对话框

class CDlgRequstRestDay : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgRequstRestDay)

public:
	CDlgRequstRestDay(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgRequstRestDay();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CDlgRequstRestDay };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CButton m_radioButton1;
	
	
	CButton m_CheckButton1;
	CButton m_CheckButton2;
	CButton m_CheckButton3;
	CButton m_CheckButton4;
	CButton m_CheckButton5;
	CButton m_CheckButton6;
	CButton m_CheckButton7;
	CButton m_CheckButton8;
	CButton m_CheckButton9;
	CButton m_CheckButton10;
	CButton m_CheckButton11;
	CButton m_CheckButton12;
	CButton m_CheckButton13;
	CButton m_CheckButton14;
	CButton m_CheckButton15;
	CButton m_CheckButton16;
	CButton m_CheckButton17;
	CButton m_CheckButton18;
	CButton m_CheckButton19;
	CButton m_CheckButton20;
	CButton m_CheckButton21;
	CButton m_CheckButton22;
	CButton m_CheckButton23;
	CButton m_CheckButton24;
	CButton m_CheckButton25;
	CButton m_CheckButton26;

	CButton* m_pButton_CanRestDay[20]; //可以休息的日期按钮数组

	CButton* m_pButton_HasNoWork[20]; //不工作的日期按钮数组

	CComboBox m_ComboBox_AllPerson;
	virtual BOOL OnInitDialog();

	afx_msg void OnCbnSelchangeComboOld();
	CButton m_CheckButton27;
	CButton m_CheckButton28;
	afx_msg void OnBnClickedOk();
};

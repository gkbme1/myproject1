#pragma once
#include "afxdialogex.h"


// TakeLongHoliday 对话框

class TakeLongHoliday : public CDialogEx
{
	DECLARE_DYNAMIC(TakeLongHoliday)

public:
	TakeLongHoliday(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~TakeLongHoliday();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TakeLongHoliday };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();

	CTime m_starttime;
	CTime m_endtime;

	virtual BOOL OnInitDialog();
	CComboBox m_ComboBox_AllPerson;
};

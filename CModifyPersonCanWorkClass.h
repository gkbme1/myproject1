#pragma once
#include "afxdialogex.h"

//班次权限设置界面上，每个级别实际摆了多少个复选框。
//对话框里每级只有30个(IDC_CHECK_CLASS1~30 / 31~60 / 61~90 / 91~120)，
//界面显示、写CSV、读CSV三处必须统一按这个数来：
//写成 OneDayClasssNum-1(=31) 会多取一个控件ID —— 级别1多取到 IDC_CHECK57，
//级别2多取到 IDC_CHECK_CLASS61(那是级别3的第一个框)，把下一个级别的第一个
//班次权限当成本级别的 QA 权限写进数组；同时还会出现"写31列、读30列"的错位。
#define CanWorkClassCheckNum 30

// CModifyPersonCanWorkClass 对话框

class CModifyPersonCanWorkClass : public CDialogEx
{
	DECLARE_DYNAMIC(CModifyPersonCanWorkClass)

public:
	CModifyPersonCanWorkClass(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CModifyPersonCanWorkClass();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CModifyPersonCanWorkClass };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
};

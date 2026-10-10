#pragma once
#include "afxdialogex.h"
#include <string>
using namespace std;
#include "ISAuthorizeDLL_DrugTrace.h"


// CActiveSoftwareDlg 对话框

class CActiveSoftwareDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CActiveSoftwareDlg)

public:
	CActiveSoftwareDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CActiveSoftwareDlg();

	CString strMachineCode;
	

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CActiveSoftwareDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CISAuthorizeDLL_DRUGTRACE* m_pAuthorizeDLL;

	afx_msg void OnBnClickedOk();
};

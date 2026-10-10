#pragma once
#include "afxdialogex.h"
#include "GridCtrl_src/GridCtrl.h"
#include "SchedulCommonStruct.h"

// CPersonRestDlg 对话框

class CPersonRestDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPersonRestDlg)

public:
	CPersonRestDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CPersonRestDlg();




// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CPersonRestDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CGridCtrl m_pGrid_Person;

	afx_msg void OnGridEndEdit(NMHDR* pNotifyStruct, LRESULT* pResult);
	void ReadPersonRestFromCSV(); 
	void UpDateLockInfoToPersonList(CString strLockInfo, CString strPersonName);

	void PutPersonToList(vector<PersonBasicInfo>& theLevel_PersonList, int& iRowCount);
	afx_msg void OnBnClickedOk();
};

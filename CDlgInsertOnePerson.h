#pragma once


// CDlgInsertOnePerson 对话框

class CDlgInsertOnePerson : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgInsertOnePerson)

public:
	CDlgInsertOnePerson(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgInsertOnePerson();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CDlgInsertOnePerson };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	int m_iCurrentLevel;
	int m_iSelectLevel;
	CComboBox m_comLevels;
	CString m_strPersonName;

	bool m_bIsFreeze; //是否冻结

};

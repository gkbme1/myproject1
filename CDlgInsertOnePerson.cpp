// CDlgInsertOnePerson.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "CDlgInsertOnePerson.h"
#include "afxdialogex.h"


// CDlgInsertOnePerson 对话框

IMPLEMENT_DYNAMIC(CDlgInsertOnePerson, CDialogEx)

CDlgInsertOnePerson::CDlgInsertOnePerson(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDlgInsertOnePerson, pParent)
{
	m_iCurrentLevel = -1;
	m_iSelectLevel = -1;
	m_bIsFreeze = true;
}

CDlgInsertOnePerson::~CDlgInsertOnePerson()
{
}

void CDlgInsertOnePerson::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_comLevels);
}


BEGIN_MESSAGE_MAP(CDlgInsertOnePerson, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDlgInsertOnePerson::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgInsertOnePerson 消息处理程序


void CDlgInsertOnePerson::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItemText(IDC_EDIT1, m_strPersonName);
	m_iSelectLevel = m_comLevels.GetCurSel()+1;

	if (((CButton*)GetDlgItem(IDC_CHECK_FREEZE))->GetCheck())
	{
		m_bIsFreeze = true;
	}
	else
	{
		m_bIsFreeze = false;
	}
	CDialogEx::OnOK();
}


BOOL CDlgInsertOnePerson::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化


	((CButton*)GetDlgItem(IDC_CHECK_FREEZE))->SetCheck(1);
	m_comLevels.AddString(_T("级别1"));
	m_comLevels.AddString(_T("级别2"));
	m_comLevels.AddString(_T("级别3"));
	m_comLevels.SetCurSel(m_iCurrentLevel);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}

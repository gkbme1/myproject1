// CRuleManageDlg.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CRuleManageDlg.h"
#include "SchedulCommonStruct.h"
#include <string>
using namespace std;

// CRuleManageDlg 对话框

extern int g_rule_work_5_day;
extern int g_Night_MorningSwitch;
extern int g_OneRound_ClassNum;
extern int g_bAutoSave;

extern string g_filePath;
extern void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength);
extern ModifyClassSettingType g_modifyClassSettingType;

IMPLEMENT_DYNAMIC(CRuleManageDlg, CDialogEx)

CRuleManageDlg::CRuleManageDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CRuleManageDlg, pParent)
{

}

CRuleManageDlg::~CRuleManageDlg()
{
}

void CRuleManageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK1_WORK_5_DAY, m_work_5_day);
	DDX_Control(pDX, IDC_CHECK1_NIGHT_MORNING, m_Night_MorningSwitch);
	DDX_Control(pDX, IDC_CHECK2_ONEROUND_CLASSNUM, m_OneRound_ClassNum);
}


BEGIN_MESSAGE_MAP(CRuleManageDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRuleManageDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CRuleManageDlg 消息处理程序

BOOL CRuleManageDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);


	if (g_rule_work_5_day)
	{
		((CButton*)GetDlgItem(IDC_CHECK1_WORK_5_DAY))->SetCheck(1);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK1_WORK_5_DAY))->SetCheck(0);
	}
	if (g_Night_MorningSwitch)
	{
		((CButton*)GetDlgItem(IDC_CHECK1_NIGHT_MORNING))->SetCheck(1);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK1_NIGHT_MORNING))->SetCheck(0);
	}

	if (g_OneRound_ClassNum)
	{
		((CButton*)GetDlgItem(IDC_CHECK2_ONEROUND_CLASSNUM))->SetCheck(1);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK2_ONEROUND_CLASSNUM))->SetCheck(0);
	}

	if (g_bAutoSave)
	{
		((CButton*)GetDlgItem(IDC_CHECK2_AutoSave))->SetCheck(1);
	}
	else
	{
		((CButton*)GetDlgItem(IDC_CHECK2_AutoSave))->SetCheck(0);
	}

	if (g_modifyClassSettingType == NoSaveToNextTemplate)
	{
		((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(1);
		((CButton*)GetDlgItem(IDC_RADIO2))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_RADIO3))->SetCheck(0);
	}
	else if (g_modifyClassSettingType == SaveToNextTemplate)
	{
		((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_RADIO2))->SetCheck(1);
		((CButton*)GetDlgItem(IDC_RADIO3))->SetCheck(0);
	}
	else if (g_modifyClassSettingType == UserDefineTemplate)
	{
		((CButton*)GetDlgItem(IDC_RADIO1))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_RADIO2))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_RADIO3))->SetCheck(1);
	}




	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CRuleManageDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	if (((CButton*)GetDlgItem(IDC_CHECK1_WORK_5_DAY))->GetCheck())
	{
		g_rule_work_5_day = 1;
	}
	else
	{
		g_rule_work_5_day = 0;
	}

	if (((CButton*)GetDlgItem(IDC_CHECK1_NIGHT_MORNING))->GetCheck())
	{
		g_Night_MorningSwitch = 1;
	}
	else
	{
		g_Night_MorningSwitch = 0;
	}

	if (((CButton*)GetDlgItem(IDC_CHECK2_ONEROUND_CLASSNUM))->GetCheck())
	{
		g_OneRound_ClassNum = 1;
	}
	else
	{
		g_OneRound_ClassNum = 0;
	}

	CString strModifyClassSetting;
	if (((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck())
	{
		g_modifyClassSettingType = NoSaveToNextTemplate;
		strModifyClassSetting = _T("0");
	}
	else if (((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck())
	{
		g_modifyClassSettingType = SaveToNextTemplate;
		strModifyClassSetting = _T("1");
	}
	else if (((CButton*)GetDlgItem(IDC_RADIO3))->GetCheck())
	{
		g_modifyClassSettingType = UserDefineTemplate;
		strModifyClassSetting = _T("2");

	}


	if (((CButton*)GetDlgItem(IDC_CHECK2_AutoSave))->GetCheck())
	{
		g_bAutoSave = 1;
	}
	else
	{
		g_bAutoSave = 0;
	}


	TCHAR strFileName[100] = { 0 };
	STRING2TCHAR(g_filePath, strFileName, 100);

	CString strFilePath_temp = strFileName;
	strFilePath_temp += _T("\\RuleConfig.ini");

	WritePrivateProfileString(_T("RuleConfig"), _T("Work_5_Day"), g_rule_work_5_day ? _T("1") : _T("0"), strFilePath_temp);
	WritePrivateProfileString(_T("RuleConfig"), _T("Night_MorningSwitch"), g_Night_MorningSwitch ? _T("1") : _T("0"), strFilePath_temp);
	WritePrivateProfileString(_T("RuleConfig"), _T("OneRound_ClassNum"), g_OneRound_ClassNum ? _T("1") : _T("0"), strFilePath_temp);
	
	WritePrivateProfileString(_T("RuleConfig"), _T("AutoSave"), g_bAutoSave ? _T("1") : _T("0"), strFilePath_temp);
	WritePrivateProfileString(_T("RuleConfig"), _T("ModifyClassSetting"), strModifyClassSetting, strFilePath_temp);

	CDialogEx::OnOK();
}

// CDlgRequstRestDay.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CDlgRequstRestDay.h"
#include "SchedulCommonStruct.h"
#include "PIVAS_ScheduleDlg.h"


extern CPIVASScheduleDlg* g_pMainDlg;

//人员列表
extern vector<PersonBasicInfo> g_Level1_PersonList;
extern vector<PersonBasicInfo> g_Level2_PersonList;
extern vector<PersonBasicInfo> g_Level3_PersonList;
extern vector<PersonBasicInfo> g_Level4_PersonList;



extern int main_table[quantity][MaxPersonNum][grade];					//储存quantity张排班表

extern void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength);

extern char ClassName[OneDayClasssNum][50];

// CDlgRequstRestDay 对话框

IMPLEMENT_DYNAMIC(CDlgRequstRestDay, CDialogEx)

CDlgRequstRestDay::CDlgRequstRestDay(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDlgRequstRestDay, pParent)
{




}

CDlgRequstRestDay::~CDlgRequstRestDay()
{
}

void CDlgRequstRestDay::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO1, m_radioButton1);
	DDX_Control(pDX, IDC_CHECK2, m_CheckButton1);
	DDX_Control(pDX, IDC_CHECK3, m_CheckButton2);
	DDX_Control(pDX, IDC_CHECK4, m_CheckButton3);
	DDX_Control(pDX, IDC_CHECK5, m_CheckButton4);
	DDX_Control(pDX, IDC_CHECK6, m_CheckButton5);
	DDX_Control(pDX, IDC_CHECK7, m_CheckButton6);
	DDX_Control(pDX, IDC_CHECK8, m_CheckButton7);
	DDX_Control(pDX, IDC_CHECK9, m_CheckButton8);
	DDX_Control(pDX, IDC_CHECK10, m_CheckButton9);
	DDX_Control(pDX, IDC_CHECK11, m_CheckButton10);
	DDX_Control(pDX, IDC_CHECK12, m_CheckButton11);
	DDX_Control(pDX, IDC_CHECK13, m_CheckButton12);
	DDX_Control(pDX, IDC_CHECK14, m_CheckButton13);
	DDX_Control(pDX, IDC_CHECK15, m_CheckButton14);
	DDX_Control(pDX, IDC_CHECK16, m_CheckButton15);
	DDX_Control(pDX, IDC_CHECK17, m_CheckButton16);
	DDX_Control(pDX, IDC_CHECK18, m_CheckButton17);
	DDX_Control(pDX, IDC_CHECK19, m_CheckButton18);
	DDX_Control(pDX, IDC_CHECK20, m_CheckButton19);
	DDX_Control(pDX, IDC_CHECK21, m_CheckButton20);
	DDX_Control(pDX, IDC_CHECK22, m_CheckButton21);
	DDX_Control(pDX, IDC_CHECK23, m_CheckButton22);
	DDX_Control(pDX, IDC_CHECK24, m_CheckButton23);
	DDX_Control(pDX, IDC_CHECK25, m_CheckButton24);
	DDX_Control(pDX, IDC_CHECK26, m_CheckButton25);
	DDX_Control(pDX, IDC_CHECK27, m_CheckButton26);
	DDX_Control(pDX, IDC_COMBO1, m_ComboBox_AllPerson);
	DDX_Control(pDX, IDC_CHECK28, m_CheckButton27);
	DDX_Control(pDX, IDC_CHECK29, m_CheckButton28);
}


BEGIN_MESSAGE_MAP(CDlgRequstRestDay, CDialogEx)

ON_CBN_SELCHANGE(IDC_COMBO1, &CDlgRequstRestDay::OnCbnSelchangeComboOld)

ON_BN_CLICKED(IDOK, &CDlgRequstRestDay::OnBnClickedOk)
END_MESSAGE_MAP()


// CDlgRequstRestDay 消息处理程序

BOOL CDlgRequstRestDay::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
	m_radioButton1.SetCheck(BST_CHECKED); // 默认选中第一个单选按钮

	// TODO:  在此添加额外的初始化
	m_pButton_CanRestDay[0] = &m_CheckButton1;
	m_pButton_CanRestDay[1] = &m_CheckButton2;
	m_pButton_CanRestDay[2] = &m_CheckButton3;
	m_pButton_CanRestDay[3] = &m_CheckButton4;
	m_pButton_CanRestDay[4] = &m_CheckButton5;
	m_pButton_CanRestDay[5] = &m_CheckButton6;
	m_pButton_CanRestDay[6] = &m_CheckButton7;
	m_pButton_CanRestDay[7] = &m_CheckButton8;
	m_pButton_CanRestDay[8] = &m_CheckButton9;
	m_pButton_CanRestDay[9] = &m_CheckButton10;
	m_pButton_CanRestDay[10] = &m_CheckButton11;
	m_pButton_CanRestDay[11] = &m_CheckButton12;
	m_pButton_CanRestDay[12] = &m_CheckButton13;
	m_pButton_CanRestDay[13] = &m_CheckButton14;

	m_pButton_HasNoWork[0] = &m_CheckButton15;
	m_pButton_HasNoWork[1] = &m_CheckButton16;
	m_pButton_HasNoWork[2] = &m_CheckButton17;
	m_pButton_HasNoWork[3] = &m_CheckButton18;
	m_pButton_HasNoWork[4] = &m_CheckButton19;
	m_pButton_HasNoWork[5] = &m_CheckButton20;
	m_pButton_HasNoWork[6] = &m_CheckButton21;
	m_pButton_HasNoWork[7] = &m_CheckButton22;
	m_pButton_HasNoWork[8] = &m_CheckButton23;
	m_pButton_HasNoWork[9] = &m_CheckButton24;
	m_pButton_HasNoWork[10] = &m_CheckButton25;
	m_pButton_HasNoWork[11] = &m_CheckButton26;
	m_pButton_HasNoWork[12] = &m_CheckButton27;
	m_pButton_HasNoWork[13] = &m_CheckButton28;




	// 初始化所有人下拉框

	int iPersonNum = g_Level1_PersonList.size();
	for (int i = 0; i < iPersonNum; i++)
	{
		m_ComboBox_AllPerson.AddString(g_Level1_PersonList[i].strPersonName.c_str());
	}
	iPersonNum = g_Level2_PersonList.size();
	for (int i = 0; i < iPersonNum; i++)
	{
		m_ComboBox_AllPerson.AddString(g_Level2_PersonList[i].strPersonName.c_str());
	}

	iPersonNum = g_Level3_PersonList.size();
	for (int i = 0; i < iPersonNum; i++)
	{
		m_ComboBox_AllPerson.AddString(g_Level3_PersonList[i].strPersonName.c_str());
	}
	iPersonNum = g_Level4_PersonList.size();
	for (int i = 0; i < iPersonNum; i++)
	{
		m_ComboBox_AllPerson.AddString(g_Level4_PersonList[i].strPersonName.c_str());
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CDlgRequstRestDay::OnCbnSelchangeComboOld()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取选择的人员姓名

	//UpdateData(FALSE);
	CString strPersonName;

	int nIndex = m_ComboBox_AllPerson.GetCurSel(); // 获取当前选中项的索引
	if (nIndex != CB_ERR) // 检查是否成功获取索引
	{
		m_ComboBox_AllPerson.GetLBText(nIndex, strPersonName); // 获取选中项的文本
		//AfxMessageBox(strText); // 显示文本
	}



	//获取人员序号
	int iPersonIndex = g_pMainDlg->FindPesrsonIndexByName(strPersonName);

	if(iPersonIndex < 0)
	{
		AfxMessageBox(_T("未找到该人员，请检查人员列表！"));
		return;
	}
	//根据人员姓名设置对应的休息日复选框状态
	int iClassIndex_Work = -1;
	for (int iDayInex = 0; iDayInex < AllDay; iDayInex++)
	{
		iClassIndex_Work = -1;
		for (int iClassIndex = iDayInex * OneDayClasssNum; iClassIndex < (iDayInex + 1) * OneDayClasssNum; iClassIndex++)
		{
			int iClassValue = main_table[0][iPersonIndex][iClassIndex];
			if (iClassValue > 0)
			{
				//当天的班次
				iClassIndex_Work = iClassIndex% OneDayClasssNum;
			}

		}

		//根据班次判断是否允许调休
		if (iClassIndex_Work < 0)
		{
			//当天休息
			m_pButton_CanRestDay[iDayInex]->SetCheck(BST_UNCHECKED); //当天休息不能调休
			m_pButton_CanRestDay[iDayInex]->EnableWindow(FALSE); //当天休息不能调休

			m_pButton_HasNoWork[iDayInex]->SetCheck(BST_UNCHECKED); //当天休息
			m_pButton_HasNoWork[iDayInex]->EnableWindow(TRUE); 

			CString strContent;
			strContent.Format(_T("第%d天(休息)"), iDayInex + 1);
			m_pButton_CanRestDay[iDayInex]->SetWindowText(strContent);
			m_pButton_HasNoWork[iDayInex]->SetWindowText(strContent);

		}
		else 
		{
			bool bCanRest = g_pMainDlg->bJudgeCanRestByClassIndex(iClassIndex_Work); //是否可以调休
			
			if (bCanRest)
			{
				m_pButton_CanRestDay[iDayInex]->SetCheck(BST_UNCHECKED); //当天休息不能调休
				m_pButton_CanRestDay[iDayInex]->EnableWindow(TRUE); //当天休息不能调休


				m_pButton_HasNoWork[iDayInex]->SetCheck(BST_UNCHECKED);
				m_pButton_HasNoWork[iDayInex]->EnableWindow(FALSE);


				CString strContent;
				TCHAR strClassName[100] = { 0 };
				STRING2TCHAR(ClassName[iClassIndex_Work], strClassName, 100);
				strContent.Format(_T("第%d天(%s)可调休"), iDayInex + 1, strClassName);

				m_pButton_CanRestDay[iDayInex]->SetWindowText(strContent);

				strContent.Format(_T("第%d天(%s)"), iDayInex + 1, strClassName);
				m_pButton_HasNoWork[iDayInex]->SetWindowText(strContent);
			}
			else 
			{
				m_pButton_CanRestDay[iDayInex]->SetCheck(BST_UNCHECKED); //当天休息不能调休
				m_pButton_CanRestDay[iDayInex]->EnableWindow(FALSE); //当天休息不能调休


				m_pButton_HasNoWork[iDayInex]->SetCheck(BST_UNCHECKED);
				m_pButton_HasNoWork[iDayInex]->EnableWindow(FALSE);


				CString strContent;
				TCHAR strClassName[100] = { 0 };
				STRING2TCHAR(ClassName[iClassIndex_Work], strClassName, 100);
				strContent.Format(_T("第%d天(%s)不可调休"), iDayInex + 1, strClassName);

				m_pButton_CanRestDay[iDayInex]->SetWindowText(strContent);


				strContent.Format(_T("第%d天(%s)"), iDayInex + 1, strClassName);
				m_pButton_HasNoWork[iDayInex]->SetWindowText(strContent);
			}

		}

	}



}
void CDlgRequstRestDay::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//将调休信息存到g_vectorPersonRestInfo中
	bool bCeck1 = ((CButton*)GetDlgItem(IDC_RADIO1))->GetCheck();
	bool bCeck2 = ((CButton*)GetDlgItem(IDC_RADIO2))->GetCheck();
	bool bCeck3 = ((CButton*)GetDlgItem(IDC_RADIO3))->GetCheck();

	for(int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		bool bCanRest = m_pButton_CanRestDay[iDayIndex]->GetCheck() == BST_CHECKED;
		bool bHasNoWork = m_pButton_HasNoWork[iDayIndex]->GetCheck() == BST_CHECKED;
		if (bCanRest || bHasNoWork)
		{
			CString strPersonName;
			int nIndex = m_ComboBox_AllPerson.GetCurSel(); // 获取当前选中项的索引
			if (nIndex != CB_ERR) // 检查是否成功获取索引
			{
				m_ComboBox_AllPerson.GetLBText(nIndex, strPersonName); // 获取选中项的文本
			}
			int iPersonIndex = g_pMainDlg->FindPesrsonIndexByName(strPersonName);
			if (iPersonIndex < 0)
			{
				AfxMessageBox(_T("未找到该人员，请检查人员列表！"));
				return;
			}
			CString strRestType;
			RestEnumType iSureRestDay = NO_REST;
			if (bCeck1)
			{
				strRestType = RestOrWorkTye3; //这轮调休这轮补
			}
			else if (bCeck2)
			{
				strRestType = RestOrWorkTye4; //这轮调休下轮补班
			}
			else if (bCeck3)
			{
				strRestType = RestOrWorkTye5; //这轮补班下轮调休

			}

			if (bHasNoWork)
			{
				iSureRestDay = HasRest_NoAdjust;
			}
			g_pMainDlg->PutPersonLastRestInfoToVect(strPersonName, strRestType,1, iSureRestDay, iDayIndex);
		}
	}


	CDialogEx::OnOK();
}

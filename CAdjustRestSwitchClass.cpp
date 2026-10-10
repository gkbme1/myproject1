// CAdjustRestSwitchClass.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CAdjustRestSwitchClass.h"
#include "PIVAS_ScheduleDlg.h"



extern vector<PersonBasicInfo> g_AllLevel_PersonList;
extern char ClassName[OneDayClasssNum][50];
extern void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength);
extern int g_bAutoSave;

extern CPIVASScheduleDlg* g_pMainDlg;

extern vector<ReturnClassPersonAndDay> g_vectorReturnClassPersonAndDay;
extern int main_table[quantity][MaxPersonNum][grade];
extern time_t g_tStartTime;

//调休安排成功之后，让客户顺手给这个格子填一个自定义班次/说明(实现在 PIVAS_ScheduleDlg.cpp)
extern void AskFillRelaxClassAfterArrange(CPIVASScheduleDlg* pMainDlg, int iPersonIndex, int iDayIndex);

// CAdjustRestSwitchClass 对话框

//运行时自己建的两个控件用的命令号。
//资源文件里没有这两个控件，号段避开系统保留的和资源里已用的就行。
#define IDC_ADJUST_PLAN_LIST		50010
#define IDC_ADJUST_TOGGLE_DETAIL	50011

//方案的规则提示文字：硬性违规和"只提示不拦截"的软规则(A/T 数量)说法不一样，
//不能一律写成"违反规则"，否则客户会以为这个方案不能用。
static CString MakeRuleDescText(RestAdjustPlan& thePlan)
{
	CString strText;
	if (thePlan.bIsRuleValid)
	{
		strText.Format(_T("提示(不影响执行)：%s"), thePlan.strRuleErrorDesc);
	}
	else
	{
		strText.Format(_T("该方案违反规则：%s"), thePlan.strRuleErrorDesc);
	}
	return strText;
}

IMPLEMENT_DYNAMIC(CAdjustRestSwitchClass, CDialogEx)

CAdjustRestSwitchClass::CAdjustRestSwitchClass(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CAdjustRestSwitchClass, pParent)
{
	m_iCurrentSelectIndex = 0;
	m_bShowDetail = false;		//默认只看"接班候选人 + 方案数"，要看链路再展开

}
void CAdjustRestSwitchClass::ShowReturnClassInfo()
{
	g_vectorReturnClassPersonAndDay.clear();




	CString strIndex;
	strIndex.Format(_T("当前方案：%d / %d"), m_iCurrentSelectIndex + 1, m_theAllPlanClassInfo.size());
	SetDlgItemText(IDC_STATIC_0, strIndex);

	//每次换方案先把违规提示清空，否则翻到合规方案时上一条的红字还赖在界面上
	SetDlgItemText(IDC_STATIC_5, _T(""));



	RestAdjustPlan theAdjustPlanClassInfo = m_theAllPlanClassInfo[m_iCurrentSelectIndex];

	if (theAdjustPlanClassInfo.iPlanType == 2)
	{
		CString strSwitchType;
		strSwitchType.Format(_T("更换类型：两人轮换"));
		SetDlgItemText(IDC_STATIC_4, strSwitchType);
	}
	else if (theAdjustPlanClassInfo.iPlanType == ChainPlanType)
	{
		CString strSwitchType;
		if (theAdjustPlanClassInfo.bEndIsUniversalCell)
		{
			strSwitchType.Format(_T("更换类型：%d人联动轮换(班次给到万能格)"), (int)theAdjustPlanClassInfo.vecChainPersons.size());
		}
		else
		{
			strSwitchType.Format(_T("更换类型：%d人联动轮换"), (int)theAdjustPlanClassInfo.vecChainPersons.size());
		}
		SetDlgItemText(IDC_STATIC_4, strSwitchType);
	}
	else
	{
		CString strSwitchType;
		strSwitchType.Format(_T("更换类型：三人轮换"));
		SetDlgItemText(IDC_STATIC_4, strSwitchType);
	}

	CTime cThisDay = CTime(g_tStartTime + theAdjustPlanClassInfo.iDayIndex_Source * 24 * 60 * 60);

	CString strDateInfo;
	strDateInfo.Format(_T("%d月-%02d号"), cThisDay.GetMonth(), cThisDay.GetDay());


	CString strData1;
	strData1.Format(_T("第%d天"), theAdjustPlanClassInfo.iDayIndex_Source + 1);
	strData1 += strDateInfo;


	CString strData2;

	CString strPerson_Source = g_AllLevel_PersonList[theAdjustPlanClassInfo.iPerson_Source].strPersonName.c_str();
	CString strPerson_Dst = g_AllLevel_PersonList[theAdjustPlanClassInfo.iPersonIndex_B].strPersonName.c_str();

	CString strPerson_C;
	if (theAdjustPlanClassInfo.iPersonIndex_C > 0)
	{
		strPerson_C = g_AllLevel_PersonList[theAdjustPlanClassInfo.iPersonIndex_C].strPersonName.c_str();
	}

	if (theAdjustPlanClassInfo.iPlanType == 2)
	{
		strData2.Format(_T("%s的%s班次与%s的%s班次更换"), strPerson_Source, theAdjustPlanClassInfo.strClassNameSrc, strPerson_Dst, theAdjustPlanClassInfo.strClassName_B);

		if (!theAdjustPlanClassInfo.strRuleErrorDesc.IsEmpty())
		{
			SetDlgItemText(IDC_STATIC_5, MakeRuleDescText(theAdjustPlanClassInfo));
		}

	}
	else if (theAdjustPlanClassInfo.iPlanType == ChainPlanType)
	{
		//N人链式联动：逐级列出每一步谁接谁的班
		int iChainNum = (int)theAdjustPlanClassInfo.vecChainPersons.size();
		for (int i = 1; i < iChainNum; i++)
		{
			CString strOneStep;
			strOneStep.Format(_T("%s 接 %s 的 %s 班"),
				g_AllLevel_PersonList[theAdjustPlanClassInfo.vecChainPersons[i]].strPersonName.c_str(),
				g_AllLevel_PersonList[theAdjustPlanClassInfo.vecChainPersons[i - 1]].strPersonName.c_str(),
				theAdjustPlanClassInfo.vecChainClassName[i - 1]);

			if (i > 1)
			{
				strData2 += _T("； ");
			}
			strData2 += strOneStep;
		}

		CString strEnd;
		strEnd.Format(_T("；最终 %s 当天调休"), strPerson_Source);
		strData2 += strEnd;

		if (!theAdjustPlanClassInfo.strRuleErrorDesc.IsEmpty())
		{
			SetDlgItemText(IDC_STATIC_5, MakeRuleDescText(theAdjustPlanClassInfo));
		}
	}
	else
	{
		strData2.Format(_T("%s的%s班次 与 %s的%s班次、%s的%s班次更换"), strPerson_Source, theAdjustPlanClassInfo.strClassNameSrc,
			strPerson_Dst, theAdjustPlanClassInfo.strClassName_B, strPerson_C, theAdjustPlanClassInfo.strClassName_C);

		//三人轮换原来根本不显示规则提示，A/T 超标这种软规则客户根本看不到
		if (!theAdjustPlanClassInfo.strRuleErrorDesc.IsEmpty())
		{
			SetDlgItemText(IDC_STATIC_5, MakeRuleDescText(theAdjustPlanClassInfo));
		}
	}

	//默认只给客户看一句话：谁接班、一共几个方案。
	//链路详情（谁接谁的班）放在下面，点"展开详情"才显示，
	//否则一上来就是一大段底层逻辑，客户反而不知道该选谁。
	CString strData3;
	strData3.Format(_T("%s（第 %d / 共 %d 个方案）"),
		MakePlanBriefText(theAdjustPlanClassInfo), m_iCurrentSelectIndex + 1, (int)m_theAllPlanClassInfo.size());

	CString strData4;




	SetDlgItemText(IDC_STATIC_1, strData1);
	SetDlgItemText(IDC_STATIC_3, strData3);		//摘要行，总是显示
	SetDlgItemText(IDC_STATIC_2, m_bShowDetail ? strData2 : CString(_T("")));	//链路详情，展开才显示
	//SetDlgItemText(IDC_STATIC_4, strData4);

	//下拉框的选中项跟着当前方案走
	if (m_comboPlanList.GetSafeHwnd() != NULL)
	{
		if (m_comboPlanList.GetCurSel() != m_iCurrentSelectIndex)
		{
			m_comboPlanList.SetCurSel(m_iCurrentSelectIndex);
		}
	}



	if (theAdjustPlanClassInfo.iPlanType == ChainPlanType)
	{
		//链式联动：链条上每一个人当天的格子都要高亮出来，方便一眼看清联动范围
		for (int i = 0; i < (int)theAdjustPlanClassInfo.vecChainPersons.size(); i++)
		{
			ReturnClassPersonAndDay theOneChainPerson;
			theOneChainPerson.iPersonIndex = theAdjustPlanClassInfo.vecChainPersons[i];
			theOneChainPerson.iDayIndex = theAdjustPlanClassInfo.iDayIndex_Source;
			g_vectorReturnClassPersonAndDay.push_back(theOneChainPerson);
		}
	}
	else
	{
		ReturnClassPersonAndDay theReturnClassPersonAndDay1;
		theReturnClassPersonAndDay1.iPersonIndex = theAdjustPlanClassInfo.iPerson_Source;
		theReturnClassPersonAndDay1.iDayIndex = theAdjustPlanClassInfo.iDayIndex_Source;
		ReturnClassPersonAndDay theReturnClassPersonAndDay2;
		theReturnClassPersonAndDay2.iPersonIndex = theAdjustPlanClassInfo.iPersonIndex_B;
		theReturnClassPersonAndDay2.iDayIndex = theAdjustPlanClassInfo.iDayIndex_Source;
		g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay1);
		g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay2);


		if (theAdjustPlanClassInfo.iPersonIndex_C > 0)
		{
			ReturnClassPersonAndDay theReturnClassPersonAndDay3;
			theReturnClassPersonAndDay3.iPersonIndex = theAdjustPlanClassInfo.iPersonIndex_C;
			theReturnClassPersonAndDay3.iDayIndex = theAdjustPlanClassInfo.iDayIndex_Source;
			g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay3);
		}
	}
;


	//g_pMainDlg->ShowAllPersonToList(false, false, true);

	// 【修改此处】：彻底废弃全量刷新和异步消息投递
	// 直接同步调用纯颜色刷新函数。由于该函数内部不包含任何控件重建操作，
	// 即使瞬间高频触发，UI 依然能在 1 毫秒内响应完毕，绝不阻塞。
	if (g_pMainDlg != nullptr)
	{
		g_pMainDlg->UpdateGridColorsOnly();
	}

}

CAdjustRestSwitchClass::~CAdjustRestSwitchClass()
{
}

void CAdjustRestSwitchClass::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAdjustRestSwitchClass, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LAST_PLAN, &CAdjustRestSwitchClass::OnBnClickedButtonLastPlan)
	ON_BN_CLICKED(IDC_BUTTON_NEXT_PLAN, &CAdjustRestSwitchClass::OnBnClickedButtonNextPlan)
	ON_BN_CLICKED(IDOK, &CAdjustRestSwitchClass::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CAdjustRestSwitchClass::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_ADJUST_TOGGLE_DETAIL, &CAdjustRestSwitchClass::OnBnClickedToggleDetail)
	ON_CBN_SELCHANGE(IDC_ADJUST_PLAN_LIST, &CAdjustRestSwitchClass::OnSelchangePlanList)
END_MESSAGE_MAP()


// CAdjustRestSwitchClass 消息处理程序

void CAdjustRestSwitchClass::OnBnClickedButtonLastPlan()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_iCurrentSelectIndex <= 0) return;
	m_iCurrentSelectIndex--;
	ShowReturnClassInfo();
}

void CAdjustRestSwitchClass::OnBnClickedButtonNextPlan()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_iCurrentSelectIndex >= (int)m_theAllPlanClassInfo.size() - 1) return;
	m_iCurrentSelectIndex++;
	ShowReturnClassInfo();
}
void CAdjustRestSwitchClass::InitData(vector<RestAdjustPlan>& outPlans, int iPersonIndex, int iDayIndex, int iClassIndexOneDay, CString itemText, bool bIsHoliDayRst, int HoliDayRlexIndex, int iRow, int iCol)
{
	m_theAllPlanClassInfo = outPlans;
	m_iPersonIndex = iPersonIndex;
	m_iDayIndex = iDayIndex;
	m_iClassIndexOneDay = iClassIndexOneDay;
	m_itemText = itemText;
	m_bIsHoliDayRst = bIsHoliDayRst;
	m_HoliDayRlexIndex = HoliDayRlexIndex;
	m_iRow = iRow;
	m_iCol = iCol;
}

void CAdjustRestSwitchClass::OnBnClickedOk()
{
	if (g_pMainDlg == nullptr)
	{
		DestroyWindow();
		return;
	}

	int selIndex = m_iCurrentSelectIndex;
	bool bHasChangeShedule = false;

	if (selIndex >= 0 && selIndex < m_theAllPlanClassInfo.size())
	{
		RestAdjustPlan selectedPlan = m_theAllPlanClassInfo[selIndex];

		//执行前把规则提示弹一遍：
		//A/T 数量超标这种软规则不拦截，但不能像以前那样一声不响就说换班成功。
		if (!selectedPlan.strRuleErrorDesc.IsEmpty())
		{
			CString strAsk;
			if (selectedPlan.bIsRuleValid)
			{
				strAsk.Format(_T("该方案有以下提示：\n\n%s\n\n是否继续执行？"), selectedPlan.strRuleErrorDesc);
			}
			else
			{
				strAsk.Format(_T("该方案违反了排班规则：\n\n%s\n\n是否仍要执行？"), selectedPlan.strRuleErrorDesc);
			}

			if (MessageBox(strAsk, _T("换班校验提示"), MB_OKCANCEL | MB_ICONWARNING) != IDOK)
			{
				return;		//不关窗，让客户继续翻别的方案
			}
		}

		// 确定调休状态枚举值
		int iClassIndex_relax = Class_ChangeRelax + m_iDayIndex * OneDayClasssNum;
		int iRelaxStatus = (m_itemText == _T("补休")) ? WORK_TO_REST_INIT : WORK_TO_Relax;
		if (m_bIsHoliDayRst)
		{
			iRelaxStatus = m_HoliDayRlexIndex;
		}

		// 执行换班
		if (selectedPlan.iPlanType == 2)
		{
			// 【两人直换】
			int iPerson_B = selectedPlan.iPersonIndex_B;

			main_table[0][iPerson_B][m_iDayIndex * OneDayClasssNum + m_iClassIndexOneDay] = 1;
			main_table[0][m_iPersonIndex][m_iDayIndex * OneDayClasssNum + m_iClassIndexOneDay] = 0;

			main_table[0][m_iPersonIndex][iClassIndex_relax] = iRelaxStatus;

			//接班的如果是万能格，他本来就没有调休名额，接了班就是正常上班，
			//不能标成"调休转上班"，否则他白白多出一个调休名额。
			if (!selectedPlan.bEndIsUniversalCell)
			{
				main_table[0][iPerson_B][iClassIndex_relax] = Relax_TO_WORK;
			}

			g_vectorReturnClassPersonAndDay.clear();
			g_pMainDlg->AddOneModifyToVect(m_iPersonIndex, m_iDayIndex);
			g_pMainDlg->AddOneModifyToVect(iPerson_B, m_iDayIndex);
			bHasChangeShedule = true;
		}
		else if (selectedPlan.iPlanType == 3)
		{
			// 【三人轮换】
			int iPerson_B = selectedPlan.iPersonIndex_B;
			int iPerson_C = selectedPlan.iPersonIndex_C;

			int iClassIndexOneDay_C = g_pMainDlg->GetPersonOneDayWorkClassIndex(iPerson_C, m_iDayIndex);

			// B接手C的班次
			main_table[0][iPerson_B][m_iDayIndex * OneDayClasssNum + iClassIndexOneDay_C] = 1;
			main_table[0][iPerson_C][m_iDayIndex * OneDayClasssNum + iClassIndexOneDay_C] = 0;

			// C接手A的班次
			main_table[0][iPerson_C][m_iDayIndex * OneDayClasssNum + m_iClassIndexOneDay] = 1;
			main_table[0][m_iPersonIndex][m_iDayIndex * OneDayClasssNum + m_iClassIndexOneDay] = 0;

			// 设置调休状态
			main_table[0][m_iPersonIndex][iClassIndex_relax] = iRelaxStatus;
			main_table[0][iPerson_B][iClassIndex_relax] = Relax_TO_WORK;

			g_vectorReturnClassPersonAndDay.clear();
			g_pMainDlg->AddOneModifyToVect(m_iPersonIndex, m_iDayIndex);
			g_pMainDlg->AddOneModifyToVect(iPerson_B, m_iDayIndex);
			g_pMainDlg->AddOneModifyToVect(iPerson_C, m_iDayIndex);
			bHasChangeShedule = true;
		}
		else if (selectedPlan.iPlanType == ChainPlanType)
		{
			// 【N人链式联动】
			// vecChainPersons[0]      : 申请调休的人，换班后当天调休
			// vecChainPersons[i]      : 接手 vecChainPersons[i-1] 原班次的人
			// vecChainPersons[最后一个]: 当天原本调休/补休、现在顶班上岗的人
			vector<int>& vecChainPersons = selectedPlan.vecChainPersons;
			vector<int>& vecChainClassIdx = selectedPlan.vecChainClassIdx;

			int iChainNum = (int)vecChainPersons.size();
			//末端是万能格时，两个人就能闭合（申请人 + 万能格），不需要中间人
			if (iChainNum >= 2 && (int)vecChainClassIdx.size() == iChainNum)
			{
				// 从链条末端往回搬班次：每个人接下上家的班，同时交出自己原来的班
				for (int i = iChainNum - 1; i >= 1; i--)
				{
					int iPerson_Take = vecChainPersons[i];		//接班的人
					int iClass_Take = vecChainClassIdx[i - 1];	//要接的是上家原来的班
					int iClass_Give = vecChainClassIdx[i];		//自己原来的班，-1表示原本就在休息

					if (iClass_Take >= 0)
					{
						main_table[0][iPerson_Take][m_iDayIndex * OneDayClasssNum + iClass_Take] = 1;
					}
					if (iClass_Give >= 0)
					{
						main_table[0][iPerson_Take][m_iDayIndex * OneDayClasssNum + iClass_Give] = 0;
					}

					g_pMainDlg->AddOneModifyToVect(iPerson_Take, m_iDayIndex);
				}

				// 申请人腾出自己的班，当天转为调休(或补休、节假日调休)
				if (vecChainClassIdx[0] >= 0)
				{
					main_table[0][m_iPersonIndex][m_iDayIndex * OneDayClasssNum + vecChainClassIdx[0]] = 0;
				}
				main_table[0][m_iPersonIndex][iClassIndex_relax] = iRelaxStatus;

				// 末端顶班的人：
				// 原本是调休的 -> 调休名额让给了申请人，自己标成"调休转上班"；
				// 原本是万能格 -> 本来就没有调休名额，接了班就是正常上班，标记保持为空。
				if (!selectedPlan.bEndIsUniversalCell)
				{
					main_table[0][vecChainPersons[iChainNum - 1]][iClassIndex_relax] = Relax_TO_WORK;
				}

				g_vectorReturnClassPersonAndDay.clear();
				g_pMainDlg->AddOneModifyToVect(m_iPersonIndex, m_iDayIndex);
				bHasChangeShedule = true;
			}
		}
	}

	// 统一处理撤销栈队列、自动保存和界面刷新
	if (bHasChangeShedule)
	{
		g_pMainDlg->m_pGrid.SetItemText(m_iRow, m_iCol, m_itemText); // 应用界面的文字修改
		g_pMainDlg->AddPlanToCurrentRedoArray();
		if (g_bAutoSave)
		{
			g_pMainDlg->SetTimer(TIMER_AUTO_SAVE, 2000, NULL);
		}
		g_pMainDlg->SetFocus();
		g_pMainDlg->PostMessage(WM_REFRESH_GRID, 0, 0);

		//调休安排成功：格子会刷成蓝底，再让客户顺手填一个自定义班次/说明(补端午、年假之类)。
		//以前安排完统一写死"调休(调整)"，客户要改还得先"删除班次"把格子清空再输入。
		AskFillRelaxClassAfterArrange(g_pMainDlg, m_iPersonIndex, m_iDayIndex);
	}

	DestroyWindow(); // 非模态对话框须使用 DestroyWindow 关闭自身
}



//方案的一句话摘要：谁接这个班、用的是哪种换法。
//客户最关心的就是"这个班最后落到谁头上"。
CString CAdjustRestSwitchClass::MakePlanBriefText(RestAdjustPlan& thePlan)
{
	CString strTakePersonName;
	CString strTypeName;

	if (thePlan.iPlanType == ChainPlanType)
	{
		if (thePlan.vecChainPersons.size() >= 2)
		{
			strTakePersonName = g_AllLevel_PersonList[thePlan.vecChainPersons[1]].strPersonName.c_str();
		}
		if (thePlan.bEndIsUniversalCell)
		{
			strTypeName.Format(_T("%d人联动·末端万能格"), (int)thePlan.vecChainPersons.size());
		}
		else
		{
			strTypeName.Format(_T("%d人联动"), (int)thePlan.vecChainPersons.size());
		}
	}
	else if (thePlan.iPlanType == 2)
	{
		if (thePlan.iPersonIndex_B >= 0 && thePlan.iPersonIndex_B < (int)g_AllLevel_PersonList.size())
		{
			strTakePersonName = g_AllLevel_PersonList[thePlan.iPersonIndex_B].strPersonName.c_str();
		}
		strTypeName = thePlan.bEndIsUniversalCell ? _T("两人换班·万能格") : _T("两人换班");
	}
	else
	{
		if (thePlan.iPersonIndex_C >= 0 && thePlan.iPersonIndex_C < (int)g_AllLevel_PersonList.size())
		{
			strTakePersonName = g_AllLevel_PersonList[thePlan.iPersonIndex_C].strPersonName.c_str();
		}
		strTypeName = _T("三人轮换");
	}

	if (strTakePersonName.IsEmpty())
	{
		strTakePersonName = _T("?");
	}

	CString strBrief;
	strBrief.Format(_T("%s 接班（%s）"), strTakePersonName, strTypeName);
	return strBrief;
}

//把所有方案灌进下拉框，方案多的时候直接跳，不用一个个点"下一个方案"
void CAdjustRestSwitchClass::FillPlanListCombo()
{
	if (m_comboPlanList.GetSafeHwnd() == NULL)
	{
		return;
	}

	m_comboPlanList.ResetContent();
	for (int i = 0; i < (int)m_theAllPlanClassInfo.size(); i++)
	{
		CString strOneLine;
		strOneLine.Format(_T("%d. %s"), i + 1, MakePlanBriefText(m_theAllPlanClassInfo[i]));

		//违规/有提示的方案在列表里就标出来，客户不用一个个点进去看
		if (!m_theAllPlanClassInfo[i].strRuleErrorDesc.IsEmpty())
		{
			strOneLine += m_theAllPlanClassInfo[i].bIsRuleValid ? _T("  [有提示]") : _T("  [违规]");
		}

		m_comboPlanList.AddString(strOneLine);
	}

	if (m_iCurrentSelectIndex >= 0 && m_iCurrentSelectIndex < (int)m_theAllPlanClassInfo.size())
	{
		m_comboPlanList.SetCurSel(m_iCurrentSelectIndex);
	}
}

//展开/收起链路详情
void CAdjustRestSwitchClass::OnBnClickedToggleDetail()
{
	m_bShowDetail = !m_bShowDetail;

	if (m_btnToggleDetail.GetSafeHwnd() != NULL)
	{
		m_btnToggleDetail.SetWindowText(m_bShowDetail ? _T("收起详情") : _T("展开详情"));
	}

	ShowReturnClassInfo();
}

//从下拉框里直接跳到某个方案
void CAdjustRestSwitchClass::OnSelchangePlanList()
{
	if (m_comboPlanList.GetSafeHwnd() == NULL)
	{
		return;
	}

	int iSel = m_comboPlanList.GetCurSel();
	if (iSel == CB_ERR || iSel < 0 || iSel >= (int)m_theAllPlanClassInfo.size())
	{
		return;
	}

	m_iCurrentSelectIndex = iSel;
	ShowReturnClassInfo();
}

//资源里 IDC_STATIC_2 只有一行高、宽 276 个对话框单位，
//4 人联动的方案描述根本放不下，尾巴直接看不见。
//这里在运行时把窗口撑大、把文字控件改成多行大区域，
//静态文本默认就会自动按词换行，不用改 .rc 资源文件。
void CAdjustRestSwitchClass::ResizeForLongText()
{
	CWnd* pText2 = GetDlgItem(IDC_STATIC_2);		//方案详情
	CWnd* pText3 = GetDlgItem(IDC_STATIC_3);		//备用说明行
	CWnd* pText5 = GetDlgItem(IDC_STATIC_5);		//违规/提示
	CWnd* pBtnOk = GetDlgItem(IDOK);
	CWnd* pBtnCancel = GetDlgItem(IDCANCEL);

	if (pText2 == NULL)
	{
		return;
	}

	CRect rcClient;
	GetClientRect(&rcClient);
	int iOldWidth = rcClient.Width();

	//窗口加宽约 60%、加高 180 像素
	int iAddWidth = iOldWidth * 6 / 10;
	int iAddHeight = 180;

	CRect rcWindow;
	GetWindowRect(&rcWindow);
	SetWindowPos(NULL, 0, 0, rcWindow.Width() + iAddWidth, rcWindow.Height() + iAddHeight,
		SWP_NOMOVE | SWP_NOZORDER);
	CenterWindow();

	GetClientRect(&rcClient);
	int iNewWidth = rcClient.Width();
	int iNewHeight = rcClient.Height();

	CRect rcText2;
	pText2->GetWindowRect(&rcText2);
	ScreenToClient(&rcText2);

	int iLeft = rcText2.left;
	int iTop = rcText2.top;
	int iTextWidth = iNewWidth - iLeft - 12;
	if (iTextWidth < 100)
	{
		iTextWidth = 100;
	}

	const int iHeight2 = 110;		//方案详情，够放四五行
	const int iHeight3 = 40;
	const int iHeight5 = 60;

	pText2->MoveWindow(iLeft, iTop, iTextWidth, iHeight2);
	if (pText3 != NULL)
	{
		pText3->MoveWindow(iLeft, iTop + iHeight2 + 6, iTextWidth, iHeight3);
	}
	if (pText5 != NULL)
	{
		pText5->MoveWindow(iLeft, iTop + iHeight2 + 6 + iHeight3 + 6, iTextWidth, iHeight5);
	}

	//按钮挪到新窗口的右下角
	int iBtnTop = iNewHeight - 34;
	if (pBtnCancel != NULL)
	{
		CRect rcBtn;
		pBtnCancel->GetWindowRect(&rcBtn);
		ScreenToClient(&rcBtn);
		pBtnCancel->MoveWindow(iNewWidth - rcBtn.Width() - 12, iBtnTop, rcBtn.Width(), rcBtn.Height());
	}
	if (pBtnOk != NULL)
	{
		CRect rcBtn;
		pBtnOk->GetWindowRect(&rcBtn);
		ScreenToClient(&rcBtn);
		pBtnOk->MoveWindow(iNewWidth - rcBtn.Width() * 2 - 30, iBtnTop, rcBtn.Width(), rcBtn.Height());
	}

	Invalidate();
}

//在运行时补出两个控件：方案下拉框 + 展开详情按钮。
//资源文件里没有它们，全在代码里建，省得动 .rc。
void CAdjustRestSwitchClass::CreateRuntimeControls()
{
	CWnd* pBtnLast = GetDlgItem(IDC_BUTTON_LAST_PLAN);
	if (pBtnLast == NULL)
	{
		return;
	}

	CRect rcBtnLast;
	pBtnLast->GetWindowRect(&rcBtnLast);
	ScreenToClient(&rcBtnLast);

	CRect rcClient;
	GetClientRect(&rcClient);

	int iLeft = rcBtnLast.left;
	int iTop = rcBtnLast.bottom + 8;
	int iHeight = rcBtnLast.Height();

	int iComboWidth = rcClient.Width() - iLeft - 130;
	if (iComboWidth < 160)
	{
		iComboWidth = 160;
	}

	//下拉框：高度要给够，展开的列表高度就是这个矩形的高度
	CRect rcCombo(iLeft, iTop, iLeft + iComboWidth, iTop + 240);
	if (m_comboPlanList.GetSafeHwnd() == NULL)
	{
		m_comboPlanList.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL
			| CBS_DROPDOWNLIST, rcCombo, this, IDC_ADJUST_PLAN_LIST);
		m_comboPlanList.SetFont(GetFont());
	}

	//展开/收起详情按钮
	CRect rcBtn(iLeft + iComboWidth + 10, iTop, iLeft + iComboWidth + 10 + 110, iTop + iHeight);
	if (m_btnToggleDetail.GetSafeHwnd() == NULL)
	{
		m_btnToggleDetail.Create(_T("展开详情"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
			rcBtn, this, IDC_ADJUST_TOGGLE_DETAIL);
		m_btnToggleDetail.SetFont(GetFont());
	}

	//下面的文字控件往下挪，给这一行腾位置
	int iMoveDown = (iTop + iHeight + 10) - rcBtnLast.bottom;
	if (iMoveDown > 0)
	{
		int arrTextId[4] = { IDC_STATIC_1, IDC_STATIC_3, IDC_STATIC_2, IDC_STATIC_5 };
		for (int k = 0; k < 4; k++)
		{
			CWnd* pText = GetDlgItem(arrTextId[k]);
			if (pText == NULL)
			{
				continue;
			}
			CRect rcText;
			pText->GetWindowRect(&rcText);
			ScreenToClient(&rcText);
			pText->MoveWindow(rcText.left, rcText.top + iMoveDown, rcText.Width(), rcText.Height());
		}
	}
}

BOOL CAdjustRestSwitchClass::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	//先把窗口和文字控件撑大，再填内容，否则 4 人联动的方案文字会被截断
	ResizeForLongText();

	//补出方案下拉框和"展开详情"按钮，并把所有方案灌进去
	CreateRuntimeControls();
	FillPlanListCombo();

	ShowReturnClassInfo();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CAdjustRestSwitchClass::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码

	g_vectorReturnClassPersonAndDay.clear();
	if (g_pMainDlg != nullptr)
	{
		g_pMainDlg->PostMessage(WM_REFRESH_GRID, 0, 0);
	}

	DestroyWindow();
}
void CAdjustRestSwitchClass::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this; // 清理非模态对话框动态分配的内存，防止泄露
}

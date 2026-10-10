// CDlgRestDayManagement.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CDlgRestDayManagement.h"

#include "SchedulCommonStruct.h"
#include "PIVAS_ScheduleDlg.h"

#include "string.h"
#include <vector>
#include <map>

using namespace std;


extern void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength);

extern CPIVASScheduleDlg* g_pMainDlg;


//调休补休管理数据
extern vector<PersonRestInfo> g_vectorPersonRestInfo;
extern char ClassName[OneDayClasssNum][50];

extern vector<PersonBasicInfo> g_AllLevel_PersonList;


extern int main_table_temp[quantity][MaxPersonNum][grade];					//储存quantity张排班表



// CDlgRestDayManagement 对话框

IMPLEMENT_DYNAMIC(CDlgRestDayManagement, CDialogEx)

CDlgRestDayManagement::CDlgRestDayManagement(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDlgRestDayManagement, pParent)
{

}

CDlgRestDayManagement::~CDlgRestDayManagement()
{
}

void CDlgRestDayManagement::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_ListCtrl_WorkRestList);
}


BEGIN_MESSAGE_MAP(CDlgRestDayManagement, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CDlgRestDayManagement::OnAddRequestRestDay)
	ON_BN_CLICKED(IDC_BUTTON3, &CDlgRestDayManagement::OnAdjustRestAndNeeWorkPerson)
	ON_BN_CLICKED(IDC_BUTTON4, &CDlgRestDayManagement::OnAdjustRestAndNeeWorkPerson_EX)
END_MESSAGE_MAP()


// CDlgRestDayManagement 消息处理程序

BOOL CDlgRestDayManagement::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化


	m_ListCtrl_WorkRestList.SetExtendedStyle(m_ListCtrl_WorkRestList.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOAUTOARRANGE);

	//将解析的数据显示到ListCtrl中
	m_ListCtrl_WorkRestList.InsertColumn(1, _T("序号"), LVCFMT_CENTER, 60);
	m_ListCtrl_WorkRestList.InsertColumn(2, _T("姓名"), LVCFMT_CENTER, 100);
	m_ListCtrl_WorkRestList.InsertColumn(3, _T("调休类型"), LVCFMT_CENTER, 150);
	m_ListCtrl_WorkRestList.InsertColumn(4, _T("是否指定休息日期"), LVCFMT_CENTER, 150);
	m_ListCtrl_WorkRestList.InsertColumn(5, _T("调休天数"), LVCFMT_CENTER, 100);
	m_ListCtrl_WorkRestList.InsertColumn(6, _T("补班天数"), LVCFMT_CENTER, 100);






	ShowAllPersonRestWorkInfo();


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CDlgRestDayManagement::ShowAllPersonRestWorkInfo()
{


	m_ListCtrl_WorkRestList.DeleteAllItems();

	//m_ListCtrl.SetBkColor(RGB(255, 0, 0)); // 例如，设置为红色


	int iPersonNum = g_vectorPersonRestInfo.size();
	int iLineIndex = 0;
	for (int i = 0; i < iPersonNum; i++)
	{

		if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
		{
			iLineIndex++;
			m_ListCtrl_WorkRestList.InsertItem(iLineIndex, _T(""));
		}
		else if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedRestDay > 0)
		{
			iLineIndex++;
			m_ListCtrl_WorkRestList.InsertItem(iLineIndex, _T(""));
		}
		else if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay > 0)
		{
			iLineIndex++;
			m_ListCtrl_WorkRestList.InsertItem(iLineIndex, _T(""));
		}
		else
		{

		}
	}


	int iColumnIndex = 0;
	iLineIndex = 1;
	for (int i = 0; i < g_vectorPersonRestInfo.size(); i++)
	{
		iColumnIndex = 0;
		CString strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
		int iPersonLevel = g_vectorPersonRestInfo[i].level;
		CString strRestType;
		CString strIsSpecifyDate = _T("否");
		CString strDayNumber = _T(""); //休息日期
		if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
		{
			strRestType = RestOrWorkTye1; //上轮调休这轮补班

			strDayNumber.Format(_T("%d"), g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay);

			CString strIndex;
			strIndex.Format(_T("%d"), iLineIndex); //设置序号
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIndex);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strPersonName);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strRestType);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIsSpecifyDate);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, 0);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strDayNumber);
			iLineIndex++;
		}

		iColumnIndex = 0;

		if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedRestDay > 0)
		{
			strRestType = RestOrWorkTye2; //上轮加班这轮补休
			strDayNumber.Format(_T("%d"), g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedRestDay);

			CString strIndex;
			strIndex.Format(_T("%d"), iLineIndex); //设置序号
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIndex);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strPersonName);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strRestType);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIsSpecifyDate);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strDayNumber);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, 0);			
			iLineIndex++;
		}

	}


	for (int i = 0; i < g_vectorPersonRestInfo.size(); i++)
	{
		iColumnIndex = 0;
		CString strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
		int iPersonLevel = g_vectorPersonRestInfo[i].level;
		CString strRestType;
		CString strIsSpecifyDate = _T("否");
		CString strDayNumber = _T(""); //休息日期
		if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay > 0)
		{
			strRestType = RestOrWorkTye3; //这轮调休这轮补班
			strDayNumber.Format(_T("%d"), g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay);


			CString strIndex;
			strIndex.Format(_T("%d"), iLineIndex); //设置序号
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIndex);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strPersonName);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strRestType);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strIsSpecifyDate);
			iColumnIndex++;
			m_ListCtrl_WorkRestList.SetItemText(iLineIndex - 1, iColumnIndex, strDayNumber);
			iLineIndex++;
		}

		if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedWordDay > 0)
		{
		}

	}


	
}

void CDlgRestDayManagement::OnAddRequestRestDay()
{
	// TODO: 在此添加控件通知处理程序代码
	//弹出申请对话框
	if (m_DlgRequstRestDay.DoModal() == IDOK)
	{
		//将调休表格刷一下
		ShowAllPersonRestWorkInfo();

	}
}

void CDlgRestDayManagement::OnAdjustRestAndNeeWorkPerson()
{
	// TODO: 在此添加控件通知处理程序代码

	vector<AdjustRestAndWorkPerson> vecNeedRestDay;



	//先看看哪些班需要人补，明确的班次和天数
	int iPersonNum = g_vectorPersonRestInfo.size();
	int iLineIndex = 0;
	for (int i = 0; i < iPersonNum; i++)
	{

		if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay > 0)
		{
			//看看哪一天需要调休
			for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
			{
				if (g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType > 0 && !g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].bHasProcessed)
				{
					//需要调休
					RestEnumType iSureRestType = g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType;
					if (iSureRestType == Request_REST || iSureRestType == Request_REST_AND_WORK)
					{
						CString strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
						//获取人员序号
							//获取人员序号
						int iPersonIndex = g_pMainDlg->FindPesrsonIndexByName(strPersonName);
						if (iPersonIndex >= 0)
						{
							AdjustRestAndWorkPerson theRestAndWorkPerson;
							theRestAndWorkPerson.iPersonIndex = iPersonIndex;
							int iClass_OneDay = -1;
							iClass_OneDay = g_pMainDlg->GetPersonOneDayWorkClassIndex(iPersonIndex, iDayIndex);
							theRestAndWorkPerson.iClass_OneDay = iClass_OneDay;
							theRestAndWorkPerson.iDayIndex = iDayIndex;
							theRestAndWorkPerson.iRestType = iSureRestType;
							vecNeedRestDay.push_back(theRestAndWorkPerson);
						}

					}

				}
			}

		}
	}


	//对每个有明确需要调休的班次进行调整，优先找上轮需要补班的人，

	int iNeedRestNum = vecNeedRestDay.size();
	for (int k = 0; k < iNeedRestNum; k++)
	{


		AdjustRestAndWorkPerson theRestAndWorkPerson = vecNeedRestDay[k];
		int iPersonIndex = theRestAndWorkPerson.iPersonIndex;
		int iDayIndex = theRestAndWorkPerson.iDayIndex;
		int iClass_OneDay = theRestAndWorkPerson.iClass_OneDay;
		RestEnumType iRestType = theRestAndWorkPerson.iRestType;
		CString strPersonName = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
		int iPersonLevel = g_vectorPersonRestInfo[iPersonIndex].level;

		CString strOutPut;
		TCHAR strClassName[100] = {0};
		STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
		strOutPut.Format(_T("步骤1：对第%d个人：(%s)调休进行处理，第%d天(%s)班次 \r\n"), k+1, strPersonName.GetBuffer(), iDayIndex+1, strClassName);

		GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


		CString strRestType;
		//这轮调休这轮不补班
		if (iRestType == Request_REST)
		{
			strRestType = RestOrWorkTye4; 
		}
		else if (iRestType == Request_REST_AND_WORK)
		{
			strRestType = RestOrWorkTye3; //这轮休息这轮补班
		}
		else
		{
			continue; //不处理其他类型
		}

		//查看当天谁能补
		//找到当天休息的人
		vector<int> NoWorkPersonList;
		g_pMainDlg->GetThisDayRestPersons(iDayIndex, NoWorkPersonList);

		//看看这些休息的人哪个能补这个班次
		vector<int> NoWorkPersonCanWorkInThisClass;		
		g_pMainDlg->GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);

		int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();
		//若找不到则去找本来这天上班，但是他需要其他天需要调休的人
		if (iFindPersonNum == 0)
		{

		}
		else
		{
			//找到某些人，尝试进行调整

			CString strTempOut;
			strTempOut.Format(_T("步骤2：第%d天，如下人员休息尝试换班："), iDayIndex+1);
			map<CString,int> mapPersonCanWorkInThisClass;
			for (int m = 0; m < iFindPersonNum; m++)
			{
				CString strPersonName = g_AllLevel_PersonList[NoWorkPersonCanWorkInThisClass[m]].strPersonName.c_str();
				mapPersonCanWorkInThisClass[strPersonName] = NoWorkPersonCanWorkInThisClass[m];
				//所有休息的人员列表
				strTempOut += strPersonName;
				strTempOut += _T(",");
			}

			strTempOut += _T("\r\n");
			strOutPut += strTempOut;
			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
			int iPersonNum = g_vectorPersonRestInfo.size();

			//这些人里面有没有上轮休息了这轮无偿补休的人	
			// 
			//优先找上轮休息这需要补班的人
			strTempOut.Format(_T("步骤3：优先找上轮休息这需要补班的人："));
			for (int i = 0; i < iPersonNum; i++)
			{

				if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
				{
					strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
					map<CString, int>::iterator it_temp;
					it_temp = mapPersonCanWorkInThisClass.find(strPersonName);
					if (it_temp != mapPersonCanWorkInThisClass.end())
					{
						strTempOut += strPersonName;
						strTempOut += _T(",");
					}
				}
			}
			strTempOut += _T("\r\n");
			strOutPut += strTempOut;
			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


			g_pMainDlg->ChangeCurrentSheduleToTempTable();
			int iLineIndex = 0;
			bool bHasFindWorkPerson = false;
			for (int i = 0; i < iPersonNum; i++)
			{

				if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
				{
					strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
					map<CString, int>::iterator it_temp;
					it_temp = mapPersonCanWorkInThisClass.find(strPersonName);
					if (it_temp != mapPersonCanWorkInThisClass.end())
					{
						int iPersonIndex_temp = it_temp->second;
						//检查这个人当天上班是否满足连上5天，是否满足上了夜班不能上早班要求
						int iClassIndex = iClass_OneDay + iDayIndex*OneDayClasssNum;
						main_table_temp[0][iPersonIndex_temp][iClassIndex] = 2;
						//检查是否合格
						ClassCheckErroType iErrorType;
						CString strErroDes;
						bool bIsChekOK = g_pMainDlg->bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType, strErroDes);

						

						if (!bIsChekOK)
						{
							main_table_temp[0][iPersonIndex][iClassIndex] = 1;
							main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;

							strTempOut.Format(_T("步骤4：%s-上第%天（%s）班次，检查结果：不通过，错误:%d 不满足：%s 规则 \r\n"), strPersonName.GetBuffer(), iDayIndex + 1, strClassName, 
								iErrorType, strErroDes.GetBuffer());

							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
						}
						else//能给换
						{
							main_table_temp[0][iPersonIndex][iClassIndex] = 0;
							bHasFindWorkPerson = true;

							strTempOut.Format(_T("步骤4：%s-上第%d天（%s）班次，检查结果：通过\r\n"), strPersonName.GetBuffer(), iDayIndex+1, strClassName);

							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


							g_pMainDlg->ChangeTempSheduleToCurrentTable();
							g_pMainDlg->ShowAllPersonToList(false, false, true);


							//将找到的人的补班天数减去1
							g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay -= 1;
							break;

						}



					}
				}
			}


			if (!bHasFindWorkPerson)
			{

				strTempOut.Format(_T("步骤5：未找到合适上班人员，查找这轮调休这轮补班的人，能不能跟其进行对调\r\n"));
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

				//这些人有没有这轮调休且需要补班的人

				for (int i = 0; i < iPersonNum; i++)
				{

					if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay > 0)
					{
						CString strPersonName_temp3 = g_vectorPersonRestInfo[i].strPersonName.c_str();
						int strPersonLevel_temp3 = g_vectorPersonRestInfo[i].level;

						bool bCheck_temp1 = false;
						bool bCheck_temp3 = false;
						//
						vector<int> Request_REST_DAY;
						for (int iDayInex_temp3 = 0; iDayInex_temp3 < AllDay; iDayInex_temp3++)
						{
							if (iDayInex_temp3 != iDayIndex && g_vectorPersonRestInfo[i].NeedRestDay[iDayInex_temp3].restType == Request_REST_AND_WORK
								 && !g_vectorPersonRestInfo[i].NeedRestDay[iDayInex_temp3].bHasProcessed)
							{
								bCheck_temp1 = true; 
								Request_REST_DAY.push_back(iDayInex_temp3);
							}
						}
						if (bCheck_temp1)
						{

							int iPersonIndex_temp3 = g_pMainDlg->FindPesrsonIndexByName(strPersonName_temp3);
							map<CString, int>::iterator it_temp;
							it_temp = mapPersonCanWorkInThisClass.find(strPersonName);
							if (it_temp != mapPersonCanWorkInThisClass.end())
							{
								int iALLDay = Request_REST_DAY.size();
								for (int Index = 0; Index < iALLDay; Index++)
								{
									int iDayInex_temp3 = Request_REST_DAY[Index];

									//获取某人某天的班次名称
									int iClassInex_temp3 = -1;
									int iClass_ChangeRelaxValue = 0;
									CString strClassName_temp3 = g_pMainDlg->GetClassNameOneDayOnePerson(iDayInex_temp3, iPersonIndex_temp3, iClassInex_temp3, iClass_ChangeRelaxValue);
									strTempOut.Format(_T("步骤6：找到这轮调休这轮补班的人：%s，尝试将其%d天的（%s）班与%s的%d天的（%s）班对调\r\n"),
										strPersonName_temp3.GetBuffer(), iDayInex_temp3, strClassName_temp3.GetBuffer(),
										strPersonName.GetBuffer(), iDayIndex, strClassName);

									ClassCheckErroType iErrorType_temp3 = HasNO_ERROR;
									//判断一下是否能对调
									bool bCheckOk = g_pMainDlg->JudgeCanSwitchClassByTwoPerson(iPersonIndex, iDayIndex, iClass_OneDay, iPersonLevel,
										iPersonIndex_temp3, iDayInex_temp3, iClassInex_temp3, strPersonLevel_temp3, iErrorType_temp3);

									if (bCheckOk)
									{
										//对调成功
										bCheck_temp3 = true;

										strTempOut.Format(_T("步骤7：这轮调休这轮补班的人：%s，其%d天的（%s）班与%s的%d天的（%s）班对调成功！\r\n"),
											strPersonName_temp3.GetBuffer(), iDayInex_temp3, strClassName_temp3.GetBuffer(),
											strPersonName.GetBuffer(), iDayIndex, strClassName);

										//将找到的那个人那天的期望调休跟补班置成已完成

										break;
									}
									else 
									{
										bCheck_temp3 = false;
										//对调失败
										strTempOut.Format(_T("步骤7：这轮调休这轮补班的人：%s，其%d天的（%s）班与%s的%d天的（%s）班对调失败，继续找其他人！\r\n"),
											strPersonName_temp3.GetBuffer(), iDayInex_temp3, strClassName_temp3.GetBuffer(),
											strPersonName.GetBuffer(), iDayIndex, strClassName);
									}
								}

							}
							if (bCheck_temp3)
							{
								break;
							}


						}

					}
				}


			}






			
		}

		//g_pMainDlg->AddRequestRestDay(strPersonName, iPersonLevel, strRestType, iDayIndex, 1);

	}



}

void CDlgRestDayManagement::OnAdjustRestAndNeeWorkPerson_EX()
{
	// TODO: 在此添加控件通知处理程序代码

	//对每天需要调休的人员进行处理
	
	

	for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		vector<AdjustRestAndWorkPerson> vecNeedRestDay;
		int iPersonNum = g_vectorPersonRestInfo.size();
		for (int i = 0; i < iPersonNum; i++)
		{
			if (g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay > 0)
			{
				if (g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType > 0 && !g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].bHasProcessed)
				{
					CString strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
					RestEnumType iSureRestType = g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType;
					//获取人员序号					
					int iPersonIndex = g_pMainDlg->FindPesrsonIndexByName(strPersonName);
					if (iPersonIndex >= 0)
					{
						AdjustRestAndWorkPerson theRestAndWorkPerson;
						theRestAndWorkPerson.iPersonIndex = iPersonIndex;
						int iClass_OneDay = -1;
						iClass_OneDay = g_pMainDlg->GetPersonOneDayWorkClassIndex(iPersonIndex, iDayIndex);
						theRestAndWorkPerson.iClass_OneDay = iClass_OneDay;
						theRestAndWorkPerson.iDayIndex = iDayIndex;
						theRestAndWorkPerson.iRestType = iSureRestType;
						vecNeedRestDay.push_back(theRestAndWorkPerson);
					}
				}
			}
		}




		int iNeedRestNum = vecNeedRestDay.size();
		for (int k = 0; k < iNeedRestNum; k++)
		{


			AdjustRestAndWorkPerson theRestAndWorkPerson = vecNeedRestDay[k];
			int iPersonIndex = theRestAndWorkPerson.iPersonIndex;
			int iDayIndex = theRestAndWorkPerson.iDayIndex;
			int iClass_OneDay = theRestAndWorkPerson.iClass_OneDay;
			RestEnumType iRestType = theRestAndWorkPerson.iRestType;
			CString strPersonName = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
			int iPersonLevel = g_pMainDlg->FindPesrsonLevelByName(strPersonName);

			CString strOutPut;
			TCHAR strClassName[100] = { 0 };
			STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
			strOutPut.Format(_T("步骤1：对第%d个人：(%s)调休进行处理，第%d天(%s)班次 \r\n"), k + 1, strPersonName.GetBuffer(), iDayIndex + 1, strClassName);

			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


			CString strRestType;
			//这轮调休这轮不补班
			if (iRestType == Request_REST)
			{
				strRestType = RestOrWorkTye4;
			}
			else if (iRestType == Request_REST_AND_WORK)
			{
				strRestType = RestOrWorkTye3; //这轮休息这轮补班
			}
			else
			{
				continue; //不处理其他类型
			}

			//查看当天谁能补
			//找到当天调休的人
			vector<int> NoWorkPersonList;
			g_pMainDlg->GetThisDayRestPersons(iDayIndex, NoWorkPersonList);

			//看看这些休息的人哪个能补这个班次
			vector<int> NoWorkPersonCanWorkInThisClass;
			g_pMainDlg->GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);

			int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();
			//若找不到则去找本来这天上班，但是他需要其他天需要调休的人
			if (iFindPersonNum == 0)
			{
				CString strTempOut;
				strTempOut.Format(_T("步骤2：对第%d个人：(%s)调休进行处理，第%d天(%s)班次,未找到能调休的人，不允许调休"), k + 1, strPersonName.GetBuffer(), iDayIndex + 1, strClassName);
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
			}
			else
			{
				//找到某些人，尝试进行调整

				CString strTempOut;
				strTempOut.Format(_T("步骤2：第%d天，如下人员休息尝试换班："), iDayIndex + 1);
				map<CString, int> mapPersonCanWorkInThisClass;
				for (int m = 0; m < iFindPersonNum; m++)
				{
					CString strPersonName = g_AllLevel_PersonList[NoWorkPersonCanWorkInThisClass[m]].strPersonName.c_str();
					mapPersonCanWorkInThisClass[strPersonName] = NoWorkPersonCanWorkInThisClass[m];
					//所有休息的人员列表
					strTempOut += strPersonName;
					strTempOut += _T(",");
				}

				strTempOut += _T("\r\n");
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				//int iPersonNum = g_vectorPersonRestInfo.size();

				////这些人里面有没有上轮休息了这轮无偿补休的人	
				//// 
				////优先找上轮休息这需要补班的人
				//strTempOut.Format(_T("步骤3：优先找上轮休息这需要补班的人："));
				//for (int i = 0; i < iPersonNum; i++)
				//{

				//	if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
				//	{
				//		strPersonName = g_vectorPersonRestInfo[i].strPersonName.c_str();
				//		map<CString, int>::iterator it_temp;
				//		it_temp = mapPersonCanWorkInThisClass.find(strPersonName);
				//		if (it_temp != mapPersonCanWorkInThisClass.end())
				//		{
				//			strTempOut += strPersonName;
				//			strTempOut += _T(",");
				//		}
				//	}
				//}
				//strTempOut += _T("\r\n");
				//strOutPut += strTempOut;
				//GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


				g_pMainDlg->ChangeCurrentSheduleToTempTable();
				int iLineIndex = 0;
				bool bHasFindWorkPerson = false;
				int iPersonNum = NoWorkPersonCanWorkInThisClass.size();
				for (int i = 0; i < iPersonNum; i++)
				{

					//if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
					{
						
						int iPersonIndex_temp = NoWorkPersonCanWorkInThisClass[i];
						strPersonName = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();

						//检查这个人当天上班是否满足连上5天，是否满足上了夜班不能上早班要求
						int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
						main_table_temp[0][iPersonIndex_temp][iClassIndex] = 2;
						//检查是否合格
						ClassCheckErroType iErrorType;
						CString strErroDes;
						bool bIsChekOK = g_pMainDlg->bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType, strErroDes);



						if (!bIsChekOK)
						{
							main_table_temp[0][iPersonIndex][iClassIndex] = 1;
							main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;

							strTempOut.Format(_T("步骤4：%s-上第%天（%s）班次，检查结果：不通过，错误:%d \r\n"), strPersonName.GetBuffer(), iDayIndex + 1, strClassName, iErrorType);

							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
						}
						else//能给换
						{
							main_table_temp[0][iPersonIndex][iClassIndex] = 0;
							bHasFindWorkPerson = true;

							strTempOut.Format(_T("步骤4：%s-上第%d天（%s）班次，检查结果：通过\r\n"), strPersonName.GetBuffer(), iDayIndex + 1, strClassName);

							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


							g_pMainDlg->ChangeTempSheduleToCurrentTable();
							g_pMainDlg->ShowAllPersonToList(false, false, true);


							//将找到的人的补班天数减去1
							g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay -= 1;
							break;

						}



						
					}
				}


				if (!bHasFindWorkPerson)
				{

					strTempOut.Format(_T("步骤5：对第%d个人：(%s)调休进行处理，第%d天(%s)班次,调休失败未找到合适上班的人\r\n"),k + 1, strPersonName.GetBuffer(), iDayIndex + 1, strClassName);
					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

				}


			}

			//g_pMainDlg->AddRequestRestDay(strPersonName, iPersonLevel, strRestType, iDayIndex, 1);

		}


	}





	


}

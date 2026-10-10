// CDiagRetunClassSelect.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CDiagRetunClassSelect.h"
#include "PIVAS_ScheduleDlg.h"

extern vector<PersonBasicInfo> g_AllLevel_PersonList;
extern char ClassName[OneDayClasssNum][50];
extern void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength);


extern CPIVASScheduleDlg* g_pMainDlg;

extern vector<ReturnClassPersonAndDay> g_vectorReturnClassPersonAndDay;

extern int g_bAutoSave;



// CDiagRetunClassSelect 对话框

IMPLEMENT_DYNAMIC(CDiagRetunClassSelect, CDialogEx)

CDiagRetunClassSelect::CDiagRetunClassSelect(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CDiagRetunClassSelect, pParent)
{
	m_iCurrentSelectIndex = -1;
}

CDiagRetunClassSelect::~CDiagRetunClassSelect()
{
}

void CDiagRetunClassSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDiagRetunClassSelect, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LAST_PLAN, &CDiagRetunClassSelect::OnBnClickedButtonLastPlan)
	ON_BN_CLICKED(IDC_BUTTON_NEXT_PLAN, &CDiagRetunClassSelect::OnBnClickedButtonNextPlan)
	ON_BN_CLICKED(IDOK, &CDiagRetunClassSelect::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDiagRetunClassSelect::OnBnClickedCancel)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CDiagRetunClassSelect 消息处理程序

BOOL CDiagRetunClassSelect::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_iCurrentSelectIndex = 0;

	ShowReturnClassInfo();



	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
void CDiagRetunClassSelect::InitData(vector<ReturnClassStruct>& theReturnClassInfo)
{
	m_theReturnClassInfo = theReturnClassInfo;
}



void CDiagRetunClassSelect::OnBnClickedButtonLastPlan()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_iCurrentSelectIndex <= 0) return;
	m_iCurrentSelectIndex--;
	ShowReturnClassInfo();
}

void CDiagRetunClassSelect::OnBnClickedButtonNextPlan()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_iCurrentSelectIndex >= (int)m_theReturnClassInfo.size() - 1) return;
	m_iCurrentSelectIndex++;
	ShowReturnClassInfo();
}

void CDiagRetunClassSelect::ShowReturnClassInfo()
{

	g_vectorReturnClassPersonAndDay.clear();

	CString strIndex;
	strIndex.Format(_T("当前方案：%d / %d"), m_iCurrentSelectIndex+1, m_theReturnClassInfo.size());
	SetDlgItemText(IDC_STATIC_0, strIndex);

	
	
	ReturnClassStruct theReturnClassInfo = m_theReturnClassInfo[m_iCurrentSelectIndex];
	CString strData1;
	strData1.Format(_T("第%d天"), theReturnClassInfo.iDayIndex_Source + 1);

	CString strData2;

	CString strPerson_Source = g_AllLevel_PersonList[theReturnClassInfo.iPerson_Source].strPersonName.c_str();
	CString strPerson_Dst = g_AllLevel_PersonList[theReturnClassInfo.iPerson_Dst].strPersonName.c_str();


	TCHAR strSrcClass_Source[100] = { 0 };
	STRING2TCHAR(ClassName[theReturnClassInfo.iClass_OneDay_SrcPerson_SrcClass], strSrcClass_Source, 100);

	TCHAR strSrcClass_dst[100] = { 0 };
	STRING2TCHAR(ClassName[theReturnClassInfo.iClass_OneDay_DstPerson_SrcClass], strSrcClass_dst, 100);


	strData2.Format(_T("%s的%s班次与%s的%s班次更换"), strPerson_Source, strSrcClass_Source, strPerson_Dst, strSrcClass_dst);


	CString strData3;
	strData3.Format(_T("第%d天"), theReturnClassInfo.iDayIndex_Dst + 1);

	CString strData4;

	TCHAR strDstClass_Source[100] = { 0 };
	STRING2TCHAR(ClassName[theReturnClassInfo.iClass_OneDay_SrcPerson_DstClass], strDstClass_Source, 100);

	TCHAR strDsrClass_dst[100] = { 0 };
	STRING2TCHAR(ClassName[theReturnClassInfo.iClass_OneDay_DstPerson_DstClass], strDsrClass_dst, 100);

	strData4.Format(_T("%s的%s班次与%s的%s班次更换"), strPerson_Source, strDstClass_Source, strPerson_Dst, strDsrClass_dst);




	SetDlgItemText(IDC_STATIC_1, strData1);
	SetDlgItemText(IDC_STATIC_2, strData2);
	SetDlgItemText(IDC_STATIC_3, strData3);
	SetDlgItemText(IDC_STATIC_4, strData4);


	ReturnClassPersonAndDay theReturnClassPersonAndDay1;
	theReturnClassPersonAndDay1.iPersonIndex = theReturnClassInfo.iPerson_Source;
	theReturnClassPersonAndDay1.iDayIndex = theReturnClassInfo.iDayIndex_Source;
	ReturnClassPersonAndDay theReturnClassPersonAndDay2;
	theReturnClassPersonAndDay2.iPersonIndex = theReturnClassInfo.iPerson_Dst;
	theReturnClassPersonAndDay2.iDayIndex = theReturnClassInfo.iDayIndex_Source;
	ReturnClassPersonAndDay theReturnClassPersonAndDay3;
	theReturnClassPersonAndDay3.iPersonIndex = theReturnClassInfo.iPerson_Rest;
	theReturnClassPersonAndDay3.iDayIndex = theReturnClassInfo.iDayIndex_Source;

	ReturnClassPersonAndDay theReturnClassPersonAndDay4;
	theReturnClassPersonAndDay4.iPersonIndex = theReturnClassInfo.iPerson_Source;
	theReturnClassPersonAndDay4.iDayIndex = theReturnClassInfo.iDayIndex_Dst;

	ReturnClassPersonAndDay theReturnClassPersonAndDay5;
	theReturnClassPersonAndDay5.iPersonIndex = theReturnClassInfo.iPerson_Dst;
	theReturnClassPersonAndDay5.iDayIndex = theReturnClassInfo.iDayIndex_Dst;


	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay1);
	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay2);
	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay3);
	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay4);
	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay5);


	//g_pMainDlg->ShowAllPersonToList(false, false, true);

	// 【修改此处】：彻底废弃全量刷新和异步消息投递
	// 直接同步调用纯颜色刷新函数。由于该函数内部不包含任何控件重建操作，
	// 即使瞬间高频触发，UI 依然能在 1 毫秒内响应完毕，绝不阻塞。
	if (g_pMainDlg != nullptr)
	{
		g_pMainDlg->UpdateGridColorsOnly();
	}

	//g_vectorReturnClassPersonAndDay.clear();

}
void CDiagRetunClassSelect::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//将当前方案更新到排班表中，并刷一次界面



	//g_vectorReturnClassPersonAndDay.clear();



	g_pMainDlg->TakeRetunrClassToCurentTable(m_theReturnClassInfo[m_iCurrentSelectIndex]);
	//g_pMainDlg->ShowAllPersonToList(false, false, true);
	
	//撤销恢复
	g_pMainDlg->AddPlanToCurrentRedoArray();
	if (g_bAutoSave)
	{
		g_pMainDlg->OnBnClickedButton_Save();
	}

	//弹个MessgaeBox提示成功
	MessageBox(_T("换班成功！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
	//g_vectorReturnClassPersonAndDay.clear();
	//g_pMainDlg->ShowAllPersonToList(false, false, true);
	if (g_pMainDlg != nullptr)
	{
		g_pMainDlg->PostMessage(WM_REFRESH_GRID, 0, 0);
	}



	CDialogEx::OnOK();
}

void CDiagRetunClassSelect::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码

	g_vectorReturnClassPersonAndDay.clear();
	//g_pMainDlg->ShowAllPersonToList(false, false, true);

	CDialogEx::OnCancel();
}

void CDiagRetunClassSelect::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	g_vectorReturnClassPersonAndDay.clear();

	//g_pMainDlg->ShowAllPersonToList(false, false, true);


	CDialogEx::OnClose();
}

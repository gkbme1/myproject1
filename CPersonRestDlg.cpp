// CPersonRestDlg.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CPersonRestDlg.h"
#include "SchedulCommonStruct.h"

#include "NewCellTypes/GridURLCell.h"
#include "NewCellTypes/GridCellCombo.h"
#include "NewCellTypes/GridCellCheck.h"
#include "NewCellTypes/GridCellNumeric.h"
#include "NewCellTypes/GridCellDateTime.h"

using namespace std;
#include <map>

extern vector<PersonBasicInfo> g_Level1_PersonList;
extern vector<PersonBasicInfo> g_Level2_PersonList;
extern vector<PersonBasicInfo> g_Level3_PersonList;
extern vector<PersonBasicInfo> g_Level4_PersonList;
extern vector<PersonBasicInfo> g_AllLevel_PersonList;

extern string g_filePath;
extern std::string& CStringtoString(std::string& strDst, const CString& strSrc);

extern map<CString, PersonRestDay > g_mapPersonRestDayNum;

// CPersonRestDlg 对话框

IMPLEMENT_DYNAMIC(CPersonRestDlg, CDialogEx)

CPersonRestDlg::CPersonRestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CPersonRestDlg, pParent)
{

}

CPersonRestDlg::~CPersonRestDlg()
{
}

void CPersonRestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GRID_Person, m_pGrid_Person);
}


BEGIN_MESSAGE_MAP(CPersonRestDlg, CDialogEx)
	ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID_Person, OnGridEndEdit)

	ON_BN_CLICKED(IDOK, &CPersonRestDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CPersonRestDlg 消息处理程序

void CPersonRestDlg::PutPersonToList(vector<PersonBasicInfo>& theLevel_PersonList, int& iRowCount)
{
	GV_ITEM Item;
	COLORREF clr_text = G_Text_Defalut_COLORREF;
	COLORREF clr_BK = G_Bk_Defalut_COLORREF;
	//vector<wstring> vOptions; 
	CStringArray vOptions;//下拉选项
	vOptions.Add(_T("0"));
	vOptions.Add(_T("1"));
	vOptions.Add(_T("2"));
	vOptions.Add(_T("3"));
	vOptions.Add(_T("4"));
	vOptions.Add(_T("5"));
	vOptions.Add(_T("6"));
	vOptions.Add(_T("7"));
	vOptions.Add(_T("8"));
	vOptions.Add(_T("9"));
	vOptions.Add(_T("10"));
	vOptions.Add(_T("11"));
	vOptions.Add(_T("12"));
	vOptions.Add(_T("13"));
	vOptions.Add(_T("14"));
	vOptions.Add(_T("15"));
	vOptions.Add(_T("16"));
	vOptions.Add(_T("17"));
	vOptions.Add(_T("18"));
	vOptions.Add(_T("19"));
	vOptions.Add(_T("20"));


	CStringArray vOptions_Lock;//下拉选项
	vOptions_Lock.Add(_T("不锁定"));
	vOptions_Lock.Add(_T("锁定"));

	for (int k = 0; k < theLevel_PersonList.size(); k++)
	{
		//序号
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = iRowCount + 1;
		Item.col = 0;
		Item.nFormat = DT_CENTER | DT_WORDBREAK;
		CString strIndex;
		strIndex.Format(_T("%d"), iRowCount + 1);
		Item.strText = strIndex;
		m_pGrid_Person.SetItem(&Item);
		//级别
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = iRowCount + 1;
		Item.col = 1;
		Item.nFormat = DT_CENTER | DT_WORDBREAK;
		Item.strText = _T("级别1");
		m_pGrid_Person.SetItem(&Item);
		//人员
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = iRowCount + 1;
		Item.col = 2;
		Item.nFormat = DT_CENTER | DT_WORDBREAK;
		CString strPerson;
		strPerson = theLevel_PersonList[k].strPersonName.c_str();
		Item.strText = strPerson;
		m_pGrid_Person.SetItem(&Item);
		//年假剩余天数


		m_pGrid_Person.SetCellType(iRowCount + 1, 3, RUNTIME_CLASS(CGridCellCombo));
		CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 3);
		//pCell->SetBackClr(clr_BK);
		pCell->SetOptions(vOptions);
		pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
		pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
		m_pGrid_Person.SetItemText(iRowCount + 1, 3, _T("0"));
		pCell->SetTextClr(clr_text);

		//已自动调休天数
		Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIS_READONLY;
		Item.row = iRowCount + 1;
		Item.col = 4;
		Item.nFormat = DT_CENTER | DT_WORDBREAK;
		CString strHasUsedRestDay;
		map<CString, PersonRestDay >::iterator iter_temp_Peer = g_mapPersonRestDayNum.find(strPerson);
		if (iter_temp_Peer != g_mapPersonRestDayNum.end())
		{
			strHasUsedRestDay.Format(_T("%d"), iter_temp_Peer->second.iHasUsedDay);
		}
		else
		{
			strHasUsedRestDay.Format(_T("%d"), 0);
		}
		Item.strText = strHasUsedRestDay;
		m_pGrid_Person.SetItem(&Item);


		m_pGrid_Person.SetCellType(iRowCount + 1, 5, RUNTIME_CLASS(CGridCellCombo));
		pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 5);
		//pCell->SetBackClr(clr_BK);
		pCell->SetOptions(vOptions_Lock);
		pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
		pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
		m_pGrid_Person.SetItemText(iRowCount + 1, 5, _T("不锁定"));
		pCell->SetTextClr(clr_text);

		//Item.mask = GVIF_TEXT | GVIF_FORMAT;
		//Item.row = iRowCount + 1;
		//Item.col = 3;
		//Item.nFormat = DT_CENTER | DT_WORDBREAK;
		//Item.strText = _T("15");
		//m_pGrid_Person.SetItem(&Item);

		iRowCount += 1;
	}

}



BOOL CPersonRestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化


	m_pGrid_Person;
	int iPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();

	m_pGrid_Person.SetEditable(true);
	m_pGrid_Person.SetTextBkColor(RGB(0xFF, 0xFF, 0xE0));//黄色背景
	m_pGrid_Person.SetRowCount(iPersonNum + 1); //初始为10行
	m_pGrid_Person.SetColumnCount(6); //初始化为11列
	m_pGrid_Person.SetFixedRowCount(1); //表头为一行
	m_pGrid_Person.SetFixedColumnCount(1); //表头为一列


	m_pGrid_Person.SetRowHeight(0, 25); //设置各行高          
	m_pGrid_Person.SetColumnWidth(0, 50); //设置0列宽 


	//设置表格显示属性
	GV_ITEM Item;
	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 0;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("序号");
	m_pGrid_Person.SetItem(&Item);

	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 1;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("级别");
	m_pGrid_Person.SetItem(&Item);

	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 2;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("人员");
	m_pGrid_Person.SetItem(&Item);


	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 3;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("年假剩余天数");
	m_pGrid_Person.SetItem(&Item);


	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 4;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("自动调休使用天数");
	m_pGrid_Person.SetItem(&Item);

	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	Item.row = 0;
	Item.col = 5;
	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	Item.strText = _T("班次锁定");
	m_pGrid_Person.SetItem(&Item);


	int iRowCount = 0;
	


	//将所有人员列到grid中
	PutPersonToList(g_Level1_PersonList, iRowCount);
	PutPersonToList(g_Level2_PersonList, iRowCount);
	PutPersonToList(g_Level3_PersonList, iRowCount);
	PutPersonToList(g_Level4_PersonList, iRowCount);
	ReadPersonRestFromCSV();

	//for (int k = 0; k < g_Level1_PersonList.size(); k++)
	//{
	//	//序号
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 0;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strIndex;
	//	strIndex.Format(_T("%d"), iRowCount + 1);
	//	Item.strText = strIndex;
	//	m_pGrid_Person.SetItem(&Item);
	//	//级别
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 1;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	Item.strText = _T("级别1");
	//	m_pGrid_Person.SetItem(&Item);
	//	//人员
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 2;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strPerson;
	//	strPerson = g_Level1_PersonList[k].strPersonName.c_str();
	//	Item.strText = strPerson;
	//	m_pGrid_Person.SetItem(&Item);
	//	//年假剩余天数


	//	m_pGrid_Person.SetCellType(iRowCount+1, 3, RUNTIME_CLASS(CGridCellCombo));
	//	CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount+1, 3);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount+1, 3, _T("0"));
	//	pCell->SetTextClr(clr_text);

	//	//已自动调休天数
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 4;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strHasUsedRestDay;
	//	map<CString, PersonRestDay >::iterator iter_temp_Peer = g_mapPersonRestDayNum.find(strPerson);
	//	if (iter_temp_Peer != g_mapPersonRestDayNum.end())
	//	{
	//		strHasUsedRestDay = itoa(iter_temp_Peer->second.iHasUsedDay);
	//	}
	//	Item.strText = strHasUsedRestDay;
	//	m_pGrid_Person.SetItem(&Item);


	//	m_pGrid_Person.SetCellType(iRowCount + 1, 5, RUNTIME_CLASS(CGridCellCombo));
	//	pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 4);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions_Lock);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 5, _T("不锁定"));
	//	pCell->SetTextClr(clr_text);

	//	//Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	//Item.row = iRowCount + 1;
	//	//Item.col = 3;
	//	//Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	//Item.strText = _T("15");
	//	//m_pGrid_Person.SetItem(&Item);

	//	iRowCount += 1;
	//}


	//for (int k = 0; k < g_Level2_PersonList.size(); k++)
	//{
	//	//序号
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 0;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strIndex;
	//	strIndex.Format(_T("%d"), iRowCount + 1);
	//	Item.strText = strIndex;
	//	m_pGrid_Person.SetItem(&Item);
	//	//级别
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 1;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	Item.strText = _T("级别2");
	//	m_pGrid_Person.SetItem(&Item);
	//	//人员
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 2;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strPerson;
	//	strPerson = g_Level2_PersonList[k].strPersonName.c_str();
	//	Item.strText = strPerson;
	//	m_pGrid_Person.SetItem(&Item);
	//	m_pGrid_Person.GetCell(iRowCount + 1, 2)->SetTextClr(G_Text_Relax_INIT_COLORREF);


	//	
	//	//年假剩余天数
	//	m_pGrid_Person.SetCellType(iRowCount + 1, 3, RUNTIME_CLASS(CGridCellCombo));
	//	CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 3);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 3, _T("0"));
	//	pCell->SetTextClr(clr_text);

	//	m_pGrid_Person.SetCellType(iRowCount + 1, 4, RUNTIME_CLASS(CGridCellCombo));
	//	pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 4);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions_Lock);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 4, _T("不锁定"));
	//	pCell->SetTextClr(clr_text);

	//	iRowCount += 1;
	//}

	//for (int k = 0; k < g_Level3_PersonList.size(); k++)
	//{
	//	//序号
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 0;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strIndex;
	//	strIndex.Format(_T("%d"), iRowCount + 1);
	//	Item.strText = strIndex;
	//	m_pGrid_Person.SetItem(&Item);
	//	//级别
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 1;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	Item.strText = _T("级别3");
	//	m_pGrid_Person.SetItem(&Item);
	//	//人员
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 2;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strPerson;
	//	strPerson = g_Level3_PersonList[k].strPersonName.c_str();
	//	Item.strText = strPerson;
	//	m_pGrid_Person.SetItem(&Item);
	//	m_pGrid_Person.GetCell(iRowCount + 1, 2)->SetTextClr(G_Text_Purple_COLORREF);

	//	//年假剩余天数
	//	m_pGrid_Person.SetCellType(iRowCount + 1, 3, RUNTIME_CLASS(CGridCellCombo));
	//	CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 3);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 3, _T("0"));
	//	pCell->SetTextClr(clr_text);

	//	m_pGrid_Person.SetCellType(iRowCount + 1, 4, RUNTIME_CLASS(CGridCellCombo));
	//	pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 4);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions_Lock);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 4, _T("不锁定"));
	//	pCell->SetTextClr(clr_text);

	//	iRowCount += 1;
	//}


	//for (int k = 0; k < g_Level4_PersonList.size(); k++)
	//{
	//	//序号
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 0;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strIndex;
	//	strIndex.Format(_T("%d"), iRowCount + 1);
	//	Item.strText = strIndex;
	//	m_pGrid_Person.SetItem(&Item);
	//	//级别
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 1;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	Item.strText = _T("级别4");
	//	m_pGrid_Person.SetItem(&Item);
	//	//人员
	//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
	//	Item.row = iRowCount + 1;
	//	Item.col = 2;
	//	Item.nFormat = DT_CENTER | DT_WORDBREAK;
	//	CString strPerson;
	//	strPerson = g_Level4_PersonList[k].strPersonName.c_str();
	//	Item.strText = strPerson;
	//	m_pGrid_Person.SetItem(&Item);
	//	m_pGrid_Person.GetCell(iRowCount + 1, 2)->SetTextClr(G_Text_Blue_COLORREF);



	//	//年假剩余天数
	//	m_pGrid_Person.SetCellType(iRowCount + 1, 3, RUNTIME_CLASS(CGridCellCombo));
	//	CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 3);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 3, _T("0"));
	//	pCell->SetTextClr(clr_text);

	//	m_pGrid_Person.SetCellType(iRowCount + 1, 4, RUNTIME_CLASS(CGridCellCombo));
	//	pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(iRowCount + 1, 4);
	//	pCell->SetBackClr(clr_BK);
	//	pCell->SetOptions(vOptions_Lock);
	//	pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
	//	pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
	//	m_pGrid_Person.SetItemText(iRowCount + 1, 4, _T("不锁定"));
	//	pCell->SetTextClr(clr_text);


	//	iRowCount += 1;
	//}



	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
void CPersonRestDlg::OnGridEndEdit(NMHDR* pNotifyStruct, LRESULT* pResult)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;

	int iRow = pItem->iRow;
	int iColumn = pItem->iColumn;

	if (iColumn < 3)
	{
		return;
	}

	//将当前所有数据保存到csv文件中
	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员假期信息.csv";

	//CStringtoString(filename, strFullFilePath);
	fp = fopen(filename.c_str(), "w");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}

	char* chData = new char[200000];
	memset(chData, 0, 200000);
	char* chData_temp = chData;

	sprintf(chData, "序号, 级别, 人员, 年假剩余天数,\n");
	chData += strlen(chData);

	int iPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();


	for (int RowNum = 1; RowNum < iPersonNum + 1; RowNum++)
	{
		CString strPersonName;
		CString strRestDay;
		CString strHasUsedDay;
		PersonRestDay thePersonRestDay;
		for (int iColNum = 0; iColNum < 6; iColNum++)
		{

			CString strData = m_pGrid_Person.GetItemText(RowNum, iColNum);
			if (strData.IsEmpty())
			{
				strData = _T(" ");
			}


			string strOneCellData;
			CStringtoString(strOneCellData, strData);
			strcat(chData, strOneCellData.c_str());
			strcat(chData, ",");
			if (iColNum == 2)
			{
				strPersonName = strData;
			}
			if (iColNum == 3)
			{
				strRestDay = strData;
				thePersonRestDay.iAllRestDay = _ttoi(strRestDay);
				//g_mapPersonRestDayNum[strPersonName] = thePersonRestDay;
			}
			if (iColNum == 4)
			{
				strHasUsedDay = strData;
				thePersonRestDay.iHasUsedDay = _ttoi(strHasUsedDay);
				g_mapPersonRestDayNum[strPersonName] = thePersonRestDay;
			}
			if (iColNum == 5)
			{
				//班次锁定
				CString strLockInfo = strData;
				UpDateLockInfoToPersonList(strLockInfo, strPersonName);

			}

		}
		//CStringtoString(strPersonName, g_Level1_PersonList[k].strPersonName.c_str());
		//sprintf(chData, "%d,%d,%s,\n", k, 1, strPersonName.c_str());
		strcat(chData, "\n");
		chData += strlen(chData);
	}




	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_SET);
		fwrite(chData_temp, strlen(chData_temp) + 1, 1, fp);
		fclose(fp);
	}
	if (chData_temp)
	{
		delete chData_temp;
	}

}


void CPersonRestDlg::UpDateLockInfoToPersonList(CString strLockInfo, CString strPersonName)
{
	for (int k = 0; k < g_Level1_PersonList.size(); k++)
	{
		if (g_Level1_PersonList[k].strPersonName.c_str() == strPersonName)
		{
			if (strLockInfo == _T("锁定"))
			{
				g_Level1_PersonList[k].bShiftLocked = true;
			}
			else
			{
				g_Level1_PersonList[k].bShiftLocked = false;
			}
			break;
		}
	}

	for (int k = 0; k < g_Level2_PersonList.size(); k++)
	{
		if (g_Level2_PersonList[k].strPersonName.c_str() == strPersonName)
		{
			if (strLockInfo == _T("锁定"))
			{
				g_Level2_PersonList[k].bShiftLocked = true;
			}
			else
			{
				g_Level2_PersonList[k].bShiftLocked = false;
			}
			break;
		}
	}

	for (int k = 0; k < g_Level3_PersonList.size(); k++)
	{
		if (g_Level3_PersonList[k].strPersonName.c_str() == strPersonName)
		{
			if (strLockInfo == _T("锁定"))
			{
				g_Level3_PersonList[k].bShiftLocked = true;
			}
			else
			{
				g_Level3_PersonList[k].bShiftLocked = false;
			}
			break;
		}
	}

	for (int k = 0; k < g_Level4_PersonList.size(); k++)
	{
		if (g_Level4_PersonList[k].strPersonName.c_str() == strPersonName)
		{
			if (strLockInfo == _T("锁定"))
			{
				g_Level4_PersonList[k].bShiftLocked = true;
			}
			else
			{
				g_Level4_PersonList[k].bShiftLocked = false;
			}
			break;
		}
	}

	for (int k = 0; k < g_AllLevel_PersonList.size(); k++)
	{
		if (g_AllLevel_PersonList[k].strPersonName.c_str() == strPersonName)
		{
			if (strLockInfo == _T("锁定"))
			{
				g_AllLevel_PersonList[k].bShiftLocked = true;
			}
			else
			{
				g_AllLevel_PersonList[k].bShiftLocked = false;
			}
			break;
		}
	}
}

void CPersonRestDlg::ReadPersonRestFromCSV()
{


	//从csv文件中读取人员年假剩余天数信息
	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员假期信息.csv";
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}
	char chLine[512];

	fgets(chLine, 512, fp); //跳过第一行表头
	int iRowCount = 1;

	//g_mapPersonRestDayNum.clear();
	
	while (fgets(chLine, 512, fp) != NULL)
	{
		//解析每一行数据
		char* token;
		token = strtok(chLine, ",");
		int iColCount = 0;
		CString strPersonName;
		CString strRestDays;
		CString strHasUsedDay;
		CString strLockInfo;
		PersonRestDay thePersonRestDay;
		while (token != NULL)
		{
			CString strToken(token);
			if (iColCount == 2)
			{
				strPersonName = strToken;
			}
			else if (iColCount == 3)
			{
				strRestDays = strToken;

				//将人员年假剩余天数信息保存到map中
				thePersonRestDay.iAllRestDay = _ttoi(strRestDays);

				map<CString, PersonRestDay>::iterator iter =  g_mapPersonRestDayNum.find(strPersonName);
				if (iter == g_mapPersonRestDayNum.end())
				{
					g_mapPersonRestDayNum[strPersonName] = thePersonRestDay;
				}
				else
				{
					iter->second.iAllRestDay = thePersonRestDay.iAllRestDay;
				}

			}
			else if (iColCount == 4)
			{
				strHasUsedDay = strToken;
				//thePersonRestDay.iHasUsedDay = _ttoi(strHasUsedDay);
				thePersonRestDay.iHasUsedDay = 0;
				//g_mapPersonRestDayNum[strPersonName] = thePersonRestDay;
			}
			else if (iColCount == 5)
			{
				strLockInfo = strToken;
				//Lock信息更新到所有人员列表中
				UpDateLockInfoToPersonList(strLockInfo, strPersonName);
			}

			token = strtok(NULL, ",");
			iColCount += 1;
		}
		//在grid中找到对应人员，设置其年假剩余天数
		for (int col = 1; col < m_pGrid_Person.GetRowCount(); col++)
		{
			CString strGridPersonName = m_pGrid_Person.GetItemText(col, 2);
			if (strGridPersonName == strPersonName)
			{
				m_pGrid_Person.SetItemText(col, 3, strRestDays);
				//m_pGrid_Person.SetItemText(col, 4, strHasUsedDay);
				break;
			}
		}

		if (!strLockInfo.IsEmpty())
		{
			for (int col = 1; col < m_pGrid_Person.GetRowCount(); col++)
			{
				CString strGridPersonName = m_pGrid_Person.GetItemText(col, 2);
				if (strGridPersonName == strPersonName)
				{
					m_pGrid_Person.SetItemText(col, 5, strLockInfo);
					//COLORREF clr_text = G_Text_Defalut_COLORREF;
					COLORREF clr_BK = G_Bk_Defalut_COLORREF;
					CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid_Person.GetCell(col, 5);
					if (strLockInfo == _T("锁定"))
					{
						clr_BK = G_Bk_ReturnClass_COLORREF;
						
						pCell->SetBackClr(clr_BK);
					}
					else
					{
						pCell->SetBackClr(clr_BK);
					}
					break;
				}
			}
		}

		iRowCount += 1;
	}
}

void CPersonRestDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}

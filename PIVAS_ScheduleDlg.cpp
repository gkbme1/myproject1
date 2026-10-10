
// PIVAS_ScheduleDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "PIVAS_Schedule.h"
#include "PIVAS_ScheduleDlg.h"
#include "afxdialogex.h"

#include "string.h"
#include <vector>
#include <map>
using namespace std;
#include <iostream>

#include "NewCellTypes/GridURLCell.h"
#include "NewCellTypes/GridCellCombo.h"
#include "NewCellTypes/GridCellCheck.h"
#include "NewCellTypes/GridCellNumeric.h"
#include "NewCellTypes/GridCellDateTime.h"

#include "CRuleManageDlg.h"
#include "CPersonRestDlg.h"

#include "SkinH.h"
#pragma comment(lib, "SkinH.lib")

#include "CDlgInsertOnePerson.h"

#include "excel.h"
using namespace excel9;

#include "CDiagRetunClassSelect.h"
#include "CActiveSoftwareDlg.h"

#include "TakeLongHoliday.h"
#include <fstream>


#include "CHoliDayRestDlg.h"
#include "CModifyPersonCanWorkClass.h"

#include "CAdjustRestSwitchClass.h"


//排班第一天的时间
time_t g_tStartTime = 0; 
vector<CString> g_vectorTableHeadInfo1; //表头信息
vector<CString> g_vectorTableHeadInfo2; //表头信息

map<int, int> g_mapClassIndexNeedStatistic; //班次索引到班次名称映射

std::map<CString, int> g_ModfifyResttMap;
std::map<CString, int> g_ModfifyResttMap_Ex;

std::map<int,CString > g_BuHoliDayMap;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SOFTWARE_NAME_DRUG "DRUG"
#define SOFTWARE_VERSION_DRUG "V2.0.0"

int main_table[quantity][MaxPersonNum][grade];					//储存quantity张排班表

int main_table_temp[quantity][MaxPersonNum][grade];					//储存quantity张排班表

int main_table_init[quantity][MaxPersonNum][grade];					//储存quantity张排班表

//撤销恢复表格
int main_table_undo[MaxUndoNum][MaxPersonNum][grade];					//储存quantity张排班表


//人员假期undo表
int g_PersonRest_undo[MaxUndoNum];

//自定义班次(补端毛、Z破之类)的undo表。
//这些班次只存在 g_mapAllUserDefineClass 里，不在 main_table 中，
//不单独快照的话，批量删除自定义班次之后就再也撤销不回来了。
map<CString, CString> g_UserDefineClass_undo[MaxUndoNum];

//模板里写死的"休"只存在 g_mapAllRestClass 里，同样不在 main_table 中，
//不单独快照的话，万能格改成"休"之后就再也撤销不回来了。
map<CString, int> g_RestClass_undo[MaxUndoNum];

//本轮"成功安排调休"的格子底色——淡蓝色(原上班 -> 现调休)
#define G_Bk_RelaxArranged_COLORREF		RGB(189, 215, 238)

//本轮"被调出来顶班"的格子底色——淡黄色(原调休 -> 现上班)
#define G_Bk_TakeOverWork_COLORREF		RGB(255, 235, 156)

//和导入的原始排班表不一样的格子，用红色字体标出来
#define G_Text_Changed_COLORREF			RGB(255, 0, 0)

//软规则(只提示、不拦截)踩线之后要提示的文字
CString g_strSoftRuleWarn;

//调休成功的格子用蓝色字体(客户口径：调休=蓝字，被顶班人班次变化=红字)
#define G_Text_RelaxArranged_COLORREF	RGB(0, 0, 255)

//本轮操作过的格子留的浅灰底，便于回溯改过哪些地方
#define G_Bk_Operated_COLORREF			RGB(233, 233, 233)

//软规则提示在链式方案里用这个前缀做标记，区分"硬违规"和"只是提示"
#define SoftRuleTipPrefix				_T("[提示]")

//本轮操作过的格子("人员索引-天索引")，只用来画底色，不参与任何排班计算
map<CString, int> g_mapOperatedCells;

//人员特例规则：某些人有自己的规矩（比如只上 QA、不上 T 班、不参与自动联动）。
//这些规矩写在 exe 旁边的"人员特例规则.csv"里，不写死在代码里，
//以后换人、改规矩改文件就行。文件格式（第一行是表头，用逗号分隔）：
//	姓名,不参与自动联动(1=是),只允许班次(用|分隔、空=不限),禁止班次(用|分隔)
//	方海利,1,QA,
//	代安然,0,,T
struct PersonSpecialRule
{
	bool bNotJoinAutoAdjust;		//不参与自动联动/轮班
	vector<CString> vecOnlyClass;	//只允许上这几个班次（空=不限）
	vector<CString> vecBanClass;	//禁止上这几个班次
	PersonSpecialRule()
	{
		bNotJoinAutoAdjust = false;
	}
};
map<CString, PersonSpecialRule> g_mapPersonSpecialRule;


int g_undo_index = -1; //撤销索引
int g_undo_AllNum = 0; //当前所有步数

map<UINT, CString> g_myClassVector;


//星期名称
wchar_t WeekDayName[7][50] =
{ _T("一"), _T("二"), _T("三"), _T("四"), _T("五"), _T("六"), _T("日"),};

//班次名称
char ClassName[OneDayClasssNum][50] =
{ "2(1)", "2(2)", "2(3)", "2(4)", "2(5)", "2(6)", "3", "3(10)", "3 *", "3 * -1", "3 - 1", "3 - 2", "3 - 3", "3 - 4", "3 - 5", "3 - 6", "3 - 7", "4 - 1", "4 - 2", "4 - 3",
"4 - 4","A","B" ,"C" ,"D" ,"T" ,"Z" ,"帮夜" ,"夜1" , "夜2","QA", //定义班次名称 补休，调休，休
"调休"};

int g_arrGeneral_1Classes[NormalClasss1Num] = { Class_2_3,Class_2_4,Class_2_5,Class_2_6,Class_3_1, Class_3_2, Class_3_3,Class_3 };



//班次类型
ClassType AllClassType[OneDayClasssNum] =
{
	Noon,Noon,Noon,Noon,Noon,Noon,Moning,Moning,Moning,Moning,Moning,Moning,Moning,Moning,Moning,Noon,Moning,Noon,Noon,Noon,
	Noon,Noon,Noon,Noon,Noon,Noon,Noon,Night,Night,Night,
	//数组长度是 OneDayClasssNum(32)，原来只写了30项，QA 和 调休 两位默认成了 Moning(0)，
	//早晚班连班规则会把它们当早班算，这里补齐。
	Noon,Noon
};



//定义4个等级人员能上班的班次(0为不可上，大于0为该班次可上，值为该班次产出，标准为10)
int NurseCanWorkClassesLevel[nurse_level][OneDayClasssNum] =
{
	//等级1：不能上A，B，C，D，T，Z，4 - 4，共27人
	{ 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,00,00,00,00,00,00,00,10,10,10,0,10},
	//等级2：不能上A，T	共7人
	{ 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,00,10,10,10,00,10,00,10,00,0,10},
	//等级3：不能上3(10)， A，B，C，D，T，Z，，4-4，2(1)， 2(2)， 4-1，4-2，4-3， 3*，3*-1 共14人
	//QA(倒数第2个)原来是10，改成00：QA 是上一轮表格带进来的外来标记，本系统不排它，
	//配成"能上"会让它出现在级别3的下拉框里，也会让批量删除认不出它。
	{ 00,00,10,10,10,10,10,00,10,10,10,10,10,10,10,10,10,00,00,00,00,00,00,00,00,00,00,10,0,0,00,10},
	//等级4：只能上A，B，C，D，T  共6人
	{ 00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,10,10,10,10,10,00,00,00,00,0,10},
};

//定义每个级别调休优先查找可调人级别的顺序
int PersonAdjustOrder[4][4] = 
{
	{ 1, 3, 2, 4 },
	{ 2,4,1,3 },		//原来写的是 {2,4,1,2}，级别3被漏掉、级别2重复了一次
	{ 3,1,2,4 },
	{ 4,2,1,3 }
};

#define IndexColumn 0		//序号列
#define LevelColumn 1		//级别列
#define PersonName  2		//姓名列



string g_filePath;


//人员列表
vector<PersonBasicInfo> g_Level1_PersonList;
vector<PersonBasicInfo> g_Level2_PersonList;
vector<PersonBasicInfo> g_Level3_PersonList;
vector<PersonBasicInfo> g_Level4_PersonList;


vector<PersonBasicInfo> g_AllLevel_PersonList;

//调休补休管理数据
vector<PersonRestInfo> g_vectorPersonRestInfo;

map<CString, PersonRestDay > g_mapPersonRestDayNum; //人员剩余假期天数

map<CString, int > g_mapPersonIsLock; //人员是否锁定班次


int g_rule_work_5_day = 1;
int g_Night_MorningSwitch = 1;
int g_OneRound_ClassNum = 1;
int g_bAutoSave = 1;
int g_OneRound_A_T_MAX3 = 1;

int g_Bangye_Role = 1;


ModifyClassSettingType g_modifyClassSettingType = NoSaveToNextTemplate; //修改班次设置类型

CDiagRetunClassSelect *g_theDlgRequstRestDay = nullptr;

vector<ReturnClassPersonAndDay> g_vectorReturnClassPersonAndDay;

vector<ReturnClassPersonAndDay> g_vectorPersonCanRest;
//所有休息的班次
map<CString,int> g_mapAllRestClass;


//所有用户自定义的班次
map<CString, CString> g_mapAllUserDefineClass;
_Application g_ExcelApp;



bool g_bHasInitFinish = false; //是否已经完成初始化

CAdjustRestSwitchClass* g_pAdjustRestDlg = nullptr;


void CalcExcelRange(int iRowNum, int iColNum, CComVariant& varBegin, CComVariant& varEnd)
{
	//=============得到Excel 中 cell的指针============	
//开始单元格
	CString str_begin;
	str_begin = _T("A1");
	//结束单元格

	CString str1;

	CString str_end;
	if (iColNum <= 26)
	{
		str1.Format(_T("%c"), 'A' + iColNum - 1);
		str_end.Format(_T("%s%d"), str1, iRowNum);
	}
	else
	{
		int n1, n2;
		n1 = (iColNum) / 26;
		n2 = (iColNum) % 26;

		if (n2 == 0)//能够整除
		{
			if (n1 == 0)
				str1 = "";
			else
				str1.Format(_T("%c"), 'A' + n1 - 2);

			char ch2 = 'Z';
			str_end.Format(_T("%s%c%d"), str1, ch2, iRowNum);
		}
		else
		{
			if (n1 == 0)
				str1 = "";
			else
				str1.Format(_T("%c"), 'A' + n1 - 1);

			char ch2 = 'A' + n2 - 1;
			str_end.Format(_T("%s%c%d"), str1, ch2, iRowNum);
		}
	}

	CComVariant vt1, vt2;
	vt1 = (CComVariant)str_begin;
	vt2 = (CComVariant)str_end;

	varBegin = vt1;
	varEnd = vt2;
}



CString GetPosition(long lRow, long lCol)
{
	if (lCol > 256 || lRow > 65536)
		return _T("");

	CString strCol;
	CString strRow;

	strRow.Format(_T("%u"), lRow);

	if (lCol <= 26)
	{
		strCol.Format(_T("%c"), 64 + (char)lCol);
	}
	else
	{
		CString strTemp;
		strTemp.Format(_T("%c"), 64 + (char)lCol % 26);
		strCol.Format(_T("%c"), 64 + (char)lCol / 26);
		strCol += strTemp;
	}

	return strCol + strRow;
}



void SetBorderAround(int iStartRow, int iStartCol, int iRowNum, int iColNum, _Worksheet* wsMysheet)
{

	Range myallrange;
	CString toCell = _T("");
	CString fromCell = _T("");
	_variant_t  vRange1;   // 设置单元格的线；
	_variant_t  vRange2;
	_variant_t  vRange3;
	_variant_t  vRange4;

	// 设置单元格的线；
	vRange1.vt = VT_I2;
	vRange1.lVal = 1;   // 线的样式：0- no line; 1-solid; 2-big dot;3-small dot;4-dash dot; 5-dash dot dot;
	vRange2.vt = VT_I2;
	vRange2.lVal = 2;  // 线的粗细程度；
	vRange3.vt = VT_I2;
	vRange3.lVal = 1;   // 1-black;2-white;3-red;4-green;5-blue; 6-yellow; 7-pink;8-dark blue;
	vRange4.vt = VT_UI4;
	vRange4.uintVal = RGB(0, 0, 0);  // 我测试后认为，没有实际意义，只有vRange3起作用


	long lEndRow;
	long lEndCol;
	long lStartRow;
	long lStartCol;


	lStartRow = 1;
	lStartCol = 1;
	lEndRow = 1;
	lEndCol = iColNum;
	//画边框
	for (int w = iStartRow; w <= iRowNum; w++)
	{

		lStartRow = w;
		lStartCol = iStartCol;
		lEndRow = w;
		lEndCol = iColNum;
		toCell = GetPosition(lEndRow, lEndCol);
		fromCell = GetPosition(lStartRow, lStartCol);
		myallrange.ReleaseDispatch();
		myallrange.AttachDispatch(wsMysheet->GetRange(COleVariant(fromCell), COleVariant(toCell)));

		myallrange.BorderAround(vRange1, vRange2, vRange3, vRange4);
	}

	//画边框
	for (int w = iStartCol; w <= iColNum; w++)
	{

		//if (w ==2 ||w ==4)
		//{
		//   lStartRow = 2;
		//}
		//else
		//{
  //        lStartRow = 3;
		//}
		lStartRow = iStartRow;
		lStartCol = w;
		lEndRow = iRowNum;
		lEndCol = w;
		toCell = GetPosition(lEndRow, lEndCol);
		fromCell = GetPosition(lStartRow, lStartCol);
		myallrange.ReleaseDispatch();
		myallrange.AttachDispatch(wsMysheet->GetRange(COleVariant(fromCell), COleVariant(toCell)));

		myallrange.BorderAround(vRange1, vRange2, vRange3, vRange4);
	}


	//vRange2.lVal =3;  // 线的粗细程度；
	//for(int i =0;i<shPlanNum_Temp;i++)
	//{

	//	lStartRow = ProductArr[i].shStarRow +1;
	//	lStartCol = 1;
	//	lEndRow = ProductArr[i].shEndRow;
	//	lEndCol = iColNum;
	//	toCell = GetPosition(lEndRow, lEndCol);
	//	fromCell = GetPosition(lStartRow, lStartCol);	
	//	myallrange.ReleaseDispatch();
	//	myallrange.AttachDispatch(wsMysheet->GetRange(COleVariant(fromCell),COleVariant(toCell)));
	//	
 //     myallrange.BorderAround(vRange1,vRange2,vRange3,vRange4);
	//}

	Range range_All;
	range_All.AttachDispatch(wsMysheet->GetColumns(), TRUE);

	range_All.SetColumnWidth(COleVariant((long)10));
	range_All.AutoFit();
}


void AdjustFormat(_Worksheet* wsMysheet, int iStartRow, int iStartCol, int iEndRow, int iEndCol, bool unBold, UINT unSize)
{
	Range myallrange;
	CString toCell = _T("");
	CString fromCell = _T("");


	toCell = GetPosition(iEndRow, iEndCol);
	fromCell = GetPosition(iStartRow, iStartCol);
	myallrange = wsMysheet->GetRange(COleVariant(fromCell), COleVariant(toCell));

	excel9::Font ft;

	////字体作用范围

	ft.AttachDispatch(myallrange.GetFont());

	//ft.SetName(COleVariant(_T("宋体")));	
	//ft.SetSize(COleVariant((long)20));	
	//ft.SetBold(COleVariant((long)1));//粗体	
	//ft.SetColor(COleVariant((long)0xFF0000));//设置字体颜色

	//ft.SetName(COleVariant(pMaterialrequisitionStruct->StrFont));	

	//UINT unSize = pMaterialrequisitionStruct->unSize/10;
	ft.SetSize(COleVariant((long)unSize));

	//粗体 非斜体
	if (unBold)
	{
		ft.SetBold(_variant_t((long)1));    // 1：粗体，0：非粗体
	}
	////非粗非斜体
	//else if(pMaterialrequisitionStruct->unBold == 400)
	//{

	//}
	////非粗 斜体
//      else if(pMaterialrequisitionStruct->unBold == 655)
	  //{
	  //	ft.SetItalic(_variant_t((long)1));//斜体
	  //}
	  ////粗体 斜体
//      else if(pMaterialrequisitionStruct->unBold == 955)
	  //{
	  //	ft.SetBold(_variant_t((long)1));    // 1：粗体，0：非粗体
	  //	ft.SetItalic(_variant_t((long)1));//斜体
	  //}

	  ////下划线
	  //if(pMaterialrequisitionStruct->unUnderline == 1)
	  //{
	  //	ft.SetUnderline(_variant_t((long)2));//下划线
	  //}

	  //ft.SetBold(COleVariant((long)pMaterialrequisitionStruct->Bold));//粗体	

	  //COLORREF ForeColor = CStringToColorrefEX( pMaterialrequisitionStruct->StrFontColor);
	  //ft.SetColor(COleVariant((long)ForeColor));//设置字体颜色



	myallrange.SetHorizontalAlignment(_variant_t((long)-4131)); // -4108:居中，-4131:靠左，-4152:靠右  

	////水平对齐 默认1 居中-4108 左-4131 右-4152
	////垂直对齐 默认2 居中-4108 左-4160 右-4107

	////填充颜色  
	// //COLORREF color= RGB(255, 1, 1);
	// Interior interior;  
	// interior.AttachDispatch(myallrange.GetInterior());  
	// COLORREF BackColor = CStringToColorrefEX( pMaterialrequisitionStruct->StrBackGroundColor);

	// interior.SetColor(_variant_t(BackColor)); //设置背景颜色
	// //double dblVal = interior.GetColor().dblVal;//获取颜色

	// switch(pMaterialrequisitionStruct->EnumAlignment)
	// {
	//	case LEFT:
	//		myallrange.SetHorizontalAlignment(_variant_t((long)-4131)); // -4108:居中，-4131:靠左，-4152:靠右  
	//		break;
	//	case CENTER:
	//		myallrange.SetHorizontalAlignment(_variant_t((long)-4108)); // -4108:居中，-4131:靠左，-4152:靠右  
	//		break;
	//	case RIGHT:
	//		myallrange.SetHorizontalAlignment(_variant_t((long)-4152)); // -4108:居中，-4131:靠左，-4152:靠右  
	//		break;
	//	default:
	//		break;
	// }


}




void MergeRangs(_Worksheet* wsMysheet, int iStartRow, int iStartCol, int iEndRow, int iEndCol)
{
	Range myallrange;
	CString toCell = _T("");
	CString fromCell = _T("");


	//pMaterialrequisitionStruct = (HBDB_RequisitionPrintSetup *)pNeedMergeList->GetNext(pos);
	toCell = GetPosition(iEndRow, iEndCol);
	fromCell = GetPosition(iStartRow, iStartCol);
	myallrange = wsMysheet->GetRange(COleVariant(fromCell), COleVariant(toCell));


	myallrange.Merge(COleVariant((short)0));


	Range range_All;
	range_All.AttachDispatch(wsMysheet->GetColumns(), TRUE);

	range_All.SetColumnWidth(COleVariant((long)10));

	//range_All.AttachDispatch(wsMysheet->GetRows(),TRUE);
 //   range_All.AutoFit(); 
}


void SetBodyAlignment(int iStartRow, int iEndRow, int iStartCol, int iEndCol, _Worksheet* wsMysheet)
{
	Range myallrange;
	CString toCell = _T("");
	CString fromCell = _T("");
	toCell = GetPosition(iEndRow, iEndCol);
	fromCell = GetPosition(iStartRow, iStartCol);
	myallrange = wsMysheet->GetRange(COleVariant(fromCell), COleVariant(toCell));
	//myallrange.SetHorizontalAlignment(COleVariant((short)3));//设置名头水平对齐方式为水平居中
	myallrange.SetHorizontalAlignment(_variant_t((long)-4108)); // -4108:居中，-4131:靠左，-4152:靠右  

}



std::string& CStringtoString(std::string& strDst, const CString& strSrc)
{
	int iLen = 0;
	//获取字节长度    
	iLen = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);
	char *pchParam = NULL;
	pchParam = new char[iLen];
	memset(pchParam, 0, iLen);
	//将tchar值赋给_char   
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, pchParam, iLen, NULL, NULL);
	strDst = std::string(pchParam);
	if (pchParam != NULL)
	{
		delete[]pchParam;
		pchParam = NULL;
	}
	return strDst;
}
void STRING2TCHAR(string strData, wchar_t* pwszString, int bufferlength)
{

	int iLen = MultiByteToWideChar(CP_ACP, 0, strData.c_str(), -1, NULL, 0);

	//wchar_t *pwszString =new wchar_t[iLen];

	int iCopyLenth = bufferlength > iLen ? iLen : bufferlength;
	MultiByteToWideChar(CP_ACP, NULL, strData.c_str(), -1, pwszString, iCopyLenth);

	return;
}

//人员特例规则的两个判断，前面几个函数就要用，实现写在下面
static bool bJudgePersonCanWorkClassBySpecialRule(CString strPersonName, int iClassIndex_OneDay);
static bool bJudgePersonJoinAutoAdjust(CString strPersonName);

//把 csv 里的一行按逗号拆开。
//不能用 strtok：它会把连着的逗号当成一个，"代安然,0,,T" 会被拆成三段，列就错位了。
static void SplitCsvLine(const std::string& strLine, vector<std::string>& vecField)
{
	vecField.clear();

	std::string strOne;
	for (size_t i = 0; i < strLine.size(); i++)
	{
		char ch = strLine[i];
		if (ch == ',')
		{
			vecField.push_back(strOne);
			strOne = "";
		}
		else if (ch != '\r' && ch != '\n')
		{
			strOne += ch;
		}
	}
	vecField.push_back(strOne);
}

//把 "QA|代班" 这种用竖线分隔的班次名拆成一个个 CString
static void SplitClassNameList(const std::string& strData, vector<CString>& vecClassName)
{
	vecClassName.clear();

	std::string strOne;
	for (size_t i = 0; i <= strData.size(); i++)
	{
		char ch = (i < strData.size()) ? strData[i] : '|';
		if (ch == '|')
		{
			if (!strOne.empty())
			{
				TCHAR strOne_T[100] = { 0 };
				STRING2TCHAR(strOne, strOne_T, 100);
				CString strOne_CS = strOne_T;
				strOne_CS.Trim();
				strOne_CS.Replace(_T(" "), _T(""));
				if (!strOne_CS.IsEmpty())
				{
					vecClassName.push_back(strOne_CS);
				}
			}
			strOne = "";
		}
		else if (ch != ' ')
		{
			strOne += ch;
		}
	}
}

//读 exe 旁边的"人员特例规则.csv"。没有这个文件就是没有特例，不影响任何功能。
//格式（第一行表头会被跳过）：
//	姓名,不参与自动联动(1=是),只允许班次(|分隔),禁止班次(|分隔)
//	方海利,1,QA,
//	代安然,0,,T
static void ReadPersonSpecialRuleFromFile()
{
	g_mapPersonSpecialRule.clear();

	std::string filename = g_filePath + "\\人员特例规则.csv";
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		OutputDebugString(_T("[Info] 没有人员特例规则.csv，按没有特例处理。\n"));
		return;
	}

	char chLine[1024] = { 0 };
	int iLineIndex = 0;
	while (fgets(chLine, sizeof(chLine), fp) != NULL)
	{
		iLineIndex++;

		std::string strLine(chLine);
		if (strLine.empty())
		{
			continue;
		}
		if (strLine[0] == '#')
		{
			continue;		//注释行
		}

		vector<std::string> vecField;
		SplitCsvLine(strLine, vecField);
		if (vecField.size() < 2)
		{
			continue;
		}

		TCHAR strName_T[100] = { 0 };
		STRING2TCHAR(vecField[0], strName_T, 100);
		CString strPersonName = strName_T;
		strPersonName.Trim();
		if (strPersonName.IsEmpty())
		{
			continue;
		}

		//第一行如果是表头就跳过
		if (iLineIndex == 1 && strPersonName.Find(_T("姓名")) >= 0)
		{
			continue;
		}

		PersonSpecialRule theRule;
		theRule.bNotJoinAutoAdjust = (atoi(vecField[1].c_str()) > 0);
		if (vecField.size() >= 3)
		{
			SplitClassNameList(vecField[2], theRule.vecOnlyClass);
		}
		if (vecField.size() >= 4)
		{
			SplitClassNameList(vecField[3], theRule.vecBanClass);
		}

		g_mapPersonSpecialRule[strPersonName] = theRule;
	}

	fclose(fp);

	char chDebug[255] = { 0 };
	sprintf(chDebug, "[Info] 人员特例规则.csv 读到 %d 条\n", (int)g_mapPersonSpecialRule.size());
	OutputDebugStringA(chDebug);
}

wstring Utf8ToUnicode(string& strInput)
{
	int iLen = MultiByteToWideChar(CP_UTF8, 0, strInput.c_str(), -1, NULL, 0);
	if (iLen <= 0)
	{
		return L"";
	}
	wchar_t* pwszString = new wchar_t[iLen];
	MultiByteToWideChar(CP_UTF8, 0, strInput.c_str(), -1, pwszString, iLen);
	wstring strOutput(pwszString);
	delete[] pwszString;
	return strOutput;
}


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CPIVASScheduleDlg 对话框

string strMachine;
bool g_bHasAuth = false;

LRESULT CPIVASScheduleDlg::OnRefreshGrid(WPARAM wParam, LPARAM lParam)
{
	// 安全地执行全量刷新
	ShowAllPersonToList(false, false, true);

	// 强制清除可能存在的多余选中区，并重绘
	m_pGrid.SetSelectedRange(-1, -1, -1, -1, FALSE, FALSE);
	m_pGrid.RedrawWindow();

	return 0;
}

CPIVASScheduleDlg::CPIVASScheduleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PIVAS_SCHEDULE_DIALOG, pParent)
{
	m_bIsUpdatingGrid = false;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pDlgRestDayManagement = nullptr; // 初始化休息日管理对话框指针

	m_RemainingTime = 180 * 24 * 60 * 60; //默认15天试用期

	//


	m_pAuthorizeDLL = new CISAuthorizeDLL_DRUGTRACE(SOFTWARE_NAME_DRUG, SOFTWARE_VERSION_DRUG, false, false);
	AuthorityState_DRUGTRACE* m_pAuthorityState;
	strMachine = m_pAuthorizeDLL->GetMachineCode();

	int iRet = m_pAuthorizeDLL->ReadAuthorityState(&m_pAuthorityState);
	if (iRet == 0)
	{
		g_bHasAuth = true;
	}


}

void CPIVASScheduleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_LIST1, m_ListCtrl);
	DDX_Control(pDX, IDC_GRID, m_pGrid);
	DDX_Control(pDX, IDC_EDIT1, m_TraceWnd);
}

BEGIN_MESSAGE_MAP(CPIVASScheduleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CPIVASScheduleDlg::OnBnClickedButton_Next)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_BUTTON2, &CPIVASScheduleDlg::OnOpenRestManageDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_GRID, OnGridDblClick)
	ON_NOTIFY(GVN_ENDLABELEDIT, IDC_GRID, OnGridEndEdit)
	ON_NOTIFY(NM_CLICK, IDC_GRID, OnGridClick)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON3, &CPIVASScheduleDlg::OnBnSavePersonOrder)
	ON_BN_CLICKED(IDC_BUTTON_RULE, &CPIVASScheduleDlg::OnBnClickedButtonRule)
	ON_NOTIFY(NM_RCLICK, IDC_GRID, OnGridRClick)
	ON_COMMAND(ID_DBEDIT_INSERT_ONELINE, &CPIVASScheduleDlg::OnDBEditorInsert)


	ON_BN_CLICKED(IDC_BUTTON5, &CPIVASScheduleDlg::OnBnClickedButton_Save)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT_EXCEL, &CPIVASScheduleDlg::OnBnClickedButtonExportExcel)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_OPER_CANCLE, &CPIVASScheduleDlg::OnBnClickedButton_Undo)
	ON_BN_CLICKED(IDC_BUTTON_OPER_RESUME, &CPIVASScheduleDlg::OnBnClickedButton_Redo)
	
	ON_BN_CLICKED(ID_BUTTON_AutoAdjust, &CPIVASScheduleDlg::OnBnClickedButton_AutoAdjust)
	ON_BN_CLICKED(ID_BUTTON_RequestRest, &CPIVASScheduleDlg::OnBnClickedButton_RequestRest)
	ON_BN_CLICKED(ID_BUTTON_ExportExcel, &CPIVASScheduleDlg::OnBnClickedButton_ExportExcel)
	ON_BN_CLICKED(ID_BUTTON_COMPARE, &CPIVASScheduleDlg::OnBnClickedButton_Compare)
	ON_BN_CLICKED(ID_BUTTON_IMPORTTEMP, &CPIVASScheduleDlg::OnBnClickedButton_Import_Temp)


	ON_MESSAGE(IDM_MYICONNOTIFY, OnMyIconNotify)							//托盘图标响应

	ON_COMMAND(IDM_QUITENGINE, QuitEngine)									//退出服务器
	ON_COMMAND(IDM_ACTIVESOFTWARE, ActiveSoftware)								//打开关于框

	ON_WM_CLOSE()

	ON_COMMAND(ID_BUTTON_Previous, &CPIVASScheduleDlg::OnBnClickedButton_Previous)
	ON_COMMAND(ID_BUTTON_Next, &CPIVASScheduleDlg::OnBnClickedButton_Next)
	ON_COMMAND(ID_BUTTON_Save, &CPIVASScheduleDlg::OnBnClickedButton_Save)
	ON_COMMAND(ID_BUTTON_Undo, &CPIVASScheduleDlg::OnBnClickedButton_Undo)
	ON_COMMAND(ID_BUTTON_Redo, &CPIVASScheduleDlg::OnBnClickedButton_Redo)
	ON_COMMAND(ID_BUTTON_History, &CPIVASScheduleDlg::OnBnClickedButton_History)
	ON_COMMAND(ID_BUTTON_ModifyCanWorkClass, &CPIVASScheduleDlg::OnBnClickedButton_ModifyCanWorkClass)
	ON_COMMAND(ID_BUTTON_SaveTemplate, &CPIVASScheduleDlg::OnBnClickedButton_SaveTemplate)
	ON_COMMAND(ID_BUTTON_PersonRest, &CPIVASScheduleDlg::OnBnClickedButton_PersonRest)
	ON_MESSAGE(WM_REFRESH_GRID, &CPIVASScheduleDlg::OnRefreshGrid)


END_MESSAGE_MAP()





//查找班次索引
int CPIVASScheduleDlg::FindeClassIndex(CString stData)
{
	for (int i = 0; i < OneDayClasssNum; i++)
	{
		TCHAR tchClassName[50] = { 0 };
		STRING2TCHAR(ClassName[i], tchClassName, 50);
		CString strClassName;
		//CString strClassName_temp;
		strClassName = tchClassName;
		strClassName.Trim();
		stData.Trim();
		strClassName.Replace(_T(" "), _T(""));
		stData.Replace(_T(" "), _T(""));
		stData.Replace(_T("—"), _T("-"));
		if (stData == strClassName)
		{
			return i;
		}
	}
	if (stData == _T(" "))
	{
		return -6; //休息班次
	}

	std::map<CString, int>::iterator iter1 = g_ModfifyResttMap.find(stData);

	if (iter1!= g_ModfifyResttMap.end())
	{
		return -10; //补假期的调休
	}


	iter1 = g_ModfifyResttMap_Ex.find(stData);
	if (iter1 != g_ModfifyResttMap_Ex.end())
	{
		return -11; //补假期的调休(修改)
	}

	if (stData == _T("休"))
	{
		return -1; //休息班次
	}
	else if (stData == _T("调休"))
	{
		return -2; //调休班次
	}
	else if (stData.Find(_T("调休(调整)")) >= 0)
	{
		return -7; //调休调整
	}
	else if (stData.Find(_T("补休(调整)")) >= 0)
	{
		return -8; //补休调整
	}
	else if (stData.Find( _T("调休")) >=0)
	{
		return -2; //调休班次
	}
	else if (stData.Find(_T("补休")) >= 0)
	{
		return -2; //调休班次
	}
	else if (stData.Find(_T("视频")) >= 0)
	{
		return -3; //调休班次
	}
	else if (stData.Find(_T("教育")) >= 0)
	{
		return -3; //调休班次
	}
	else if (stData.Find(_T("调半")) >= 0)
	{
		return -3; //调休班次半天
	}
	else
	{
		return -6; //其他班次
	}
	return -5; //未找到
		
}


//根据姓名查找级别
int CPIVASScheduleDlg::FindPesrsonLevelByName(CString strPersonName)
{
	for (int i = 0; i < g_Level1_PersonList.size(); i++)
	{
		if (g_Level1_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return 1; //级别1
		}
	}
	for (int i = 0; i < g_Level2_PersonList.size(); i++)
	{
		if (g_Level2_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return 2; //级别2
		}
	}
	for (int i = 0; i < g_Level3_PersonList.size(); i++)
	{
		if (g_Level3_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return 3; //级别3
		}
	}
	for (int i = 0; i < g_Level4_PersonList.size(); i++)
	{
		if (g_Level4_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return 4; //级别4
		}
	}
	return 0; //未找到级别

}


int CPIVASScheduleDlg::FindPesrsonIndexByName(CString strPersonName)
{
	int iIndex = 0;
	for (int i = 0; i < g_Level1_PersonList.size(); i++)
	{
		if (g_Level1_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return iIndex;
		}
		iIndex++;
	}
	for (int i = 0; i < g_Level2_PersonList.size(); i++)
	{
		if (g_Level2_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return iIndex;
		}
		iIndex++;
	}
	for (int i = 0; i < g_Level3_PersonList.size(); i++)
	{
		if (g_Level3_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return iIndex; //级别3
		}
		iIndex++;
	}
	for (int i = 0; i < g_Level4_PersonList.size(); i++)
	{
		if (g_Level4_PersonList[i].strPersonName.c_str() == strPersonName)
		{
			return iIndex; //级别4
		}
		iIndex++;
	}
	return -1; //未找到级别
}

int CPIVASScheduleDlg::GetPersonOneDayWorkClassIndex(int iPersonIndex, int iDayIndex)
{
	if (iPersonIndex<0)
	{
		return -1; //无效人员索引
	}
	int iClassIndex_OneDay = -1;
	for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum; iClassIndex++)
	{
		if (main_table[0][iPersonIndex][iClassIndex] != 0)
		{
			iClassIndex_OneDay = iClassIndex % OneDayClasssNum;
			break;
		}
	}

	return iClassIndex_OneDay;
}


int CPIVASScheduleDlg::GetThisDayRestPersons(int iDayIndex, vector<int>& NoWorkPersonList, bool bCanFindHasScheduled)
{
	int iAllPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();

	for (int iPersonIndex = 0; iPersonIndex < iAllPersonNum; iPersonIndex++)
	{
		bool bHasWork = false;
		for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum-1; iClassIndex++)
		{
			if (main_table[0][iPersonIndex][iClassIndex] != 0)
			{
				bHasWork = true;
				break;
			}
		}
		if (!bHasWork)
		{
			//不上班，且班次为调休
			if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == Relax_INIT ||
				main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] >= BU_HOLIDAY_1
				|| bCanFindHasScheduled && main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == WORK_TO_Relax
				|| bCanFindHasScheduled && main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == Relax_TO_WORK
				|| bCanFindHasScheduled && main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == REST_INIT
				|| bCanFindHasScheduled && main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == WORK_TO_REST_INIT)
			{
				NoWorkPersonList.push_back(iPersonIndex);
			}
			
		}

	}

	return 1;
}


int CPIVASScheduleDlg::GetThisDayRelaxToWorkPersons(int iDayIndex, vector<int>& RelaxToWorkPersonList)
{
	int iAllPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();

	for (int iPersonIndex = 0; iPersonIndex < iAllPersonNum; iPersonIndex++)
	{
		if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == Relax_TO_WORK)
		{
			RelaxToWorkPersonList.push_back(iPersonIndex);
		}

	}

	return 1;
}



int CPIVASScheduleDlg::GetThisDayCanWorPersons(int iDayIndex, int iClass_OneDay, vector<int>& NoWorkPersonList, vector<int>& NoWorkPersonCanWorkInThisClass)
{
	int iAllPesonNum = NoWorkPersonList.size();
	
	for (int k = 0; k < iAllPesonNum; k++)
	{
		int iPersonIndex = NoWorkPersonList[k];
		CString strPersonName;

		strPersonName = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();;

		//获取级别
		int iPersonLevel = FindPesrsonLevelByName(strPersonName);
		if (iPersonLevel <= 0 || iPersonLevel > nurse_level)
		{
			continue;
		}

		//人员特例规则：不参与自动联动的人、或者规则里禁了这个班次的人，不当候选
		if (!bJudgePersonJoinAutoAdjust(strPersonName))
		{
			continue;
		}
		if (!bJudgePersonCanWorkClassBySpecialRule(strPersonName, iClass_OneDay % OneDayClasssNum))
		{
			continue;
		}

		if (NurseCanWorkClassesLevel[iPersonLevel-1][iClass_OneDay] > 0)
		{
			NoWorkPersonCanWorkInThisClass.push_back(iPersonIndex);
		}
	}
	return 1;
}


CString CPIVASScheduleDlg::GetClassNameOneDayOnePerson(int iDayIndex, int iPersonIndex, int& iClassIndexOneDay,int& iClass_ChangeRelaxValue)
{
	CString strClassName_temp;
	int iClass_OneDay = -1;
	iClassIndexOneDay = -1;
	iClass_OneDay = GetPersonOneDayWorkClassIndex(iPersonIndex, iDayIndex);
	if (iClass_OneDay >= 0)
	{
		TCHAR strClassName[100] = { 0 };
		STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
		strClassName_temp = strClassName;
		iClassIndexOneDay = iClass_OneDay;
	}
	if (iPersonIndex >=0)
	{
		iClass_ChangeRelaxValue = main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax];

		//补五一、补端午这类名字只有在当天确实没上班的时候才拿来当格子内容。
		//原来是无条件覆盖：万一某个格子既排了真实班次、又残留着调休标记，
		//界面就只显示"补五一"，把真正上的班盖掉了，看着像操作没生效。
		bool bHasRealWorkClass = (iClass_OneDay >= 0 && iClass_OneDay != Class_ChangeRelax);

		if (iClass_ChangeRelaxValue >= BU_HOLIDAY_1 && !bHasRealWorkClass)
		{
			std::map<int, CString >::iterator  iter_temp2 = g_BuHoliDayMap.find(iClass_ChangeRelaxValue);
			if (iter_temp2 != g_BuHoliDayMap.end())
			{
				strClassName_temp = iter_temp2->second;
			}
		}
	}

	return strClassName_temp;
}

bool CPIVASScheduleDlg::JudgeCanSwitchClassByTwoPerson(int iPersonIndex_A, int iDayIndex_A, int iClassIndex_OneDay_A, int iPersonLevel_A,
	int iPersonIndex_B, int iDayIndex_B, int iClassIndex_OneDay_B, int iPersonLevel_B, ClassCheckErroType& iErrorType)
{
	bool bCheckOk = true;
	//判断A能不能上B的班次，B能不能上A的班次
	if (NurseCanWorkClassesLevel[iPersonLevel_A-1][iClassIndex_OneDay_B] > 0 &&
		NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndex_OneDay_A] > 0)
	{
	}
	else
	{
		bCheckOk = false;
		iErrorType = Class_NotMatch;
		return bCheckOk;
	}

	ChangeCurrentSheduleToTempTable();
	//对调俩人的班次

	main_table_temp[0][iPersonIndex_A][iDayIndex_A * OneDayClasssNum + iClassIndex_OneDay_A] = 0;
	main_table_temp[0][iPersonIndex_A][iDayIndex_B * OneDayClasssNum + iClassIndex_OneDay_B] = 2;

	main_table_temp[0][iPersonIndex_A][iDayIndex_B * OneDayClasssNum + iClassIndex_OneDay_B] = 0;
	main_table_temp[0][iPersonIndex_A][iDayIndex_A * OneDayClasssNum + iClassIndex_OneDay_A] = 2;
	ClassCheckErroType iErrorType_A = HasNO_ERROR;
	CString strErroDes_A;
	bool bCheckA = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex_A, iClassIndex_OneDay_A, iErrorType_A, strErroDes_A);
	ClassCheckErroType iErrorType_B = HasNO_ERROR;
	CString strErroDes_B;
	bool bCheckB = bCheckOnePersonOneDayClassIsOk(iPersonIndex_B, iDayIndex_B, iClassIndex_OneDay_B, iErrorType_B, strErroDes_B);
	if (!bCheckA || !bCheckB)
	{
		bCheckOk = false;
		if (iErrorType_A != HasNO_ERROR)
		{
			iErrorType = iErrorType_A;
		}
		else if (iErrorType_B != HasNO_ERROR)
		{
			iErrorType = iErrorType_B;
		}
		else
		{
			iErrorType = Class_NotMatch; //默认错误类型
		}

		main_table_temp[0][iPersonIndex_A][iDayIndex_A * OneDayClasssNum + iClassIndex_OneDay_A] = 1;
		main_table_temp[0][iPersonIndex_A][iDayIndex_B * OneDayClasssNum + iClassIndex_OneDay_B] = 0;

		main_table_temp[0][iPersonIndex_A][iDayIndex_B * OneDayClasssNum + iClassIndex_OneDay_B] = 1;
		main_table_temp[0][iPersonIndex_A][iDayIndex_A * OneDayClasssNum + iClassIndex_OneDay_A] = 0;


		ChangeTempSheduleToCurrentTable();


		return bCheckOk;
	}
	else
	{
		//ChangeTempSheduleToCurrentTable();
		bCheckOk = true;
		return bCheckOk;
	}

}



//获取某天上指定班次的人
void CPIVASScheduleDlg::GetPersonOneDayWorkInOneClase(int iDayIndex, int iClass_OneDay, int& iPersonIndex)
{
	int iAllPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();
	iPersonIndex = -1; //默认未找到
	for (int i = 0; i < iAllPersonNum; i++)
	{
		if (main_table[0][i][iDayIndex * OneDayClasssNum + iClass_OneDay] != 0)
		{
			iPersonIndex = i;
			return;
		}
	}
	return;
}

bool CPIVASScheduleDlg::bJudgeCanRestByClassIndex(int iClassIndex)
{
	//判断班次是否可以调休
	if (iClassIndex < 0)
	{
		return true; //休息班次不能调休
	}
	else if (iClassIndex == 0 || iClassIndex == 1 || iClassIndex == 2 || iClassIndex == 3 || iClassIndex == 4 || iClassIndex == 5)
	{
		return true; //2(1),2(2),2(3),2(4),2(5),2(6)可以调休
	}
	else if (iClassIndex == Class_3_6 || iClassIndex == Class_3_10 || iClassIndex == Class_ye_1 || iClassIndex == Class_ye_2 || iClassIndex == Class_bang_ye)
	{
		return false;// false;
	}
	else
	{
		return true; //其他班次不能调休
	}
	//else if (iClassIndex >= 6 && iClassIndex <= 17)
	//{
	//	return false; //3,3(10),3*,3*-1,3-1,3-2,3-3,3-4,3-5,3-6,3-7,4-1,4-2,4-3,4-4,A,B,C,D,T,Z，帮夜，夜1，夜2不能调休
	//}
	//else
	//{
	//	return false; //其他班次不能调休
	//}
}

//和"导入的原始排班表"(g_myClassVector)比对时用的 key。
//原来是 行*14+列 —— 步长14比实际列数还小，列一多就会和下一行撞车；
//更要命的是填表用 (i,j)、显示用 (行+1,列)、UpdateGridColorsOnly 用 (行,列) 三套算法，
//同一个格子算出来的 key 都不一样，底色自然乱跳。现在三处统一走这个函数。
static UINT MakeInitTableKey(int iGridRow, int iGridCol)
{
	return (UINT)(iGridRow * 100 + iGridCol);
}

//这个格子和导入时的原始排班表比，内容变了没有
static bool bJudgeCellChangedFromInitTable(int iGridRow, int iGridCol, CString strCurrentText)
{
	//原始表没读进来(或者只读到一星半点)就不做比对，免得整张表全标红
	if (g_myClassVector.size() <= 200)
	{
		return false;
	}

	map<UINT, CString>::iterator iter = g_myClassVector.find(MakeInitTableKey(iGridRow, iGridCol));
	if (iter == g_myClassVector.end())
	{
		return false;		//原始表里没有这个格子(比如后加的人员)，不算"变了"
	}

	CString strInitText = iter->second;
	strInitText.Replace(_T(" "), _T(""));
	strInitText.Replace(_T("—"), _T("-"));
	strCurrentText.Replace(_T(" "), _T(""));
	strCurrentText.Replace(_T("—"), _T("-"));

	//两边都是调休类：只有"调整"与否不一样才算变了
	if (strInitText.Find(_T("调休")) >= 0 && strCurrentText.Find(_T("调休")) >= 0)
	{
		bool bInitIsModify = (strInitText.Find(_T("调整")) >= 0);
		bool bNowIsModify = (strCurrentText.Find(_T("调整")) >= 0);
		return (bInitIsModify != bNowIsModify);
	}

	return (strInitText != strCurrentText);
}

//软规则(只提示不拦截)踩线之后的提示文字，由 bCheckOnePersonOneDayClassIsOk 填，手工改完班之后取走
static void ShowSoftRuleWarnIfAny(CWnd* pWnd, CString strPersonName)
{
	if (g_strSoftRuleWarn.IsEmpty() || pWnd == nullptr)
	{
		return;
	}

	CString strWarnMsg;
	strWarnMsg.Format(_T("提示：%s 的排班踩到了下面的限制，但不影响本次修改：\n\n%s"), strPersonName, g_strSoftRuleWarn);
	g_strSoftRuleWarn.Empty();
	pWnd->MessageBox(strWarnMsg, _T("规则校验提示"), MB_OK | MB_ICONINFORMATION);
}

//这个格子本轮有没有被动过(只影响底色，不参与任何排班计算)
static bool bJudgeCellHasOperated(int iPersonIndex, int iDayIndex)
{
	if (g_mapOperatedCells.empty())
	{
		return false;
	}

	CString strKey;
	strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
	return (g_mapOperatedCells.find(strKey) != g_mapOperatedCells.end());
}

//某个班次每天最多排几个人，返回 -1 表示不限。
//客户口径：3-1、3-3、3(10) 每天2人，3 班每天1~3人。
//这里只管上限（人手不够不弹窗，否则排班过程中一直被打断）。
static int GetClassDayMaxPersonNum(int iClassIndex_OneDay)
{
	if (iClassIndex_OneDay == Class_3_1 || iClassIndex_OneDay == Class_3_3 || iClassIndex_OneDay == Class_3_10)
	{
		return 2;
	}
	if (iClassIndex_OneDay == Class_3)
	{
		return 3;
	}
	return -1;
}

//当天排了这个班次的人数
static int GetClassDayPersonNum(int iDayIndex, int iClassIndex_OneDay)
{
	if (iDayIndex < 0 || iDayIndex >= AllDay || iClassIndex_OneDay < 0 || iClassIndex_OneDay >= OneDayClasssNum - 1)
	{
		return 0;
	}

	int iNum = 0;
	for (int iPersonIndex = 0; iPersonIndex < (int)g_AllLevel_PersonList.size(); iPersonIndex++)
	{
		if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndex_OneDay] != 0)
		{
			iNum++;
		}
	}
	return iNum;
}

//把出人数超标的提示挂到软规则提示上，和 A/T 那套一起弹给客户。
//只在客户把人排进这个班次的时候调，所以导入表本来就超标的旧数据不会反复弹窗。
static void AppendClassDayNumWarn(int iDayIndex, int iClassIndex_OneDay)
{
	int iMaxNum = GetClassDayMaxPersonNum(iClassIndex_OneDay);
	if (iMaxNum <= 0)
	{
		return;
	}

	int iNowNum = GetClassDayPersonNum(iDayIndex, iClassIndex_OneDay);
	if (iNowNum <= iMaxNum)
	{
		return;
	}

	TCHAR strClassName_temp[100] = { 0 };
	STRING2TCHAR(ClassName[iClassIndex_OneDay], strClassName_temp, 100);

	CString strOneDes;
	strOneDes.Format(_T("第%d天 %s 班每天最多%d人，现在有%d人"), iDayIndex + 1, strClassName_temp, iMaxNum, iNowNum);

	if (g_strSoftRuleWarn.IsEmpty())
	{
		g_strSoftRuleWarn = strOneDes;
	}
	else
	{
		g_strSoftRuleWarn += _T("；");
		g_strSoftRuleWarn += strOneDes;
	}
}

//这个人能不能上这个班次（只看人员特例规则，级别权限另外判）
static bool bJudgePersonCanWorkClassBySpecialRule(CString strPersonName, int iClassIndex_OneDay)
{
	if (g_mapPersonSpecialRule.empty())
	{
		return true;
	}
	if (iClassIndex_OneDay < 0 || iClassIndex_OneDay >= OneDayClasssNum - 1)
	{
		return true;
	}

	map<CString, PersonSpecialRule>::iterator iter = g_mapPersonSpecialRule.find(strPersonName);
	if (iter == g_mapPersonSpecialRule.end())
	{
		return true;
	}

	TCHAR strClassName_temp[100] = { 0 };
	STRING2TCHAR(ClassName[iClassIndex_OneDay], strClassName_temp, 100);
	CString strClassName_Now = strClassName_temp;
	strClassName_Now.Replace(_T(" "), _T(""));

	PersonSpecialRule& theRule = iter->second;

	//白名单：只要配了，就只能上这几个班
	if (!theRule.vecOnlyClass.empty())
	{
		bool bInOnly = false;
		for (int k = 0; k < (int)theRule.vecOnlyClass.size(); k++)
		{
			if (theRule.vecOnlyClass[k] == strClassName_Now)
			{
				bInOnly = true;
				break;
			}
		}
		if (!bInOnly)
		{
			return false;
		}
	}

	//黑名单
	for (int k = 0; k < (int)theRule.vecBanClass.size(); k++)
	{
		if (theRule.vecBanClass[k] == strClassName_Now)
		{
			return false;
		}
	}

	return true;
}

//这个人参不参与自动联动/换班查找
static bool bJudgePersonJoinAutoAdjust(CString strPersonName)
{
	if (g_mapPersonSpecialRule.empty())
	{
		return true;
	}

	map<CString, PersonSpecialRule>::iterator iter = g_mapPersonSpecialRule.find(strPersonName);
	if (iter == g_mapPersonSpecialRule.end())
	{
		return true;
	}
	return (!iter->second.bNotJoinAutoAdjust);
}

//把软规则(A/T数量这类只提示不拦截的规则)的提示，拼成方案列表里显示的一句话
static CString MakeSoftWarnDesc(CString strPersonName, CString strSoftWarn)
{
	if (strSoftWarn.IsEmpty())
	{
		return CString(_T(""));
	}

	CString strDes;
	strDes.Format(_T("%s：%s"), strPersonName, strSoftWarn);
	return strDes;
}




//解析某行列数据
void CPIVASScheduleDlg::ParseOneRangeData(int iRowIndex, int iCowIndex, CString stData,int &iPersonLevel)
{

	switch (iCowIndex)
	{
		//级别
		case LevelColumn:
		{
			if (stData == _T("级别1"))
			{
				iPersonLevel = 1;
			}
			else if (stData == _T("级别2"))
			{
				iPersonLevel = 2;
			}
			else if (stData == _T("级别3"))
			{
				iPersonLevel = 3;
			}
			else if (stData == _T("级别4"))
			{
				iPersonLevel = 4;
			}
			else
			{
				iPersonLevel = 0;
			}
			break;
		}
		//姓名
		case PersonName:
		{

			PersonBasicInfo thePersonBasicInfo;
			thePersonBasicInfo.strPersonName = stData.GetString();
			if (iPersonLevel == 1)
			{
				g_Level1_PersonList.push_back(thePersonBasicInfo);
				g_AllLevel_PersonList.push_back(thePersonBasicInfo);

			}
			else if (iPersonLevel == 2)
			{
				g_Level2_PersonList.push_back(thePersonBasicInfo);
				g_AllLevel_PersonList.push_back(thePersonBasicInfo);
			}
			else if (iPersonLevel == 3)
			{
				g_Level3_PersonList.push_back(thePersonBasicInfo);
				g_AllLevel_PersonList.push_back(thePersonBasicInfo);
			}
			else if (iPersonLevel == 4)
			{
				g_Level4_PersonList.push_back(thePersonBasicInfo);
				g_AllLevel_PersonList.push_back(thePersonBasicInfo);
			}

			break;
		}
		default:
		{

			break;
		}
	}


	if (iCowIndex > PersonName && iRowIndex >= 1)
	{
		int iDayIndex = iCowIndex - PersonName-1; //从第3列开始是日期
		// ====== 必须新增的越界保护 ======
		if (iDayIndex >= AllDay)
		{
			return; // 忽略右侧附加的“姓名”和“剩余假期”列
		}

		//查看当前班次属于哪个班
		int iClassIndex = FindeClassIndex(stData);
		//将班次存入表中
		if (iClassIndex >= 0)
		{
			main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = 1;
		}
		else
		{
			if (iClassIndex == -1)
			{
				CString strKey;
				strKey.Format(_T("%d-%d"), iRowIndex - 1, iDayIndex);
				g_mapAllRestClass[strKey] = 1;
			}
			else if (iClassIndex == -2)
			{
				iClassIndex = Class_ChangeRelax;
				main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = Relax_INIT;
			}
			else if (iClassIndex == -3)
			{
				iClassIndex = Class_ChangeRelax;
				main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = Relax_INIT;
			}
			else if (iClassIndex == -7)
			{
				iClassIndex = Class_ChangeRelax;
				main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = WORK_TO_Relax;
			}
			else if (iClassIndex == -8)
			{
				iClassIndex = Class_ChangeRelax;
				main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = WORK_TO_REST_INIT;
			}
			else if (iClassIndex == -6)
			{
				CString strKey;
				strKey.Format(_T("%d-%d"), iRowIndex - 1, iDayIndex);
				g_mapAllUserDefineClass[strKey] = stData;				
			}
			else if (iClassIndex == -10)
			{
				iClassIndex = Class_ChangeRelax;

				std::map<CString, int>::iterator iter1 = g_ModfifyResttMap.find(stData);

				if (iter1 != g_ModfifyResttMap.end())
				{
					int iRelaxType = iter1->second;
					if (iRelaxType <= BU_HOLIDAY_10)
					{
						main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = iRelaxType;

					}
				}
			}
			else if (iClassIndex == -11)
			{
				iClassIndex = Class_ChangeRelax;

				std::map<CString, int>::iterator iter1 = g_ModfifyResttMap_Ex.find(stData);

				if (iter1 != g_ModfifyResttMap_Ex.end())
				{
					int iRelaxType = iter1->second;
					if (iRelaxType <= BU_HOLIDAY_10)
					{
						main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = iRelaxType+ BU_HOLIDAY_1_WORK_TO_Relax- BU_HOLIDAY_1;

					}
				}
			}
		}
	}

}

void CPIVASScheduleDlg::ParseOneRangeData_FromInitFile(int iRowIndex, int iCowIndex, CString stData, int& iPersonLevel)
{
	switch (iCowIndex)
	{
		//级别
	case LevelColumn:
	{
		if (stData == _T("级别1"))
		{
			iPersonLevel = 1;
		}
		else if (stData == _T("级别2"))
		{
			iPersonLevel = 2;
		}
		else if (stData == _T("级别3"))
		{
			iPersonLevel = 3;
		}
		else if (stData == _T("级别4"))
		{
			iPersonLevel = 4;
		}
		else
		{
			iPersonLevel = 0;
		}
		break;
	}
	//姓名
	case PersonName:
	{

		//PersonBasicInfo thePersonBasicInfo;
		//thePersonBasicInfo.strPersonName = stData.GetString();
		//if (iPersonLevel == 1)
		//{
		//	g_Level1_PersonList.push_back(thePersonBasicInfo);
		//	g_AllLevel_PersonList.push_back(thePersonBasicInfo);

		//}
		//else if (iPersonLevel == 2)
		//{
		//	g_Level2_PersonList.push_back(thePersonBasicInfo);
		//	g_AllLevel_PersonList.push_back(thePersonBasicInfo);
		//}
		//else if (iPersonLevel == 3)
		//{
		//	g_Level3_PersonList.push_back(thePersonBasicInfo);
		//	g_AllLevel_PersonList.push_back(thePersonBasicInfo);
		//}
		//else if (iPersonLevel == 4)
		//{
		//	g_Level4_PersonList.push_back(thePersonBasicInfo);
		//	g_AllLevel_PersonList.push_back(thePersonBasicInfo);
		//}

		break;
	}
	default:
	{

		break;
	}
	}


	if (iCowIndex > PersonName && iRowIndex >= 1)
	{
		int iDayIndex = iCowIndex - PersonName - 1; //从第3列开始是日期
		// ====== 必须新增的越界保护 ======
		if (iDayIndex >= AllDay)
		{
			return; // 忽略右侧附加的列
		}
		//查看当前班次属于哪个班
		int iClassIndex = FindeClassIndex(stData);
		//将班次存入表中
		if (iClassIndex >= 0)
		{
			main_table_init[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = 1;
		}
		else
		{
			if (iClassIndex == -1)
			{
				CString strKey;
				strKey.Format(_T("%d-%d"), iRowIndex - 1, iDayIndex);
				//g_mapAllRestClass[strKey] = 1;
			}
			else if (iClassIndex == -2)
			{
				iClassIndex = Class_ChangeRelax;
				main_table_init[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = Relax_INIT;
			}
			else if (iClassIndex == -3)
			{
				iClassIndex = Class_ChangeRelax;
				main_table_init[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = Relax_INIT;
			}
			else if (iClassIndex == -6)
			{
				//CString strKey;
				//strKey.Format(_T("%d-%d"), iRowIndex - 1, iDayIndex);
				//g_mapAllUserDefineClass[strKey] = stData;
			}
		}
	}
}



void CPIVASScheduleDlg::ReadRuleConfigFromIni()
{
	//读取规则配置文件
	TCHAR strFileName[100] = { 0 };
	STRING2TCHAR(g_filePath, strFileName, 100);

	CString strFilePath_temp = strFileName;
	strFilePath_temp += _T("\\RuleConfig.ini");
	//读取规则配置文件
	g_rule_work_5_day = GetPrivateProfileInt(_T("RuleConfig"), _T("work_5_day"), 0, strFilePath_temp);
	g_Night_MorningSwitch = GetPrivateProfileInt(_T("RuleConfig"), _T("Night_MorningSwitch"), 0, strFilePath_temp);
	g_OneRound_ClassNum = GetPrivateProfileInt(_T("RuleConfig"), _T("OneRound_ClassNum"), 0, strFilePath_temp);
	g_bAutoSave = GetPrivateProfileInt(_T("RuleConfig"), _T("AutoSave"), 0, strFilePath_temp);
	g_modifyClassSettingType = (ModifyClassSettingType)GetPrivateProfileInt(_T("RuleConfig"), _T("ModifyClassSetting"), 0, strFilePath_temp);

}


void CPIVASScheduleDlg::MergeCells(int nStartRow, int nStartCol, int nEndRow, int nEndCol)
{
	for (int row = nStartRow; row <= nEndRow; row++)
	{
		for (int col = nStartCol; col <= nEndCol; col++)
		{
			CGridCellBase* pCell = (CGridCellBase*)m_pGrid.GetCell(row, col);
			
			pCell->Show(false);
			if (row == nStartRow && col == nStartCol)
			{
				CCellRange range(nStartRow, nStartCol, nEndRow, nEndCol);
				pCell->SetMergeRange(range);
			}
			else
			{
				CCellID cell(nStartRow, nStartCol);
				pCell->SetMergeCellID(cell);
			}

		}
	}
	m_pGrid.Invalidate();
}


//从最初始的文件中解析休息的班次
void CPIVASScheduleDlg::ReadInitialSchedulFromModuleFile_FromInitFile()
{
	FILE* fp;
	string filename = g_filePath;
	filename += "\\模板.csv";
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}



	char buf[100000];
	memset(buf, 0, 100000);
	fread(buf, sizeof(char), 100000, fp);
	fclose(fp);
	char* pBuf = buf;
	char* pTemp = NULL;
	pTemp = strtok(pBuf, ",");

	int iLineIndex = 0;

	bool bHasNextLine = false;
	int iColumnIndex = 0;
	int iPersonLevel = 0;

	vector<wstring> vListCtrlHeadData;

	while (pTemp != NULL)
	{
		int iClassIndex = 0;
		int iRotationIndex = 0;
		string strTemp = pTemp;

		if (strTemp.find("\n") == 0)
		{
			pTemp = pTemp + 1;
			bHasNextLine = true;
			continue;
		}
		if (strTemp.find("\r") == 0)
		{
			pTemp = pTemp + 1;
			bHasNextLine = true;
			continue;
		}
		if (bHasNextLine)
		{
			bHasNextLine = false;
			iLineIndex++;
			//m_ListCtrl.InsertItem(iLineIndex, _T(""));

			iColumnIndex = 0;
		}
		if (iLineIndex == 0)
		{
			wstring strData;
			//strData = Utf8ToUnicode(strTemp);
			wchar_t wsData[100];
			STRING2TCHAR(strTemp, wsData, 100);
			strData = wsData;

			//m_ListCtrl.InsertColumn(iColumnIndex+1, strData.c_str(), LVCFMT_CENTER, 100);

			vListCtrlHeadData.push_back(strData);
			ParseOneRangeData_FromInitFile(iLineIndex, iColumnIndex, strData.c_str(), iPersonLevel);
			iColumnIndex++;
		}
		else
		{
			wstring strData;
			wchar_t wsData[100];
			//strData = Utf8ToUnicode(strTemp);
			STRING2TCHAR(strTemp, wsData, 100);
			strData = wsData;

			//m_ListCtrl.SetItemText(iLineIndex-1, iColumnIndex, strData.c_str());
			ParseOneRangeData_FromInitFile(iLineIndex, iColumnIndex, strData.c_str(), iPersonLevel);
			iColumnIndex++;
		}


		pTemp = strtok(NULL, ",");

	}
}


void CPIVASScheduleDlg::ReadStartTimeFromFile()
{

	time_t tCurrent = time(NULL);

	TCHAR strFileName[100] = { 0 };
	STRING2TCHAR(g_filePath, strFileName, 100);
	CString strFilePath_temp = strFileName;
	strFilePath_temp += _T("\\排班起始时间.ini");

	TCHAR tcStartTime[50] = { 0 };
	GetPrivateProfileString(_T("Schedule_STARTTIME"), _T("Init_StartTime"), _T(""), tcStartTime, 50, strFilePath_temp);


	CTime cCurrentTime = tCurrent;


	//10.20解析月和日
	CString strStartTime = tcStartTime;
	int iMonth = 0;
	int iDay = 0;
	int iPos = strStartTime.Find(L".");
	if (iPos > 0)
	{
		iMonth = _ttoi(strStartTime.Left(iPos));
		iDay = _ttoi(strStartTime.Right(strStartTime.GetLength() - iPos - 1));

		int year = cCurrentTime.GetYear();
		int month = iMonth;
		int day = iDay;
		CTime temp(year, month, day, 0, 0, 0);
		tCurrent = temp.GetTime();
	}



	tm* ltm = localtime(&tCurrent);
	int weekday = ltm->tm_wday;

	int iNeedAddDays = 0; //需要增加的天数
	if (weekday == 0) //如果是星期天，则设置为星期一
	{
		iNeedAddDays = 1;
	}
	else if (weekday == 1)
	{
		iNeedAddDays = 0;
	}
	else
	{
		iNeedAddDays = 7 - weekday + 1;
	}

	g_tStartTime = tCurrent + 24 * 60 * 60 * iNeedAddDays; //设置为下一个星期一的0点
}

void CPIVASScheduleDlg::ReadInitialXlsxToCsv(CString strXlsxPatch,map<UINT, CString>& myClassVector, bool bIsImport)
{
	// TODO: Add extra validation here	
	_Application ExcelApp;
	Workbooks wbsMyBooks;
	_Workbook wbMyBook;
	Worksheets wssMysheets;
	_Worksheet wsMysheet;
	Range rgMyRge;

	_variant_t var;
	LPDISPATCH pbook;


	ExcelApp.m_bAutoRelease = FALSE;
	if (!ExcelApp.CreateDispatch(_T("Excel.Application"), NULL))
	{
		CString szErr = _T("");//g_strEXCELCREATFAIL;
		AfxMessageBox(szErr, MB_OK | MB_ICONHAND);
		return ;
	}

	wbsMyBooks.AttachDispatch(ExcelApp.GetWorkbooks(), true);
	//pbook = ExcelApp.GetActiveWorkbook();//如果当前有打开的Excel文档，返回非空。

	pbook = wbsMyBooks.Open((LPCTSTR)strXlsxPatch);
	wbMyBook.AttachDispatch(pbook, true);
	wssMysheets.AttachDispatch(wbMyBook.GetWorksheets(), true);
	int count = wssMysheets.GetCount();
	CString szSheetName;
	int iSheetCount = 0;
	int iSheetCount1 = 0;




	FILE* fp;
	string filename = g_filePath;
	filename += "\\模板_Last.csv";
	fp = fopen(filename.c_str(), "w");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}

	char* chData = new char[200000];
	memset(chData, 0, 200000);
	char* chData_temp = chData;

	//注意："六15"和"日14"之间那一格写的是一个空格而不是空字符串，它是一个真实存在的列，
	//表头一共18个字段，界面列数(SetColumnCount)依赖这个数字，不要随手删掉。
	sprintf(chData, "序号, 级别, 人员\\日期, 一14, 二15, 三15, 四15, 五15, 六15, , 日14, 一14, 二15, 三15, 四15, 五15, 六15, 日14,\n");
	chData += strlen(chData);


	for (int kk = 1; kk <= count; kk++)
	{
		wsMysheet.AttachDispatch(wssMysheets.GetItem(_variant_t(kk)), true);

		szSheetName = wsMysheet.GetName();
		rgMyRge.AttachDispatch(wsMysheet.GetUsedRange(), true);
		LPDISPATCH pr1, pr2;
		pr1 = rgMyRge.GetColumns();
		pr2 = rgMyRge.GetRows();
		Range range2, range3;
		range2.AttachDispatch(pr1, true);
		range3.AttachDispatch(pr2, true);
		int iColNum_temp = range2.GetCount();
		int iRowNum_temp = range3.GetCount();
		VARIANT RangeValueDataType;
		CComVariant AllValue = rgMyRge.GetValue2();
		CComVariant* pPropVals = NULL;
		SafeArrayAccessData(AllValue.parray, (void**)&pPropVals);

		//一维的是行数
		long lLB1, lUB1;
		SafeArrayGetLBound(AllValue.parray, 1, &lLB1);
		SafeArrayGetUBound(AllValue.parray, 1, &lUB1);

		//二维的是列数
		long lLB2, lUB2;
		SafeArrayGetLBound(AllValue.parray, 2, &lLB2);
		SafeArrayGetUBound(AllValue.parray, 2, &lUB2);
		int iIndex_temp = 3;
		int iColNum = lUB2 - lLB2 + 1;//行数
		int iRowNum = lUB1 - lLB1 + 1;//列数
		int iIndex = 0;
		int iLevel = 1;
		for (int i = 0; i < iRowNum; i++)
		{

			if (i == 1 && bIsImport)
			{
				int iColIndex = lLB2 + 1;
				_variant_t varItem;
				varItem = pPropVals[(iColIndex - 1) * iRowNum + i];


				CString strTemp;

				switch (varItem.vt)
				{
				case VT_BSTR:
					strTemp = varItem.bstrVal;
					break;
				case VT_R8: // double
					strTemp.Format(_T("%g"), varItem.dblVal);
					break;
				default:
					strTemp = _T("");
					break;
				}

				time_t tCurrent = time(NULL);
				CTime cCurrentTime = tCurrent;
				//10.20解析月和日
				CString strStartTime = strTemp;
				int iMonth = 0;
				int iDay = 0;
				int iPos = strStartTime.Find(L".");
				if (iPos > 0)
				{
					iMonth = _ttoi(strStartTime.Left(iPos));
					iDay = _ttoi(strStartTime.Right(strStartTime.GetLength() - iPos - 1));

					int year = cCurrentTime.GetYear();
					int month = iMonth;
					int day = iDay;
					CTime temp(year, month, day, 0, 0, 0);
					tCurrent = temp.GetTime();
				}
				g_tStartTime = tCurrent;

				TCHAR strFileName[100] = { 0 };
				STRING2TCHAR(g_filePath, strFileName, 100);
				CString strFilePath_temp = strFileName;
				strFilePath_temp += _T("\\排班起始时间.ini");

				WritePrivateProfileString(_T("Schedule_STARTTIME"), _T("Init_StartTime"), strStartTime, strFilePath_temp);
				WritePrivateProfileString(_T("Schedule_STARTTIME"), _T("Last_StartTime"), strStartTime, strFilePath_temp);
			}
			if (i >= 3)
			{
				iIndex_temp++;
				char chIndex[255] = { 0 };
				sprintf(chIndex, "%d", iIndex_temp);
				strcat(chData, chIndex);
				strcat(chData, ",");
				for (int j = lLB2; j <= lUB2; j++)
				{
					_variant_t varItem;
					varItem = pPropVals[(j-1)*iRowNum + i];


					CString strTemp;

					switch (varItem.vt)
					{
					case VT_BSTR:
						strTemp = varItem.bstrVal;
						break;
					case VT_R8: // double
						strTemp.Format(_T("%g"), varItem.dblVal);
						break;
					case VT_I4: // int
						strTemp.Format(_T("%d"), varItem.lVal);
						break;
					case VT_EMPTY:
					case VT_NULL:
						strTemp = _T("");
						break;
						// 其他类型可按需补充
					default:
						strTemp = _T("");
						break;
					}
					
					
					if (j == 1)
					{
						LPDISPATCH pDispCell = rgMyRge.GetCells(i + 1, j);
						excel9::Range singleCell;
						singleCell.AttachDispatch(pDispCell);

						// 2. 获取该单元格的 Font 对象
						LPDISPATCH pDispFont = singleCell.GetFont();
						excel9::Font cellFont;
						cellFont.AttachDispatch(pDispFont);

						// 3. 读取颜色
						VARIANT varColor = cellFont.GetColor();
						long fontColorVal = 0;

						if (varColor.vt == VT_R8) {
							fontColorVal = (long)varColor.dblVal; // Excel 有时会以 double 返回颜色值
						}
						else if (varColor.vt == VT_I4) {
							fontColorVal = varColor.lVal;
						}

						if (fontColorVal == 255 && iLevel == 1)
						{
							iLevel = 2;
						}

						if (fontColorVal > 255 && iLevel == 2)//10498160
						{
							iLevel = 3;
						}
						if (fontColorVal <  255 && iLevel == 3)//10498160
						{
							iLevel = 4;
						}

						char chLevel[255] = { 0 };
						sprintf(chLevel, "级别%d", iLevel);
						strcat(chData, chLevel);
						strcat(chData, ",");

						// 【修改点：在此处显式释放对象】
						cellFont.ReleaseDispatch();
						singleCell.ReleaseDispatch();
					}








					//string strOneCellData;
					//CStringtoString(strOneCellData, strTemp);
					//strcat(chData, strOneCellData.c_str());
					//strcat(chData, ",");
					if (j >= 2)
					{
						//表格坐标：excel 第 i 行对应表格第 i-1 行，第 j 列对应表格第 j+1 列
						UINT unIndex = MakeInitTableKey(i - 1, j + 1);
						myClassVector[unIndex] = strTemp;

						char chDebug[255] = { 0 };
						string strClassName;
						CStringtoString(strClassName, strTemp);
						if (i >= 3 && i <= 5)
						{
							sprintf(chDebug, "gao0505 %d-%d className:%s", i, j, strClassName.c_str());
							OutputDebugStringA(chDebug);
						}
					}





					//CString strTemp;
					//strTemp = varItem.bstrVal;
					//处理数据

					string strOneCellData;
					CStringtoString(strOneCellData, strTemp);
					strcat(chData, strOneCellData.c_str());
					strcat(chData, ",");

					char chDebug[255] = { 0 };
					sprintf(chDebug, "i=%d, j=%d, data=%s\n", i, j, strOneCellData.c_str());
					OutputDebugStringA(chDebug);
				}

				strcat(chData, "\n");
				chData += strlen(chData);
			}

		}
		::SafeArrayUnaccessData(AllValue.parray);

		//导出数据到csv文件

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



	wbMyBook.Close(true);

	//导出时如果有打开的EXCEL，会报错，所以将以下两句注释去掉  2012.2.15
	wbsMyBooks.Close();//lj.note 不知道为什么要关闭所有文档。暂时先注释，不然导出数据库后会关掉先前打开的Excel文档	

	//释放对象 
	rgMyRge.ReleaseDispatch();
	wsMysheet.ReleaseDispatch();
	wssMysheets.ReleaseDispatch();

	wbMyBook.ReleaseDispatch();
	wbsMyBooks.ReleaseDispatch();
	
	ExcelApp.Quit();
	ExcelApp.ReleaseDispatch();

	
}



//获取初始的排班模板
void CPIVASScheduleDlg::ReadInitialSchedulFromModuleFile(string strAllPatch, bool bIsImport)
{
	//m_ListCtrl.SetExtendedStyle(m_ListCtrl.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_AUTOAUTOARRANGE );
	//获取当前路径
	//m_pGrid.SetEditable(true);
	//m_pGrid.SetTextBkColor(RGB(0xFF, 0xFF, 0xE0));//黄色背景
	
	CString strXlsxfilename;
	//CString strXlsxfilename_Last;
	if (!strAllPatch.empty())
	{		
		TCHAR tchXlsxfilename[300] = { 0 };
		STRING2TCHAR(strAllPatch, tchXlsxfilename, 300);
		strXlsxfilename = tchXlsxfilename;
	}
	else
	{



		TCHAR tchXlsxfilename[300] = { 0 };
		STRING2TCHAR(g_filePath, tchXlsxfilename, 300);

		strXlsxfilename = tchXlsxfilename;
		strXlsxfilename += _T("\\模板_Last.xlsx");

		//查看该文件是否存在，如果存在则读取该文件，如果不存在则读取模板.xlsx
		if (_taccess(strXlsxfilename, 0) != 0)
		{
			strXlsxfilename = tchXlsxfilename;
			strXlsxfilename += _T("\\模板.xlsx");
		}


	}

	map<UINT, CString> myClassVector;
	ReadInitialXlsxToCsv(strXlsxfilename, myClassVector, bIsImport);


	FILE* fp;
	string filename;
	//if (!strAllPatch.empty())
	//{
	//	filename = strAllPatch;
	//}
	//else
	//{
		filename = g_filePath;
		filename += "\\模板_Last.csv";
	//}

	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{

		filename = g_filePath;
		filename += "\\模板.csv";
		fp = fopen(filename.c_str(), "r");
		if (fp == NULL)
		{
			cout << "无法打开文件" << endl;
			return;
		}
	}
	else
	{
		time_t tCurrent = time(NULL);

		TCHAR strFileName[100] = { 0 };
		STRING2TCHAR(g_filePath, strFileName, 100);
		CString strFilePath_temp = strFileName;
		strFilePath_temp += _T("\\排班起始时间.ini");

		TCHAR tcStartTime[50] = { 0 };
		GetPrivateProfileString(_T("Schedule_STARTTIME"), _T("Last_StartTime"), _T(""), tcStartTime, 50, strFilePath_temp);


		CTime cCurrentTime = tCurrent;


		//10.20解析月和日
		CString strStartTime = tcStartTime;
		int iMonth = 0;
		int iDay = 0;
		int iPos = strStartTime.Find(L".");
		if (iPos > 0)
		{
			iMonth = _ttoi(strStartTime.Left(iPos));
			iDay = _ttoi(strStartTime.Right(strStartTime.GetLength() - iPos - 1));

			int year = cCurrentTime.GetYear();
			int month = iMonth;
			int day = iDay;
			CTime temp(year, month, day, 0, 0, 0);
			tCurrent = temp.GetTime();
		}



		tm* ltm = localtime(&tCurrent);
		int weekday = ltm->tm_wday;

		int iNeedAddDays = 0; //需要增加的天数
		if (weekday == 0) //如果是星期天，则设置为星期一
		{
			iNeedAddDays = 1;
		}
		else if (weekday == 1)
		{
			iNeedAddDays = 0;
		}
		else
		{
			iNeedAddDays = 7 - weekday + 1;
		}

		g_tStartTime = tCurrent + 24 * 60 * 60 * iNeedAddDays; //设置为下一个星期一的0点
	}

	


	//------------------------------------------------------------------
	// 按行、按逗号逐个切分 CSV。
	//
	// 原先这里用 strtok(buf, ",")，而 strtok 会把连续的分隔符合并成一个，
	// 于是 CSV 里的空单元格(",,")被整个跳过，该行后面所有格子集体左移一格，
	// 最后一天的班次直接丢失 —— 这就是"导入排班表丢格子内容"的根源。
	// "删除班次"留下的空白格、以及原始表格里本来就空着的格子都会踩中。
	// 现在自己按逗号切，空字段照样占一格；行尾那个逗号不再多产生一列。
	//------------------------------------------------------------------
	char buf[100000];
	memset(buf, 0, sizeof(buf));
	//留最后一个字节做结束符，文件超长时也不会读出个没有结尾的字符串
	fread(buf, sizeof(char), sizeof(buf) - 1, fp);
	fclose(fp);

	int iLineIndex = 0;
	int iPersonLevel = 0;

	vector<wstring> vListCtrlHeadData;

	char* pLineStart = buf;
	while (*pLineStart != '\0')
	{
		//先定位本行的结尾
		char* pLineEnd = pLineStart;
		while (*pLineEnd != '\0' && *pLineEnd != '\n' && *pLineEnd != '\r')
		{
			pLineEnd++;
		}

		char chSaveLineEnd = *pLineEnd;
		*pLineEnd = '\0';

		//每行末尾都带一个逗号，去掉它，免得凭空多出一个空列
		if (pLineEnd > pLineStart && *(pLineEnd - 1) == ',')
		{
			*(pLineEnd - 1) = '\0';
		}

		if (*pLineStart != '\0')
		{
			int iColumnIndex = 0;
			char* pFieldStart = pLineStart;
			while (true)
			{
				char* pComma = strchr(pFieldStart, ',');
				if (pComma != NULL)
				{
					*pComma = '\0';
				}

				string strTemp = pFieldStart;

				wstring strData;
				wchar_t wsData[100] = { 0 };
				STRING2TCHAR(strTemp, wsData, 100);
				strData = wsData;

				if (iLineIndex == 0)
				{
					vListCtrlHeadData.push_back(strData);
				}
				ParseOneRangeData(iLineIndex, iColumnIndex, strData.c_str(), iPersonLevel);
				iColumnIndex++;

				if (pComma == NULL)
				{
					break;
				}
				pFieldStart = pComma + 1;
			}

			iLineIndex++;
		}

		if (chSaveLineEnd == '\0')
		{
			break;
		}

		//跳过换行符，兼容 \n 与 \r\n
		pLineEnd++;
		while (*pLineEnd == '\r' || *pLineEnd == '\n')
		{
			pLineEnd++;
		}
		pLineStart = pLineEnd;
	}
	//CCellRange Selection(2, 2, -1, -1);

	//if (Selection.GetColSpan() <= 1 && Selection.GetRowSpan() <= 1)
	//	return;






	//读取保存的人员顺序
	ReadPersonOrderFromFile();


	CPersonRestDlg thePersonRestDlg;
	thePersonRestDlg.ReadPersonRestFromCSV();

	int iPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size()+ g_Level4_PersonList.size();


	m_pGrid.SetEditable(true);
	//m_pGrid.SetTextBkColor(RGB(0xFF, 0xFF, 0xE0));//黄色背景
	m_pGrid.SetTextBkColor(RGB(255, 255, 255)); // 统一底层背景色为白色

	// ====== 新增：设置 Excel 样式的网格线 ======
	m_pGrid.SetGridLines(GVL_BOTH);               // 显示水平和垂直网格线
	m_pGrid.SetGridLineColor(RGB(200, 200, 200)); // Excel 经典的浅灰色网格线
	// ===========================================


	m_pGrid.SetRowCount(iPersonNum+2); //初始为10行
	m_pGrid.SetColumnCount(vListCtrlHeadData.size()+2); //初始化为11列
	m_pGrid.SetFixedRowCount(2); //表头为一行
	m_pGrid.SetFixedColumnCount(1); //表头为一列

	//将解析的数据显示到ListCtrl中
	for (int i = 0; i < vListCtrlHeadData.size(); i++)
	{
		wstring strData = vListCtrlHeadData[i];
		int iWithdth = 100; //默认宽度
		if (i == 0 || i == 1)
		{
			iWithdth = 50; //序号和级别列宽度
		}
		//m_ListCtrl.InsertColumn(i + 1, strData.c_str(), LVCFMT_CENTER, iWithdth);

		//m_pGrid.InsertColumn(strData.c_str(), LVCFMT_CENTER, iWithdth);

		m_pGrid.SetRowHeight(0, 25); //设置各行高          
		m_pGrid.SetColumnWidth(i, iWithdth); //设置0列宽 


		//设置表格显示属性
		GV_ITEM Item;
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = 0;
		Item.col = i;

		Item.nFormat = DT_CENTER | DT_WORDBREAK;
		Item.strText = strData.c_str();

		m_pGrid.SetItem(&Item);



	}

	//m_ListCtrl.SetBkColor(RGB(255, 0, 0)); // 例如，设置为红色


	iLineIndex = 0;
	for (int m = 0; m < iPersonNum; m++)
	{
		iLineIndex++;
		//m_ListCtrl.InsertItem(iLineIndex, _T(""));
	}

	ShowAllPersonToList(true,true,true);


	MergeCells(0, 0, 1, 0);
	MergeCells(0, 1, 1, 1);
	MergeCells(0, 2, 1, 2);


	// ====== 新增右侧表头合并 ======
	MergeCells(0, 3 + AllDay, 1, 3 + AllDay);
	MergeCells(0, 3 + AllDay + 1, 1, 3 + AllDay + 1);
}


void CPIVASScheduleDlg::ReadLastRestDayDataFromFile()
{
	//读取上轮的调休补休结余数据
	FILE* fp;
	string filename = g_filePath;
	filename += "\\上轮调休补休结余.csv";
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}
	char buf[100000];
	memset(buf, 0, 100000);
	fread(buf, sizeof(char), 100000, fp);
	fclose(fp);
	char* pBuf = buf;
	char* pTemp = NULL;
	pTemp = strtok(pBuf, ",");
	int iLineIndex = 0;
	int iColumnIndex = 0;
	bool bHasNextLine = false;
	
	CString strPersonName;
	int iPersonLevel = 0; //级别
	CString strRestType;
	int iDayNumber = 0;
	while (pTemp != NULL)
	{
		string strTemp = pTemp;
		if (strTemp.find("\n") == 0)
		{
			bHasNextLine = true;
			pTemp = pTemp + 1;
			continue;
		}
		if (strTemp.find("\r") == 0)
		{
			bHasNextLine = true;
			pTemp = pTemp + 1;
			continue;
		}
		if (bHasNextLine)
		{
			bHasNextLine = false;
			iLineIndex++;
			iColumnIndex = 0;
		}

		int iPersonLevel = 0;
		CString strData = Utf8ToUnicode(strTemp).c_str();
		if (iColumnIndex == 0)
		{
			strPersonName = strData;

		}
		else if (iColumnIndex == 1)
		{
			strRestType = strData; //休息类型
		}
		else if (iColumnIndex == 2)
		{
			iDayNumber = _ttoi(strData); 
			//将数据存入到对应的人员信息中
			PutPersonLastRestInfoToVect(strPersonName, strRestType,iDayNumber);
		}

		iColumnIndex++;

		pTemp = strtok(NULL, ",");
	}
}


void CPIVASScheduleDlg::ReadPersonOrderFromFile()
{
	//读取上轮的调休补休结余数据
	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员顺序.csv";
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}
	char buf[100000];
	memset(buf, 0, 100000);
	fread(buf, sizeof(char), 100000, fp);
	fclose(fp);
	char* pBuf = buf;

	int iDataLen = strlen(pBuf);
	char* pTemp = NULL;
	pTemp = strtok(pBuf, ",");
	int iLineIndex = 0;
	int iColumnIndex = 0;
	bool bHasNextLine = false;

	int iPersonLevel = 0;

	vector<PersonBasicInfo> vecPersonLevel1; //存储人员顺序列表
	vector<PersonBasicInfo> vecPersonLevel2; //存储人员顺序列表
	vector<PersonBasicInfo> vecPersonLevel3; //存储人员顺序列表
	vector<PersonBasicInfo> vecPersonLevel4; //存储人员顺序列表

	CString strPersonLevel;
	CString strPersonName;
	CString strRestType;
	int iDayNumber = 0;
	while (pTemp != NULL)
	{
		string strTemp = pTemp;
		if (strTemp.find("\n") == 0)
		{
			bHasNextLine = true;
			pTemp = pTemp + 1;
			iPersonLevel = 0;
			continue;
		}
		if (strTemp.find("\r") == 0)
		{
			bHasNextLine = true;
			pTemp = pTemp + 1;
			iPersonLevel = 0;
			continue;
		}
		if (bHasNextLine)
		{
			bHasNextLine = false;
			iLineIndex++;
			iColumnIndex = 0;
		}

		int iOffset = pTemp - pBuf;

		TCHAR strData_temp[100] = { 0 };
		STRING2TCHAR(strTemp, strData_temp, 100);
		CString strData = strData_temp;

		if (iColumnIndex == 1)
		{
			strPersonLevel = strData;
			iPersonLevel = _ttoi(strPersonLevel);
			if (iPersonLevel == 0)
			{
				break;
			}
		}
		else if (iColumnIndex == 2)
		{
			strPersonName = strData; 
			PersonBasicInfo thePersonBasicInfo;
			thePersonBasicInfo.strPersonName = strData.GetString();
			if (iPersonLevel == 1)
			{
				vecPersonLevel1.push_back(thePersonBasicInfo); //存储级别1人员
			}
			else if (iPersonLevel == 2)
			{
				vecPersonLevel2.push_back(thePersonBasicInfo); //存储级别2人员
			}
			else if (iPersonLevel == 3)
			{
				vecPersonLevel3.push_back(thePersonBasicInfo); //存储级别3人员
			}
			else if (iPersonLevel == 4)
			{
				vecPersonLevel4.push_back(thePersonBasicInfo); //存储级别3人员
			}
			else
			{
				//其他级别不处理
				break;
			}
		}

		iColumnIndex++;

		pTemp = strtok(NULL, ",");
	}

	if (g_Level1_PersonList.size() == vecPersonLevel1.size() && 
		g_Level2_PersonList.size() == vecPersonLevel2.size() &&
		g_Level3_PersonList.size() == vecPersonLevel3.size()&&
		g_Level4_PersonList.size() == vecPersonLevel4.size())
	{
		//则将文件中的人员替换到全部变量中

		g_Level1_PersonList = vecPersonLevel1;
		g_Level2_PersonList = vecPersonLevel2;
		g_Level3_PersonList = vecPersonLevel3;

		g_Level4_PersonList = vecPersonLevel4;
		//对三个级别增加3个预留的人员




		g_AllLevel_PersonList.clear();
		g_AllLevel_PersonList = vecPersonLevel1;
		for (int k = 0; k < vecPersonLevel2.size(); k++)
		{
			g_AllLevel_PersonList.push_back(vecPersonLevel2[k]);
		}

		for (int k = 0; k < vecPersonLevel3.size(); k++)
		{
			g_AllLevel_PersonList.push_back(vecPersonLevel3[k]);
		}

		for (int k = 0; k < vecPersonLevel4.size(); k++)
		{
			g_AllLevel_PersonList.push_back(vecPersonLevel4[k]);
		}
		ShowAllPersonToList(false, true, false);

	}

}


void CPIVASScheduleDlg::ShowAllPersonToList(bool bRefreshIndex_Level, bool bRefreshPerson, bool bRefreshClassInfo)
{
	m_bIsUpdatingGrid = true;
	int iPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();
	m_pGrid.SetRowCount(iPersonNum + 2); //初始为10行

	//人数一多(50多行)就有客户反映"显示不全"，这里改完行数强制重算一次滚动条，
	//保证最后几行滚得到。
	m_pGrid.ResetScrollBars();
	//遍历所有人，查看其每天的班次
	//m_ListCtrl.DeleteAllItems(); //清空列表控件
	int iLineIndex = 1;
	int iColumnIndex = 0;


	//更新表头数据 从第0行第3列开始更新时间
	//更新表头数据 从第1行第3列开始，星期几+班次统计
	g_vectorTableHeadInfo1.clear();
	g_vectorTableHeadInfo2.clear();
	for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		//计算每一天的班次数量 3开头的班次（除了3-6）的人数统计到表头上
		int iClassNum = 0;
		for (int iPersonIndex = 0; iPersonIndex < g_AllLevel_PersonList.size(); iPersonIndex++)
		{
			for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum - 1; iClassIndex++)
			{
				int iClassIndex_OneDay = iClassIndex - iDayIndex * OneDayClasssNum;
				map<int, int>::iterator iter = g_mapClassIndexNeedStatistic.find(iClassIndex_OneDay);
				if (iter != g_mapClassIndexNeedStatistic.end()) //如果该班次需要统计
				{
					if (main_table[0][iPersonIndex][iClassIndex] != 0)
					{
						iClassNum++;
						break; //找到一个就可以了
					}
				}


			}
		}
		CString strDateInfo;
		CTime cThisDay = CTime(g_tStartTime + iDayIndex * 24 * 60 * 60);

		time_t tCurrent = g_tStartTime + iDayIndex * 24 * 60 * 60;
		tm* ltm = localtime(&tCurrent);
		int weekday = ltm->tm_wday;

		strDateInfo.Format(_T("%d.%02d"), cThisDay.GetMonth(), cThisDay.GetDay());

		CString stWeekDayInfo;
		if (weekday == 0)
		{
			stWeekDayInfo.Format(_T("日（%d）"), iClassNum);
		}
		else
		{
			stWeekDayInfo.Format(_T("%s（%d）"), WeekDayName[weekday-1], iClassNum);
		}

		g_vectorTableHeadInfo1.push_back(strDateInfo);
		g_vectorTableHeadInfo2.push_back(stWeekDayInfo);
	}
	for (int iColNum = 3; iColNum < 3+14; iColNum++)
	{

		//for (int iDayIndex = 0; iDayIndex < g_vectorTableHeadInfo.size(); iDayIndex++)
		{
			//设置表格显示属性
			GV_ITEM Item;
			Item.mask = GVIF_TEXT | GVIF_FORMAT;
			Item.row = 0;
			Item.col = iColNum;

			Item.nFormat = DT_CENTER | DT_WORDBREAK;
			Item.strText = g_vectorTableHeadInfo1[iColNum-3];

			m_pGrid.SetItem(&Item);


			Item.mask = GVIF_TEXT | GVIF_FORMAT;
			Item.row = 1;
			Item.col = iColNum;

			Item.nFormat = DT_CENTER | DT_WORDBREAK;
			Item.strText = g_vectorTableHeadInfo2[iColNum-3];
			m_pGrid.SetItem(&Item);
		}
	}


	// ====== 新增：设置最右侧两列表头 ======
	int iRightNameCol = 3 + AllDay;
	int iRestDayCol = 3 + AllDay + 1;

	m_pGrid.SetColumnWidth(iRightNameCol, 80);
	m_pGrid.SetColumnWidth(iRestDayCol, 80);

	GV_ITEM ItemRight;
	ItemRight.mask = GVIF_TEXT | GVIF_FORMAT;
	ItemRight.nFormat = DT_CENTER | DT_WORDBREAK;

	ItemRight.row = 0; ItemRight.col = iRightNameCol; ItemRight.strText = _T("姓名"); m_pGrid.SetItem(&ItemRight);
	ItemRight.row = 1; ItemRight.col = iRightNameCol; ItemRight.strText = _T("姓名"); m_pGrid.SetItem(&ItemRight);

	ItemRight.row = 0; ItemRight.col = iRestDayCol; ItemRight.strText = _T("剩余假期"); m_pGrid.SetItem(&ItemRight);
	ItemRight.row = 1; ItemRight.col = iRestDayCol; ItemRight.strText = _T("剩余假期"); m_pGrid.SetItem(&ItemRight);


	//将三个级别的人员显示到ListCtrl中
	ShowOneLevelPersonToList(g_Level1_PersonList, bRefreshIndex_Level, bRefreshPerson, bRefreshClassInfo, iLineIndex, iColumnIndex, _T("级别1"),1);
	ShowOneLevelPersonToList(g_Level2_PersonList, bRefreshIndex_Level, bRefreshPerson, bRefreshClassInfo, iLineIndex, iColumnIndex, _T("级别2"),2);
	ShowOneLevelPersonToList(g_Level3_PersonList, bRefreshIndex_Level, bRefreshPerson, bRefreshClassInfo, iLineIndex, iColumnIndex, _T("级别3"),3);
	ShowOneLevelPersonToList(g_Level4_PersonList, bRefreshIndex_Level, bRefreshPerson, bRefreshClassInfo, iLineIndex, iColumnIndex, _T("级别4"), 4);

	
	
	//m_pGrid.RedrawWindow();
	//UpdateData(FALSE);
	m_bIsUpdatingGrid = false;
}


void CPIVASScheduleDlg::ShowOneLevelPersonToList(vector<PersonBasicInfo>& theLevel_PersonList, bool bRefreshIndex_Level, bool bRefreshPerson, bool bRefreshClassInfo, int& iLineIndex, int& iColumnIndex,CString strLevelName, int iCurrentPersonLevel)
{
	
	for (int i = 0; i < theLevel_PersonList.size(); i++)
	{
		CString strPersonName = theLevel_PersonList[i].strPersonName.c_str();
		int iPersonIndex = FindPesrsonIndexByName(strPersonName);
		int iPersonLevel = iCurrentPersonLevel; //当前人员级别
		bool bIsFreeze = theLevel_PersonList[i].bIsFreeze; //是否冻结该人员
		iLineIndex++;
		if (bRefreshIndex_Level)
		{
			CString strIndex;
			strIndex.Format(_T("%d"), iLineIndex-1); //设置序号
			//m_ListCtrl.SetItemText(iLineIndex - 1, IndexColumn, strIndex);
			//m_ListCtrl.SetItemText(iLineIndex - 1, LevelColumn, strLevelName);


			//设置表格显示属性
			GV_ITEM Item;
			Item.mask = GVIF_TEXT | GVIF_FORMAT;
			Item.nFormat = DT_CENTER | DT_WORDBREAK;

			Item.row = iLineIndex;
			Item.col = IndexColumn;
			Item.strText = strIndex;
			m_pGrid.SetItem(&Item);

			Item.row = iLineIndex;
			Item.col = LevelColumn;
			Item.strText = strLevelName;
			m_pGrid.SetItem(&Item);
		}
		if (bRefreshPerson)
		{
			//m_ListCtrl.SetItemText(iLineIndex - 1, PersonName, strPersonName);

			//设置表格显示属性
			GV_ITEM Item;
			Item.mask = GVIF_TEXT | GVIF_FORMAT;
			Item.nFormat = DT_CENTER | DT_WORDBREAK;

			Item.row = iLineIndex;
			Item.col = PersonName;
			Item.strText = strPersonName;			
			if (iCurrentPersonLevel == 2)
			{
				Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_FGCLR;
				Item.crFgClr = G_Text_Relax_INIT_COLORREF;
				//pCell->SetTextClr(G_Text_Relax_INIT_COLORREF);
			}
			else if (iCurrentPersonLevel == 3)
			{
				Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_FGCLR;
				Item.crFgClr = G_Text_Purple_COLORREF;
				
			}
			m_pGrid.SetItem(&Item);



		}
		iColumnIndex = PersonName + 1;
		for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
		{
			CString strClassName;
			TCHAR tchClassName[50] = { 0 };
			int iClassIndex_OneDay_temp = -1;
			int iClass_ChangeRelaxValue = 0;
			strClassName = GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex, iClassIndex_OneDay_temp, iClass_ChangeRelaxValue);
			bool bCanRest = bJudgeCanRestByClassIndex(iClassIndex_OneDay_temp);
			bool bCanRest_temp22 = bCanRest;
			bCanRest = true;//所有班次都允许调休

			//在此需要得出结论，该单元格的类型，是文本还是combox
			GridOneCellData theGridOneCellData;

			MyCellType theCellType = Text_CanEdit;
			COLORREF clr_text = G_Text_Defalut_COLORREF;
			COLORREF clr_BK_Default = G_Bk_Defalut_COLORREF;
			COLORREF clr_BK = G_Bk_Defalut_COLORREF;
			//vector<wstring> vOptions; 
			CStringArray vOptions;//下拉选项


			bool bIsSepecialRelaxClass = false; //是否是特殊的调休班次，特殊的调休班次包括：补清明、补元旦、补春节等，这些班次在显示时需要特殊标识
			bool bIsWorkToRestClass = false; //是否是休息班次，休息班次在显示时需要特殊标识


			//若是初始调休
			if (iClassIndex_OneDay_temp == Class_ChangeRelax && iClass_ChangeRelaxValue == Relax_INIT)
			{
				clr_text = G_Text_Relax_INIT_COLORREF;
			}
			
			//若是原来是工作，现在改成调休的
			if (iClass_ChangeRelaxValue == WORK_TO_Relax)
			{
				strClassName = RelaxClassName;
				clr_text = G_Text_WorkToRelax_COLORREF;
				clr_BK = G_Bk_Defalut_COLORREF;
				
			}

			if (iClassIndex_OneDay_temp == Class_ChangeRelax)
			{
				if (iClass_ChangeRelaxValue == Relax_INIT)
				{
					strClassName = RelaxClassName;
				}
				else if (iClass_ChangeRelaxValue == Relax_TO_WORK)
				{
					strClassName = RelaxClassName_MODIFY;
					bIsWorkToRestClass = true;
					clr_BK = G_Bk_TakeOverWork_COLORREF;		//调休转上班(顶班)
				}
				else if (iClass_ChangeRelaxValue == WORK_TO_Relax)
				{
					strClassName = RelaxClassName_MODIFY;
					bIsWorkToRestClass = true;
					clr_BK = G_Bk_RelaxArranged_COLORREF;		//调休安排成功
					clr_text = G_Text_RelaxArranged_COLORREF;	//客户口径：调休成功的格子用蓝色字
				}
				else if (iClass_ChangeRelaxValue == REST_INIT)
				{
					strClassName = RESTClassName;
				}
				else if (iClass_ChangeRelaxValue == WORK_TO_REST_INIT)
				{
					strClassName = RESTClassName_MODIFY;
					bIsWorkToRestClass = true;
					clr_BK = G_Bk_RelaxArranged_COLORREF;		//补休安排成功
					clr_text = G_Text_RelaxArranged_COLORREF;	//客户口径：调休成功的格子用蓝色字
				}
				else 
				{
					if (iClass_ChangeRelaxValue < BU_HOLIDAY_1_WORK_TO_Relax)
					{
						std::map<int, CString >::iterator  iter_temp2 = g_BuHoliDayMap.find(iClass_ChangeRelaxValue);
						if (iter_temp2 != g_BuHoliDayMap.end())
						{
							strClassName = iter_temp2->second;

							clr_text = G_Text_Relax_INIT_COLORREF;
							bIsSepecialRelaxClass = true;
						}
					}
					else if (iClass_ChangeRelaxValue >= BU_HOLIDAY_1_WORK_TO_Relax)
					{
						std::map<int, CString >::iterator  iter_temp2 = g_BuHoliDayMap.find(iClass_ChangeRelaxValue);
						if (iter_temp2 != g_BuHoliDayMap.end())
						{
							strClassName = iter_temp2->second;
							clr_BK = G_Bk_RelaxArranged_COLORREF;		//节假日调休安排成功
							clr_text = G_Text_RelaxArranged_COLORREF;
							clr_text = G_Text_WorkToRelax_COLORREF;
							bIsSepecialRelaxClass = true;
						}

					}
					
				}
			}

			//本轮被调出来顶班的人(原本调休 -> 现在上班)：浅黄底。
			//这种人当天是有真实班次的，走不到上面那段调休分支里，所以单独判一次。
			if (iClass_ChangeRelaxValue == Relax_TO_WORK)
			{
				clr_BK = G_Bk_TakeOverWork_COLORREF;
			}


			bool bUserDefineClass = false;
			bool bIsUniversalCell = false; //是否是"删除班次"腾出来的万能格
			//这个格子是不是调休类格子(只有调休标记、没有真实班次)
			bool bIsRelaxCell = (iClassIndex_OneDay_temp == Class_ChangeRelax);

			//调休类格子也允许客户写一个自定义说明(补端午、年假之类)。
			//原来只有空白格才认 g_mapAllUserDefineClass，调休格子里写的文字根本显示不出来，
			//客户只能先"删除班次"把格子清空再写，这就是"要手动再删除班次再改"的由来。
			if (bIsRelaxCell)
			{
				CString strKey_RelaxCell;
				strKey_RelaxCell.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
				map<CString, CString>::iterator iter_RelaxUserDefine = g_mapAllUserDefineClass.find(strKey_RelaxCell);
				if (iter_RelaxUserDefine != g_mapAllUserDefineClass.end() && !iter_RelaxUserDefine->second.IsEmpty())
				{
					strClassName = iter_RelaxUserDefine->second;
					bUserDefineClass = true;
				}
			}
			if (strClassName == _T(""))
			{

				//如果是用户自定义班次则显示班次名称

				//strClassName = ReleaseClassName; //如果没有班次，则显示休息

				//如果不是休息班次显示空白
				CString strKey;
				strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
				map<CString, CString>::iterator iter_UserDefine = g_mapAllUserDefineClass.find(strKey);
				if (iter_UserDefine != g_mapAllUserDefineClass.end())
				{
					strClassName = iter_UserDefine->second; //用户自定义班次
					bUserDefineClass = true;
				}
				else
				{
					strClassName = _T(""); //如果是休息班次，则显示空白
					map<CString, int>::iterator iter = g_mapAllRestClass.find(strKey);
					if (iter == g_mapAllRestClass.end())
					{
						strClassName = _T(""); //如果不是休息班次，则显示空白
					}
					else
					{
						strClassName = ReleaseClassName; //如果没有班次，则显示休息
					}
				}





				bCanRest = false;

				//用了"删除班次"之后留下的真正空白格，就是万能格：
				//可以直接下拉挑本级别能上的任何班次，换班时也会被优先派班。
				if (strClassName == _T("") && bJudgeIsUniversalCell(iPersonIndex, iDayIndex))
				{
					bIsUniversalCell = true;
					clr_BK = G_Bk_UniversalCell_COLORREF;
				}

				//clr_BK = G_Bk_Release_COLORREF;
			}
			else
			{
				//如果原先是休息班次，那现在也只允许编辑不允许下拉

			}

			//断点用
			if (iPersonIndex == 4 && iDayIndex == 0)
			{
				int kk = 0;
			}
			//else if (strClassName == _T("调休"))
			//{
			//	strClassName = _T("调休"); //如果是调休，则显示调休
			//	clr_text = G_Text_Relax_INIT_COLORREF;
			//}
			//else if (strClassName == _T("补休"))
			//{
			//	strClassName = _T("补休"); //如果是补休，则显示补休
			//	clr_text = G_Text_Relax_INIT_COLORREF;
			//}

			if(bIsFreeze)
			{
				//clr_BK = G_Bk_Freeze_COLORREF;
				theCellType = Text_CanNotEdit;
			}
			else if (bIsUniversalCell)
			{
				//万能格：下拉里给出本级别能上的全部班次，选中即可直接排上去
				theCellType = Combox;
				vOptions.Add(ReleaseClassName);		//也允许把它改回"休"

				//万能格同样要能改成调休/节假日调休，
				//否则删完班次的格子就只剩"排班"一条路，回不到调休状态。
				vOptions.Add(RelaxClassName);			//调休
				vOptions.Add(HoliDayRlexClassName);		//节假日调休

				for (int iClassIndex_OneDay = 0; iClassIndex_OneDay < OneDayClasssNum - 1; iClassIndex_OneDay++)
				{
					//人员特例规则禁掉的班次不进下拉框
					if (!bJudgePersonCanWorkClassBySpecialRule(strPersonName, iClassIndex_OneDay))
					{
						continue;
					}
					if (NurseCanWorkClassesLevel[iPersonLevel - 1][iClassIndex_OneDay] > 0)
					{
						TCHAR strClassName_temp[100] = { 0 };
						STRING2TCHAR(ClassName[iClassIndex_OneDay], strClassName_temp, 100);
						vOptions.Add(strClassName_temp);
					}
				}
			}
			else
			{
				bool bIsRestClass_Old = false;
				CString strKey;
				strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
				map<CString, int>::iterator iter = g_mapAllRestClass.find(strKey);
				if (iter != g_mapAllRestClass.end())
				{
					bIsRestClass_Old = true;
				}

				//可以调整
				if (bCanRest && !bIsRestClass_Old)
				{
					//clr_BK = G_Bk_CanModify_COLORREF;




					theCellType = Combox;
					if (!bCanRest_temp22)
					{
						vOptions.Add(strClassName);
					}
					else
					{


						if (strClassName == RelaxClassName_MODIFY)
						{
							vOptions.Add(RelaxClassName_MODIFY);
						}
						else if (strClassName == RESTClassName_MODIFY)
						{
							vOptions.Add(RESTClassName_MODIFY);
						}
						else if (strClassName == RelaxClassName)
						{
							vOptions.Add(RelaxClassName);
							vOptions.Add(RelaxClassName_MODIFY);
						}
						else if (bIsSepecialRelaxClass)
						{
							vOptions.Add(strClassName);
							vOptions.Add(RelaxClassName_MODIFY);
						}
						else
						{
							vOptions.Add(RelaxClassName);
						}

						{


							vOptions.Add(RepairRlexClassName);

							vOptions.Add(HoliDayRlexClassName);
						}
					}
					if (strClassName != RepairRlexClassName && strClassName != RelaxClassName)
					{
						vOptions.Add(DeleteClassName);						
					}
					else
					{
						//clr_BK = G_Bk_Defalut_COLORREF;
					}
					if (strClassName == RelaxClassName)
					{
						vOptions.Add(DeleteClassName);
					}

					//下拉内容为当天该人可上的所有班次
					for (int iClassIndex_OneDay = 0; iClassIndex_OneDay< OneDayClasssNum-1; iClassIndex_OneDay++)
					{

						bool bCanRest_temp = bJudgeCanRestByClassIndex(iClassIndex_OneDay_temp);

						//人员特例规则禁掉的班次不进下拉框
						if (!bJudgePersonCanWorkClassBySpecialRule(strPersonName, iClassIndex_OneDay))
						{
							continue;
						}

						if (NurseCanWorkClassesLevel[iPersonLevel - 1][iClassIndex_OneDay] > 0 && bCanRest_temp)
						{

							TCHAR strClassName_temp[100] = { 0 };
							STRING2TCHAR(ClassName[iClassIndex_OneDay], strClassName_temp, 100);
							vOptions.Add(strClassName_temp);
						}
					}

					

				}
				else//不可调整
				{
					//clr_BK = G_Bk_Defalut_COLORREF;
					theCellType = Text_CanNotEdit;
					if (strClassName == ReleaseClassName)
					{
						//clr_BK = G_Bk_Release_COLORREF;
						theCellType = Text_CanEdit;
					}
					if (strClassName == _T("") || bUserDefineClass || bIsRestClass_Old)
					{
						//clr_BK = G_Bk_Release_COLORREF;
						theCellType = Text_CanEdit;
					}
				}
			}

			//本轮操作过的格子留一层浅灰底，便于回溯"此前都调过哪些地方"。
			//只在格子没有其它含义色(蓝/黄/绿)时才铺，右键菜单里可以清除。
			if (clr_BK == G_Bk_Defalut_COLORREF && bJudgeCellHasOperated(iPersonIndex, iDayIndex))
			{
				clr_BK = G_Bk_Operated_COLORREF;
			}

			for (int mm = 0; mm < g_vectorReturnClassPersonAndDay.size(); mm++)
			{
				ReturnClassPersonAndDay theReturnClassPersonAndDay = g_vectorReturnClassPersonAndDay[mm];
				if (theReturnClassPersonAndDay.iPersonIndex == iPersonIndex && theReturnClassPersonAndDay.iDayIndex == iDayIndex)
				{
					clr_BK = G_Bk_ReturnClass_COLORREF_MODIFY;
				}
			}

			for (int mm = 0; mm < g_vectorPersonCanRest.size(); mm++)
			{
				ReturnClassPersonAndDay theReturnClassPersonAndDay = g_vectorPersonCanRest[mm];
				if (theReturnClassPersonAndDay.iPersonIndex == iPersonIndex && theReturnClassPersonAndDay.iDayIndex == iDayIndex)
				{
					clr_BK = G_Text_Relax_INIT_COLORREF;
				}
			}

			//和导入的原始排班表比对：内容变了就用红色字体标出来，客户一眼就能找到本轮改过哪些格子。
			//原来是把底色刷成橙色，而橙色同时又表示"还班/换班方案预览"，两种意思挤在一个颜色上，
			//加上三处 key 算法不一致，看着就是"高亮没有规律"。现在分开：
			//  红色字体 = 和原始表不一样       蓝底 = 本轮成功安排的调休
			//  浅黄底   = 本轮被调出来顶班     绿底 = 删除班次留下的万能格
			//  橙底     = 当前正在预览的换班方案(选完就恢复)
			//调休成功的格子按客户口径固定用蓝色字，不被"变动标红"覆盖；
			//被顶班人员的格子班次变了，照常标红。
			if (clr_text != G_Text_RelaxArranged_COLORREF
				&& bJudgeCellChangedFromInitTable(iLineIndex, iColumnIndex, strClassName))
			{
				clr_text = G_Text_Changed_COLORREF;
			}


			if (bRefreshClassInfo)
			{
				if (theCellType == Text_CanNotEdit)
				{
					m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCell));


					//设置表格显示属性
					GV_ITEM Item = { 0 };
					//Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_BKCLR | GVIF_FGCLR | GVIS_READONLY;
					Item.mask = GVIF_TEXT | GVIF_FORMAT| GVIF_STATE; //| GVIF_BKCLR | GVIF_FGCLR 
					//黑色边框怎么去掉
					//Item.nState = GVIS_READONLY;

					//Item.mask = GVIF_TEXT |   GVIS_READONLY;
					Item.nFormat = DT_CENTER | DT_WORDBREAK ;
					Item.row = iLineIndex;
					Item.col = iColumnIndex;
					Item.strText = strClassName;
					//Item.crBkClr = clr_BK; //背景色
					//Item.crFgClr = clr_text; //前景色
					Item.crBkClr = clr_BK;    // 无条件赋值
					Item.crFgClr = clr_text;  // 无条件赋值

					if (clr_BK != clr_BK_Default)
					{
						Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_STATE | GVIF_BKCLR;
						Item.crBkClr = clr_BK;
					}

					m_pGrid.SetItem(&Item);

					//m_pGrid.SetItemState(iLineIndex, iColumnIndex, m_pGrid.GetItemState(iLineIndex, iColumnIndex) & ~GVIS_READONLY);

				}
				else if (theCellType == Combox)
				{
					m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCellCombo));
					CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid.GetCell(iLineIndex, iColumnIndex);
					
					//if (bIsWorkToRestClass)
					//{
					//	pCell->SetBackClr(clr_BK);
					//}
					//if (clr_BK != clr_BK_Default)
					{
						pCell->SetBackClr(clr_BK);
					}
					pCell->SetOptions(vOptions);
					//万能格要能手工输入下拉里没有的内容(补端毛、Z破这类自定义班次)，
					//所以用可编辑的 CBS_DROPDOWN；其余格子保持只读下拉 CBS_DROPDOWNLIST。
					pCell->SetStyle((bIsUniversalCell || bIsRelaxCell) ? CBS_DROPDOWN : CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
					pCell->SetFormat(DT_CENTER | DT_WORDBREAK);
					m_pGrid.SetItemText(iLineIndex, iColumnIndex, strClassName);
					pCell->SetTextClr(clr_text);
				}
				else if (theCellType == Text_CanEdit)
				{
					m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCell));
					GV_ITEM Item;
					Item.mask = GVIF_TEXT | GVIF_FORMAT ;//| GVIF_BKCLR | GVIF_FGCLR
					Item.nFormat = DT_CENTER | DT_WORDBREAK;
					Item.row = iLineIndex;
					Item.col = iColumnIndex;
					Item.strText = strClassName;
					//Item.crBkClr = clr_BK; //背景色
					//Item.crFgClr = clr_text; //前景色

					Item.crBkClr = clr_BK;    // 无条件赋值
					Item.crFgClr = clr_text;  // 无条件赋值

					//if (clr_BK != clr_BK_Default)
					//{
					//	Item.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_STATE | GVIF_BKCLR;
					//	Item.crBkClr = clr_BK;
					//}
					m_pGrid.SetItem(&Item);
				}
				else
				{
					//其他类型不处理

				}
			}

			//if (bRefreshClassInfo)
			//{
			//	GV_ITEM Item;
			//	Item.mask = GVIF_TEXT | GVIF_FORMAT;
			//	if (strClassName.IsEmpty())
			//	{
			//		strClassName = _T("休"); //如果没有班次，则显示休息
			//	}
			//	else if (strClassName == _T("调休"))
			//	{
			//		strClassName = _T("调休"); //如果是调休，则显示调休

			//		COLORREF clr = RGB(255, 255, 0);
			//		Item.crBkClr = clr;             // or - m_Grid.SetItemBkColour(row, col, clr);
			//		Item.crFgClr = clr_text;// RGB(255, 0, 0);    // or - m_Grid.SetItemFgColour(row, col, RGB(255,0,0));				    
			//		Item.mask |= (GVIF_BKCLR | GVIF_FGCLR);
			//	}

			//	m_ListCtrl.SetItemText(iLineIndex - 1, iColumnIndex, strClassName);

			//	if (bCanRest)
			//	{
			//		m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCellCombo));
			//		m_pGrid.SetItemText(iLineIndex, iColumnIndex, _T(""));
			//		// 获取所有的下拉选项
			//		CStringArray options;
			//		options.Add(strClassName);
			//		options.Add(_T("调休"));

			//		COLORREF rowClr = RGB(177, 227, 228);
			//		m_pGrid.SetItemText(iLineIndex, iColumnIndex, strClassName);
			//		CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid.GetCell(iLineIndex, iColumnIndex);
			//		pCell->SetBackClr(rowClr);
			//		pCell->SetOptions(options);
			//		pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
			//	}
			//	else
			//	{

			//		if (iClass_ChangeRelaxValue == WORK_TO_Relax || strClassName != _T("调休"))
			//		{
			//			m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCell));

			//			//设置表格显示属性
			//			Item.nFormat = DT_CENTER | DT_WORDBREAK;
			//			Item.row = iLineIndex;
			//			Item.col = iColumnIndex;
			//			Item.strText = strClassName;
			//			m_pGrid.SetItem(&Item);

			//			COLORREF rowClr = RGB(0xFF, 0xFF, 0xE0);
			//			CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid.GetCell(iLineIndex, iColumnIndex);
			//			pCell->SetBackClr(rowClr);
			//		}
			//		else
			//		{
			//			m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCellCombo));
			//			m_pGrid.SetItemText(iLineIndex, iColumnIndex, _T(""));
			//			// 获取所有的下拉选项
			//			CStringArray options;
			//			options.Add(_T("调休"));
			//			CString strPersonName_temp;
			//			//查找当天所有上班的人员
			//			vector<int> vPersonIndex;
			//			for (int kk = 0; kk < g_AllLevel_PersonList.size(); kk++)
			//			{
			//				for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum - 1; iClassIndex++)
			//				{
			//					if (main_table[0][kk][iClassIndex] != 0)
			//					{
			//						vPersonIndex.push_back(kk);
			//						break;
			//					}
			//				}
			//			}


			//			for (int iIndex = 0; iIndex < vPersonIndex.size(); iIndex++)
			//			{
			//				int iPersonIndex_temp = vPersonIndex[iIndex];
			//				strPersonName_temp = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();

			//				int iClassIndex_OneDay_temp = -1;
			//				//获取班次号

			//				int iClassIndexOneDay_A = -1;
			//				int iClass_ChangeRelaxValue_A = 0;
			//				GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex_temp, iClassIndex_OneDay_temp, iClass_ChangeRelaxValue_A);

			//				//判断该人是否能上该班次
			//				if (iClassIndex_OneDay_temp >= 0 && NurseCanWorkClassesLevel[iCurrentPersonLevel][iClassIndex_OneDay_temp] > 0)
			//				{
			//					options.Add(strPersonName_temp);
			//				}
			//			}


			//			COLORREF rowClr = RGB(255, 0, 0);
			//			m_pGrid.SetItemText(iLineIndex, iColumnIndex, strClassName);
			//			CGridCellCombo* pCell = (CGridCellCombo*)m_pGrid.GetCell(iLineIndex, iColumnIndex);
			//			//pCell->SetBackClr(rowClr);
			//			pCell->SetTextClr(rowClr);
			//			pCell->SetOptions(options);
			//			pCell->SetStyle(CBS_DROPDOWNLIST); //CBS_DROPDOWN, CBS_DROPDOWNLIST, CBS_SIMPLE
			//		}

			//	}

			//}
			
			
			
			iColumnIndex++;
		}

		// ====== 新增：在每行最右侧追加姓名和假期 ======
		if (bRefreshPerson || bRefreshClassInfo)
		{
			// 1. 右侧姓名列
			m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCell));
			GV_ITEM ItemName;
			ItemName.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_STATE | GVIF_BKCLR | GVIF_FGCLR;
			ItemName.nState = GVIS_READONLY;
			ItemName.nFormat = DT_CENTER | DT_WORDBREAK;
			ItemName.row = iLineIndex;
			ItemName.col = iColumnIndex;
			ItemName.strText = strPersonName;
			ItemName.crBkClr = G_Bk_Defalut_COLORREF;
			ItemName.crFgClr = G_Text_Defalut_COLORREF;
			m_pGrid.SetItem(&ItemName);

			iColumnIndex++;

			// 2. 剩余假期列
			//int iRestDays = 0;
			//map<CString, PersonRestDay>::iterator iter_rest = g_mapPersonRestDayNum.find(strPersonName);
			//if (iter_rest != g_mapPersonRestDayNum.end())
			//{
			//	iRestDays = iter_rest->second.iAllRestDay - iter_rest->second.iHasUsedDay;
			//}

			//CString strRestDays;
			//strRestDays.Format(_T("%d"), iRestDays);

			//m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCell));
			//GV_ITEM ItemRest;
			//ItemRest.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_STATE | GVIF_BKCLR | GVIF_FGCLR;
			//ItemRest.nState = GVIS_READONLY;
			
			// 2. 剩余假期列
			int iRestDays = 0;
			map<CString, PersonRestDay>::iterator iter_rest = g_mapPersonRestDayNum.find(strPersonName);
			if (iter_rest != g_mapPersonRestDayNum.end())
			{
				iRestDays = iter_rest->second.iAllRestDay - iter_rest->second.iHasUsedDay;
			}

			CString strRestDays;
			strRestDays.Format(_T("%d"), iRestDays);

			// ====== 关键修改：变成下拉框 ======
			m_pGrid.SetCellType(iLineIndex, iColumnIndex, RUNTIME_CLASS(CGridCellCombo));

			CStringArray vOptionsRest;
			for (int i = 0; i <= 20; i++)
			{
				CString opt;
				opt.Format(_T("%d"), i);
				vOptionsRest.Add(opt);
			}

			GV_ITEM ItemRest;
			// 注意：去掉了 GVIF_STATE 和 GVIS_READONLY，允许编辑
			ItemRest.mask = GVIF_TEXT | GVIF_FORMAT | GVIF_BKCLR | GVIF_FGCLR;
			ItemRest.nFormat = DT_CENTER | DT_WORDBREAK;
			ItemRest.row = iLineIndex;
			ItemRest.col = iColumnIndex;
			ItemRest.strText = strRestDays;
			ItemRest.crBkClr = G_Bk_Defalut_COLORREF;
			ItemRest.crFgClr = (iRestDays < 0) ? RGB(255, 0, 0) : G_Text_Defalut_COLORREF;
			m_pGrid.SetItem(&ItemRest);

			// 设置下拉框的属性
			CGridCellCombo* pRestCell = (CGridCellCombo*)m_pGrid.GetCell(iLineIndex, iColumnIndex);
			if (pRestCell)
			{
				pRestCell->SetOptions(vOptionsRest);
				pRestCell->SetStyle(CBS_DROPDOWNLIST);
			}
			// ==================================

			
			
			ItemRest.nFormat = DT_CENTER | DT_WORDBREAK;
			ItemRest.row = iLineIndex;
			ItemRest.col = iColumnIndex;
			ItemRest.strText = strRestDays;
			ItemRest.crBkClr = G_Bk_Defalut_COLORREF;
			ItemRest.crFgClr = (iRestDays < 0) ? RGB(255, 0, 0) : G_Text_Defalut_COLORREF; // 如果假期透支了标红提醒
			m_pGrid.SetItem(&ItemRest);
		}
	}

}



void CPIVASScheduleDlg::PutPersonLastRestInfoToVect(CString strPersonName, CString strRestType, int iDayNumber, RestEnumType iSureRestDay, int iDayIndex)
{
	//解析级别 根据人名找级别
	int iPersonLevel = FindPesrsonLevelByName(strPersonName);
	if (iPersonLevel > 0)
	{
		int LastCale_ThisRoundNeedWorkDay = 0; //上轮结余本轮需要调休天数
		int LastCale_ThisRoundNeedRestDay = 0; //上轮结余本轮需要调休天数
		int ThisRequest_ThisRoundNeedRestDay = 0; //这轮期望休息天数
		int ThisRequest_ThisRoundNeedWordDay = 0; //这轮期望上班天数
		int ThisRequest_NextRoundNeedWordDay = 0; //下轮调休天数

		RestEnumType iSureRestDay_takeOff = NO_REST; //是否调休

		if (strRestType == _T(""))
		{

		}
		else if(strRestType == RestOrWorkTye1)
		{
			LastCale_ThisRoundNeedWorkDay = iDayNumber; //上轮结余本轮需要调休天数
		}
		else if (strRestType == RestOrWorkTye2)
		{
			LastCale_ThisRoundNeedRestDay = iDayNumber; //上轮结余本轮需要调休天数
		}
		else if (strRestType == RestOrWorkTye3)
		{
			ThisRequest_ThisRoundNeedRestDay = iDayNumber; //这轮调休这轮补班
			ThisRequest_ThisRoundNeedWordDay = iDayNumber; //这轮调休这轮补班
			iSureRestDay_takeOff = Request_REST_AND_WORK;
		}
		else if (strRestType == RestOrWorkTye4)
		{
			ThisRequest_ThisRoundNeedRestDay = iDayNumber; //这轮调休下轮补班
			iSureRestDay_takeOff = Request_REST;
		}
		else if (strRestType == RestOrWorkTye5)
		{
			ThisRequest_ThisRoundNeedRestDay = iDayNumber; //这轮补班下轮休息
		}
		else
		{
			return; //不处理其他类型
		}

		//从当前容器中查找是否有该人员信息
		int iFindIndex = -1;
		for (int i = 0; i < g_vectorPersonRestInfo.size(); i++)
		{
			if (g_vectorPersonRestInfo[i].strPersonName.c_str() == strPersonName)
			{
				g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay += LastCale_ThisRoundNeedWorkDay;
				g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedRestDay += LastCale_ThisRoundNeedRestDay;
				g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedRestDay += ThisRequest_ThisRoundNeedRestDay;
				g_vectorPersonRestInfo[i].ThisRequest_ThisRoundNeedWordDay += ThisRequest_ThisRoundNeedWordDay;
				g_vectorPersonRestInfo[i].ThisRequest_NextRoundNeedWordDay += ThisRequest_NextRoundNeedWordDay;
				if (iSureRestDay > 0 && iDayIndex >= 0 && iDayIndex < AllDay)
				{
					g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType = iSureRestDay;
				}
				if (iSureRestDay_takeOff > 0 && iDayIndex >= 0 && iDayIndex < AllDay)
				{
					g_vectorPersonRestInfo[i].NeedRestDay[iDayIndex].restType = iSureRestDay_takeOff;
				}
				iFindIndex = i;
				break;
			}
		}
		if (iFindIndex == -1)
		{
			PersonRestInfo thePersonRestInfo;
			thePersonRestInfo.strPersonName = strPersonName.GetString();
			thePersonRestInfo.level = iPersonLevel;
			thePersonRestInfo.LastCale_ThisRoundNeedWorkDay = LastCale_ThisRoundNeedWorkDay;
			thePersonRestInfo.LastCale_ThisRoundNeedRestDay = LastCale_ThisRoundNeedRestDay;
			thePersonRestInfo.ThisRequest_ThisRoundNeedRestDay = ThisRequest_ThisRoundNeedRestDay;
			thePersonRestInfo.ThisRequest_ThisRoundNeedWordDay = ThisRequest_ThisRoundNeedWordDay;
			thePersonRestInfo.ThisRequest_NextRoundNeedWordDay = ThisRequest_NextRoundNeedWordDay;
			if (iSureRestDay > 0 && iDayIndex>=0 && iDayIndex < AllDay)
			{
				thePersonRestInfo.NeedRestDay[iDayIndex].restType = iSureRestDay;
			}
			if (iSureRestDay_takeOff > 0 && iDayIndex >= 0 && iDayIndex < AllDay)
			{
				thePersonRestInfo.NeedRestDay[iDayIndex].restType = iSureRestDay_takeOff;
			}

			g_vectorPersonRestInfo.push_back(thePersonRestInfo);
		}
	}
}


void CPIVASScheduleDlg::ParseChineseTxtToMap(CString strTxtPath, std::map<CString, int>& resultMap, std::map<int, CString > &BuHoliDayMap,bool bCountFrequency)
{
	// 将 CString 路径转换为 std::string，以便使用标准的 std::ifstream
	std::string strPathA;
	CStringtoString(strPathA, strTxtPath);

	// 以文本只读模式打开文件
	std::ifstream infile(strPathA);
	if (!infile.is_open())
	{
		OutputDebugString(_T(" [Error] 无法打开指定的文本文件: ") + strTxtPath + _T("\n"));
		return;
	}

	resultMap.clear();
	std::string strLineA;
	int iIndexCounter = BU_HOLIDAY_1;

	// 逐行读取文件内容（适合 ANSI/GBK 编码或不带 BOM 的 UTF-8 编码文本）
	while (std::getline(infile, strLineA))
	{
		// 1. 移除 Windows 换行符带来的行尾回车符 '\r'
		if (!strLineA.empty() && strLineA.back() == '\r')
		{
			strLineA.pop_back();
		}

		// 2. 跳过空行
		if (strLineA.empty())
		{
			continue;
		}

		// 3. 将读取到的多字节 string 安全地转换成宽字符 (Unicode) 的 CString
		// 考虑到医院及排班系统环境，txt文件通常默认为本地 ANSI (CP_ACP) 编码
		int iLen = MultiByteToWideChar(CP_ACP, 0, strLineA.c_str(), -1, NULL, 0);
		wchar_t* pwszString = new wchar_t[iLen];
		MultiByteToWideChar(CP_ACP, NULL, strLineA.c_str(), -1, pwszString, iLen);

		CString strKey(pwszString);
		delete[] pwszString; // 释放临时缓冲区

		strKey.Trim(); // 去除首尾可能存在的空格

		// 4. 根据业务需求存入 map
		if (bCountFrequency)
		{
			// 模式 A：统计每个词条出现的次数（如果重复出现，value 会递增）
			resultMap[strKey]++;
		}
		else
		{
			BuHoliDayMap[iIndexCounter] = strKey;
			BuHoliDayMap[iIndexCounter+ BU_HOLIDAY_1_WORK_TO_Relax- BU_HOLIDAY_1] = strKey+ MODIFYName;

			// 模式 B：记录该词条首次或最后一次出现的行号/索引
			resultMap[strKey] = iIndexCounter++;	
		}
	}

	infile.close();
}


// CPIVASScheduleDlg 消息处理程序

//界面设置(行高/列宽/窗口位置)存在 exe 旁边的"界面设置.ini"里。
//客户机上"50多行显示不全""列太窄看不清"这类问题在测试机复现不了，
//先让他们自己拉一次，程序记住，下次打开就是调好的样子。
static CString GetUiSettingIniPath()
{
	TCHAR strFilePath_T[300] = { 0 };
	STRING2TCHAR(g_filePath, strFilePath_T, 300);
	CString strIniPath = strFilePath_T;
	strIniPath += _T("\\界面设置.ini");
	return strIniPath;
}

static void SaveGridLayoutToIni(CPIVASScheduleDlg* pDlg)
{
	if (pDlg == nullptr || pDlg->m_pGrid.GetSafeHwnd() == NULL)
	{
		return;
	}

	CString strIniPath = GetUiSettingIniPath();

	//行高：取第一个人员行的高度当代表
	int iRowHeight = 0;
	if (pDlg->m_pGrid.GetRowCount() > 2)
	{
		iRowHeight = pDlg->m_pGrid.GetRowHeight(2);
	}
	if (iRowHeight > 0)
	{
		CString strValue;
		strValue.Format(_T("%d"), iRowHeight);
		WritePrivateProfileString(_T("Grid"), _T("RowHeight"), strValue, strIniPath);
	}

	//列宽
	int iColNum = pDlg->m_pGrid.GetColumnCount();
	CString strValue_ColNum;
	strValue_ColNum.Format(_T("%d"), iColNum);
	WritePrivateProfileString(_T("Grid"), _T("ColNum"), strValue_ColNum, strIniPath);

	for (int iCol = 0; iCol < iColNum; iCol++)
	{
		CString strKey;
		strKey.Format(_T("ColWidth%d"), iCol);
		CString strValue;
		strValue.Format(_T("%d"), pDlg->m_pGrid.GetColumnWidth(iCol));
		WritePrivateProfileString(_T("Grid"), strKey, strValue, strIniPath);
	}

	//窗口位置和大小
	WINDOWPLACEMENT thePlacement;
	memset(&thePlacement, 0, sizeof(thePlacement));
	thePlacement.length = sizeof(thePlacement);
	if (pDlg->GetWindowPlacement(&thePlacement))
	{
		CString strValue;
		strValue.Format(_T("%d"), (int)thePlacement.rcNormalPosition.left);
		WritePrivateProfileString(_T("Window"), _T("Left"), strValue, strIniPath);
		strValue.Format(_T("%d"), (int)thePlacement.rcNormalPosition.top);
		WritePrivateProfileString(_T("Window"), _T("Top"), strValue, strIniPath);
		strValue.Format(_T("%d"), (int)(thePlacement.rcNormalPosition.right - thePlacement.rcNormalPosition.left));
		WritePrivateProfileString(_T("Window"), _T("Width"), strValue, strIniPath);
		strValue.Format(_T("%d"), (int)(thePlacement.rcNormalPosition.bottom - thePlacement.rcNormalPosition.top));
		WritePrivateProfileString(_T("Window"), _T("Height"), strValue, strIniPath);
		strValue.Format(_T("%d"), (thePlacement.showCmd == SW_SHOWMAXIMIZED) ? 1 : 0);
		WritePrivateProfileString(_T("Window"), _T("Maximized"), strValue, strIniPath);
	}
}

static void LoadGridLayoutFromIni(CPIVASScheduleDlg* pDlg)
{
	if (pDlg == nullptr || pDlg->m_pGrid.GetSafeHwnd() == NULL)
	{
		return;
	}

	CString strIniPath = GetUiSettingIniPath();

	//行高
	int iRowHeight = GetPrivateProfileInt(_T("Grid"), _T("RowHeight"), 0, strIniPath);
	if (iRowHeight >= 12 && iRowHeight <= 200)
	{
		for (int iRow = 2; iRow < pDlg->m_pGrid.GetRowCount(); iRow++)
		{
			pDlg->m_pGrid.SetRowHeight(iRow, iRowHeight);
		}
	}

	//列宽
	int iColNum = pDlg->m_pGrid.GetColumnCount();
	for (int iCol = 0; iCol < iColNum; iCol++)
	{
		CString strKey;
		strKey.Format(_T("ColWidth%d"), iCol);
		int iWidth = GetPrivateProfileInt(_T("Grid"), strKey, 0, strIniPath);
		if (iWidth >= 10 && iWidth <= 1000)
		{
			pDlg->m_pGrid.SetColumnWidth(iCol, iWidth);
		}
	}

	//窗口位置和大小
	int iWidth = GetPrivateProfileInt(_T("Window"), _T("Width"), 0, strIniPath);
	int iHeight = GetPrivateProfileInt(_T("Window"), _T("Height"), 0, strIniPath);
	if (iWidth > 300 && iHeight > 200)
	{
		int iLeft = GetPrivateProfileInt(_T("Window"), _T("Left"), 0, strIniPath);
		int iTop = GetPrivateProfileInt(_T("Window"), _T("Top"), 0, strIniPath);

		//万一上次是在双屏上关的，坐标落到屏幕外面就拉回来
		int iScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int iScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		if (iScreenWidth <= 0 || iScreenHeight <= 0)
		{
			iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
			iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
		}
		if (iLeft < -100 || iLeft > iScreenWidth - 100)
		{
			iLeft = 0;
		}
		if (iTop < -100 || iTop > iScreenHeight - 100)
		{
			iTop = 0;
		}

		pDlg->SetWindowPos(NULL, iLeft, iTop, iWidth, iHeight, SWP_NOZORDER);
	}

	if (GetPrivateProfileInt(_T("Window"), _T("Maximized"), 0, strIniPath) > 0)
	{
		pDlg->ShowWindow(SW_MAXIMIZE);
	}

	pDlg->m_pGrid.ResetScrollBars();
	pDlg->m_pGrid.Invalidate();
}

BOOL CPIVASScheduleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);


	for (int m=0;m< MaxUndoNum;m++)
	{
		g_PersonRest_undo[m] = 0;
	}

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标



	g_ExcelApp.m_bAutoRelease = FALSE;
	if (!g_ExcelApp.CreateDispatch(_T("Excel.Application"), NULL))
	{
		CString szErr = _T("");//g_strEXCELCREATFAIL;
		AfxMessageBox(szErr, MB_OK | MB_ICONHAND);
		return FALSE;
	}
	// TODO: 在此添加额外的初始化代码

	m_Imagelist.Create(32, 32, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_Previous));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_Next));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_Save));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_Undo));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_Redo));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_History));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_SwitchTemplate));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_SaveTemplate));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_PersonRest));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_ICON_RESTAUTOMODIFY));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_REQUESTRST));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_EXPORTEXCEL));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_EXPORTEXCEL));
	m_Imagelist.Add(AfxGetApp()->LoadIcon(IDI_EXPORTEXCEL));

	UINT Array[9];

	m_Toolbar.Create(this);
	m_Toolbar.LoadToolBar(IDR_TOOLBAR2);//装载工具栏资源
	//m_Toolbar.SetButtons(Array, 7);
	m_Toolbar.SetButtonText(0, _T("上一轮"));
	m_Toolbar.SetButtonText(1, _T("下一轮"));
	m_Toolbar.SetButtonText(2, _T("保存"));
	m_Toolbar.SetButtonText(3, _T("撤销"));
	m_Toolbar.SetButtonText(4, _T("恢复"));
	m_Toolbar.SetButtonText(5, _T("查看历史"));
	m_Toolbar.SetButtonText(6, _T("调整班次"));//
	m_Toolbar.SetButtonText(7, _T("保存为模板"));
	m_Toolbar.SetButtonText(8, _T("剩余假期"));//"人员假期"
	m_Toolbar.SetButtonText(9, _T("自动调休"));
	m_Toolbar.SetButtonText(10, _T("请长假"));
	m_Toolbar.SetButtonText(11, _T("导出Excel"));
	m_Toolbar.SetButtonText(12, _T("与模板比较"));
	m_Toolbar.SetButtonText(13, _T("导入模板"));
	m_Toolbar.GetToolBarCtrl().SetButtonWidth(60, 120);
	m_Toolbar.GetToolBarCtrl().SetImageList(&m_Imagelist);
	m_Toolbar.SetSizes(CSize(70, 70), CSize(28, 40));

	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);


	SkinH_Attach();

	AddTray();
	
	
	




	char fileName[MAX_PATH + 1];
	GetModuleFileNameA(0, fileName, sizeof(fileName));//改为进程启动路径,当前路径是会变化的 .
	g_filePath = fileName;//..\gtplus\bin\gtplus.exe
	g_filePath = g_filePath.substr(0, g_filePath.rfind('\\'));//Left(filePath.ReverseFind('\\'));//..\gtplus\bin

	TCHAR FilePath[300] = { 0 };
	STRING2TCHAR(g_filePath.c_str(), FilePath, 300);


	CString strTxtFile = FilePath; // g_filePath 在你的代码里是 std::string
	strTxtFile += _T("\\补元旦补清明.txt");
	ParseChineseTxtToMap(strTxtFile, g_ModfifyResttMap, g_BuHoliDayMap, false);

	//遍历g_ModfifyResttMap构建新的map
	
	std::map<CString, int>::iterator iter_m;
	for (iter_m = g_ModfifyResttMap.begin(); iter_m != g_ModfifyResttMap.end(); iter_m++)
	{
		CString strKey = iter_m->first;
		int iValue = iter_m->second;
		strKey += MODIFYName;
		//根据值构建新的map
		g_ModfifyResttMap_Ex[strKey] = iValue;
	}


	ReadStartTimeFromFile();
	//time_t tCurrent = time(NULL);

	//TCHAR strFileName[100] = { 0 };
	//STRING2TCHAR(g_filePath, strFileName, 100);
	//CString strFilePath_temp = strFileName;
	//strFilePath_temp += _T("\\排班起始时间.ini");

	//TCHAR tcStartTime[50] = { 0 };
	//GetPrivateProfileString(_T("Schedule_STARTTIME"), _T("Init_StartTime"), _T(""), tcStartTime, 50, strFilePath_temp);


	//CTime cCurrentTime = tCurrent;


	////10.20解析月和日
	//CString strStartTime = tcStartTime;
	//int iMonth = 0;
	//int iDay = 0;
	//int iPos = strStartTime.Find(L".");
	//if (iPos > 0)
	//{
	//	iMonth = _ttoi(strStartTime.Left(iPos));
	//	iDay = _ttoi(strStartTime.Right(strStartTime.GetLength()- iPos-1));

	//	int year = cCurrentTime.GetYear();
	//	int month = iMonth;
	//	int day = iDay;
	//	CTime temp(year, month, day, 0, 0, 0);
	//	tCurrent = temp.GetTime();
	//}



	//tm* ltm = localtime(&tCurrent);
	//int weekday = ltm->tm_wday;

	//int iNeedAddDays = 0; //需要增加的天数
	//if  (weekday == 0) //如果是星期天，则设置为星期一
	//{
	//	iNeedAddDays = 1;
	//}
	//else if (weekday == 1)
	//{
	//	iNeedAddDays = 0;
	//}
	//else
	//{
	//	iNeedAddDays = 7 - weekday +1;
	//}

	//g_tStartTime = tCurrent + 24 * 60 * 60 * iNeedAddDays; //设置为下一个星期一的0点


	g_mapClassIndexNeedStatistic[Class_3] = 1;
	g_mapClassIndexNeedStatistic[Class_3__] = 1;
	g_mapClassIndexNeedStatistic[Class_3___1] = 1;
	g_mapClassIndexNeedStatistic[Class_3_1] = 1;
	g_mapClassIndexNeedStatistic[Class_3_2] = 1;
	g_mapClassIndexNeedStatistic[Class_3_3] = 1;
	g_mapClassIndexNeedStatistic[Class_3_4] = 1;
	g_mapClassIndexNeedStatistic[Class_3_5] = 1;
	//g_mapClassIndexNeedStatistic[Class_3_6] = 1;
	g_mapClassIndexNeedStatistic[Class_3_7] = 1;
	g_mapClassIndexNeedStatistic[Class_3_10] = 1;







	// ========= 新增：在加载模板和排班前，优先从文件恢复班次权限矩阵 =========
	ReadPersonCanWorkClassFromFile();

	//人员特例规则（某人只上 QA、某人不上 T、某人不参与联动），没这个文件也不影响使用
	ReadPersonSpecialRuleFromFile();
	// ===================================================================

	//加载排班模板
	// 
	ReadInitialSchedulFromModuleFile_FromInitFile();




	//将模板显示
	ReadInitialSchedulFromModuleFile("");

	//加载规则配置
	ReadRuleConfigFromIni();


	//第一行显示时间
	bool bHasLastModuleFile = false;
	//若没有上次版本则使用原始模板





	//读取上轮的调休补休结余数据
	ReadLastRestDayDataFromFile();

	ReadPersonHasUsedRestDayFromFile();


	//m_ListCtrl.InsertColumn(1, _T("序号"), LVCFMT_CENTER, 40);
	//m_ListCtrl.InsertColumn(2, _T("时间"), LVCFMT_CENTER, 150);

	//m_ListCtrl.InsertColumn(3, _T("最小值"), LVCFMT_CENTER, 100);
	//m_ListCtrl.InsertColumn(4, _T("最小值质量位"), LVCFMT_CENTER, 100);
	//m_ListCtrl.InsertColumn(5, _T("最大值"), LVCFMT_CENTER, 100);
	//m_ListCtrl.InsertColumn(6, _T("最大值质量位"), LVCFMT_CENTER, 100);

	//m_ListCtrl.InsertColumn(7, _T("取舍值"), LVCFMT_CENTER, 100);
	//m_ListCtrl.InsertColumn(8, _T("取舍值质量位"), LVCFMT_CENTER, 100);


	//GridCtrlInit();

	AddPlanToCurrentRedoArray();

	ReadAccessRegEdit();

	SetTimer(1, 1000, NULL);
	SetTimer(2, 10000, NULL);

	writeAccessRegEdit();

	g_bHasInitFinish = true;



	// 假设 m_pGrid 是你的 Grid 控件对象
	m_pGrid.SetSelectedRange(-1, -1, -1, -1, FALSE, FALSE); // 取消所有选中
	m_pGrid.SetFocusCell(CCellID(-1, -1)); // 取消焦点

	//恢复上次调好的行高/列宽/窗口大小
	LoadGridLayoutFromIni(this);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPIVASScheduleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPIVASScheduleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPIVASScheduleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPIVASScheduleDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值


	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	CRect rect(lpDrawItemStruct->rcItem);
	UINT uItemID = lpDrawItemStruct->itemID;
	UINT uItemAction = lpDrawItemStruct->itemAction;
	UINT uItemState = lpDrawItemStruct->itemState;
	CListCtrl* pListCtrl = (CListCtrl*)GetDlgItem(IDC_LIST1);




}

void CPIVASScheduleDlg::OnOpenRestManageDlg()
{
	// TODO: 在此添加控件通知处理程序代码
	//if (m_pDlgRestDayManagement)
	//{

	//}
	//else
	{
		m_pDlgRestDayManagement = new CDlgRestDayManagement(this);
		if (m_pDlgRestDayManagement)
		{
			m_pDlgRestDayManagement->Create(IDD_CDlgRestDayManagement, this);
			m_pDlgRestDayManagement->ShowWindow(SW_SHOW);
		}
		else
		{
			AfxMessageBox(_T("创建休息日管理对话框失败！"));
		}
	}
}

void CPIVASScheduleDlg::ChangeCurrentSheduleToTempTable()
{
	
	for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
	{
		for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
		{
			main_table_temp[0][iPerson][iClassIndex] = main_table[0][iPerson][iClassIndex];
		}
	}
	
}
void CPIVASScheduleDlg::ChangeTempSheduleToCurrentTable()
{

	for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
	{
		for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
		{
			main_table[0][iPerson][iClassIndex] = main_table_temp[0][iPerson][iClassIndex];
		}
	}

}



bool CPIVASScheduleDlg::bCheckOnePersonOneDayClassIsOk(int iPersonIndex, int iDayIndex, int iClassIndexOneDay, ClassCheckErroType &iErrorType, CString& strErroDes)
{

	bool bCheckPass = true;

	//软规则(目前只有 A/T 数量限制)只提示不拦截，每次校验先清掉上一次的提示文字
	g_strSoftRuleWarn.Empty();


	if (g_rule_work_5_day)
	{
		//检查是否连上5天班
		//查下前面几天是上班后面几天是上班，累计看看是否大于等于5天
		int iCounter_Work = 0;
		for (int iDayIndex_temp = iDayIndex - 1; iDayIndex_temp >= 0; iDayIndex_temp--)
		{
			int iWorkCount = 0;		//记录该护士在该天的工作次数
			for (int iClassIndex_temp = iDayIndex_temp * OneDayClasssNum; iClassIndex_temp < (iDayIndex_temp + 1) * OneDayClasssNum-1; iClassIndex_temp++)
			{
				if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
				{
					iWorkCount++;
				}
			}
			if (iWorkCount == 0)		//如果该护士在该天没有上班，则休息天数加1
			{
				//bCheckPass = true;
				break;
			}
			else
			{
				iCounter_Work++;		//如果该护士在该天有上班，则不满足条件
			}
		}

		//向后的连续上班天数
		int iCounter_Work_After = 0;
		for (int iDayIndex_temp = iDayIndex + 1; iDayIndex_temp < AllDay; iDayIndex_temp++)
		{
			int iWorkCount = 0;		//记录该护士在该天的工作次数
			for (int iClassIndex_temp = iDayIndex_temp * OneDayClasssNum; iClassIndex_temp < (iDayIndex_temp + 1) * OneDayClasssNum-1; iClassIndex_temp++)
			{
				if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
				{
					iWorkCount++;
				}
			}
			if (iWorkCount == 0)		//如果该护士在该天没有上班，则休息天数加1
			{
				//bCheckPass = true;
				break;
			}
			else
			{

				iCounter_Work_After++;		//如果该护士在该天有上班，则不满足条件

			}

		}

		if (iCounter_Work > 4)
		{
			bCheckPass = false;
			iErrorType = ConitWork_5_Day;		//设置错误类型为超过5天
			strErroDes = ConitWork_5_Day_ErroDes;
		}
		else if (iCounter_Work_After > 4)
		{
			bCheckPass = false;
			iErrorType = ConitWork_5_Day;		//设置错误类型为超过5天
			strErroDes = ConitWork_5_Day_ErroDes;

		}
		else if (iCounter_Work + iCounter_Work_After + 1 > 4)		//如果向前和向后加起来超过4天，则检查不通过
		{
			bCheckPass = false;
			iErrorType = ConitWork_5_Day;		//设置错误类型为超过5天
			strErroDes = ConitWork_5_Day_ErroDes;
		}
		else
		{
			//bCheckPass = true;		//如果向前和向后都没有超过4天，则检查通过
		}

		if (!bCheckPass)
		{
			return bCheckPass;
		}
	}

	//检查前一天班、当前班、后一天班，是否会出现夜班倒早班场景

	if (g_Night_MorningSwitch)
	{
		ClassType iClassType_CurrentDay = (ClassType)AllClassType[iClassIndexOneDay]; //获取当前班次类型
		if (iClassType_CurrentDay == Night)
		{
			//查看下班次是否早班
			if (iDayIndex < AllDay - 1)
			{
				int iNextDayIndex = iDayIndex + 1;
				for (int iClassIndex_temp = iNextDayIndex * OneDayClasssNum; iClassIndex_temp < (iNextDayIndex + 1) * OneDayClasssNum; iClassIndex_temp++)
				{
					if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
					{
						ClassType iClassType_NextDay = (ClassType)AllClassType[iClassIndex_temp % OneDayClasssNum];
						if (iClassType_NextDay == Moning)
						{
							bCheckPass = false;
							iErrorType = Moning_And_Night;
							strErroDes = Moning_And_Night_ErroDes;
							break;
						}
					}
				}
			}
		}
		else if (iClassType_CurrentDay == Moning)
		{
			//查看上班次是否夜班
			if (iDayIndex << AllDay - 1 > 0)
			{
				int iLastDayIndex = iDayIndex - 1;
				for (int iClassIndex_temp = iLastDayIndex * OneDayClasssNum; iClassIndex_temp < (iLastDayIndex + 1) * OneDayClasssNum; iClassIndex_temp++)
				{
					if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
					{
						ClassType iClassType_LastDay = (ClassType)AllClassType[iClassIndex_temp % OneDayClasssNum];
						if (iClassType_LastDay == Night)
						{
							bCheckPass = false;
							iErrorType = Moning_And_Night;
							strErroDes = Moning_And_Night_ErroDes;
							break;
						}
					}
				}
			}

		}

		if (!bCheckPass)
		{
			return bCheckPass;
		}
	}

	if (g_OneRound_ClassNum)
	{
		//检查一轮内是否某些班次超过限制
	//3 - 4，3 - 5，3 - 6，3 - 7 四个班次每个人一轮不能上超过两个
		int iClassNum_3_4 = 0;
		int iClassNum_3_5 = 0;
		int iClassNum_3_6 = 0;
		int iClassNum_3_7 = 0;
		int iClassNum_3_4567 = 0;
		for (int iDayIndex_temp = 0; iDayIndex_temp < AllDay; iDayIndex_temp++)
		{
			int iWorkCount = 0;		//记录该护士在该天的工作次数
			for (int iClassIndex_temp = iDayIndex_temp * OneDayClasssNum; iClassIndex_temp < (iDayIndex_temp + 1) * OneDayClasssNum; iClassIndex_temp++)
			{
				if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
				{
					int iClassIndex_OneDay_temp = iClassIndex_temp % OneDayClasssNum;
					if (iClassIndex_OneDay_temp == Class_3_4)
					{
						iClassNum_3_4++;
						iClassNum_3_4567++;
					}
					else if (iClassIndex_OneDay_temp == Class_3_5)
					{
						iClassNum_3_5++;
						iClassNum_3_4567++;
					}
					else if (iClassIndex_OneDay_temp == Class_3_6)
					{
						iClassNum_3_6++;
						iClassNum_3_4567++;
					}
					else if (iClassIndex_OneDay_temp == Class_3_7)
					{
						iClassNum_3_7++;
						iClassNum_3_4567++;
					}
				}
			}

		}
		if (iClassNum_3_4 > 2)
		{
			iErrorType = SameClass_3_4;
			strErroDes = SameClass_3_4_ErroDes;

			bCheckPass = false;
		}
		else if (iClassNum_3_5 > 2)
		{
			iErrorType = SameClass_3_5;
			strErroDes = SameClass_3_5_ErroDes;

			bCheckPass = false;
		}
		else if (iClassNum_3_6 > 2)
		{
			iErrorType = SameClass_3_6;
			strErroDes = SameClass_3_6_ErroDes;


			bCheckPass = false;
		}
		else if (iClassNum_3_7 > 2)
		{
			iErrorType = SameClass_3_7;
			strErroDes = SameClass_3_7_ErroDes;


			bCheckPass = false;
		}
		else if (iClassNum_3_4567 > 2)
		{
			iErrorType = SameClass_3_4567;
			strErroDes = SameClass_3_4567_ErroDes;

			bCheckPass = false;
		}
		
	}


	//一轮中AT班次不能超过3次
	if (g_OneRound_A_T_MAX3)
	{
		int iClassNum_A = 0;
		int iClassNum_T = 0;
		for (int iDayIndex_temp = 0; iDayIndex_temp < AllDay; iDayIndex_temp++)
		{
			int iWorkCount = 0;		//记录该护士在该天的工作次数
			for (int iClassIndex_temp = iDayIndex_temp * OneDayClasssNum; iClassIndex_temp < (iDayIndex_temp + 1) * OneDayClasssNum; iClassIndex_temp++)
			{
				if (main_table[0][iPersonIndex][iClassIndex_temp] == 1)		//如果该护士在该班次上班，则工作次数加1
				{
					int iClassIndex_OneDay_temp = iClassIndex_temp % OneDayClasssNum;
					//规则是"A、T班次每人一轮各不能上超过3个"，A和T要分开数。
					//原先这里把 T 也算进了 A 的计数(Class_A || Class_T)，导致 A+T 合计一超过3，
					//这个人就再也接不了任何 A 班，级别4的人之间根本换不动班。
					if (iClassIndex_OneDay_temp == Class_A)
					{
						iClassNum_A++;
					}
					if (iClassIndex_OneDay_temp == Class_T)
					{
						iClassNum_T++;
					}
				}
			}
		}
		//客户口径：14天一轮里 A 最多3个、T 最多3个、A+T 合计最多4个。
		//这三条都只提示、不拦截 —— 违反了照样能排、能换班，只是给个提醒。
		//注意：有的调用方传进来的是"带天数的全局班次下标"，所以要先取模，
		//否则三人轮换这条路上的 A/T 判断永远不成立。
		int iClassIndex_OneDay_Check = iClassIndexOneDay % OneDayClasssNum;

		CString strSoftDes;
		if (iClassNum_A > 3 && iClassIndex_OneDay_Check == Class_A)
		{
			strSoftDes.Format(_T("A 班一轮最多3个，当前已有 %d 个"), iClassNum_A);
		}
		else if (iClassNum_T > 3 && iClassIndex_OneDay_Check == Class_T)
		{
			strSoftDes.Format(_T("T 班一轮最多3个，当前已有 %d 个"), iClassNum_T);
		}
		else if (iClassNum_A + iClassNum_T > 4 && (iClassIndex_OneDay_Check == Class_A || iClassIndex_OneDay_Check == Class_T))
		{
			strSoftDes.Format(_T("A+T 一轮合计最多4个，当前 A %d 个、T %d 个"), iClassNum_A, iClassNum_T);
		}

		if (!strSoftDes.IsEmpty())
		{
			//注意：这里不再把 bCheckPass 置为 false，
			//否则这个人就再也接不了 A/T 班，级别4的几个人之间根本换不动班。
			g_strSoftRuleWarn = strSoftDes;
		}
	}
	
	
	//周日不能上帮夜
	if (g_Bangye_Role)
	{
		//查看该天是否周日
		if (iDayIndex == 6 || iDayIndex == 13) //如果是周日
		{
			int iClassIndex_OneDay_temp = iClassIndexOneDay % OneDayClasssNum;
			if (iClassIndex_OneDay_temp == Class_bang_ye)
			{
				iErrorType = BangYe_Role;
				strErroDes = BangYe_Role_ErroDes;
				bCheckPass = false;
			}
		}
	}
	
	return bCheckPass;
}


bool CPIVASScheduleDlg::CheckPersonRestCanModifyOk(CString strPersonName, CTime cStartTime, CTime cEndTime)
{
	bool bCanModify = true;
	int iPersonIndex = FindPesrsonIndexByName(strPersonName);
	if (iPersonIndex < 0)
	{
		return bCanModify;
	}
	time_t tStartTime = cStartTime.GetTime();
	time_t tEndTime = cEndTime.GetTime();
	for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		time_t tThisDayTime = g_tStartTime + iDayIndex * 24 * 60 * 60;
		if (tThisDayTime >= tStartTime && tThisDayTime <= tEndTime)
		{
			//查看该天是否是休息日
			int iClassIndexOneDay = -1;
			CString strClassName;
			int iClass_ChangeRelaxValue = 0;
			strClassName = GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex, iClassIndexOneDay, iClass_ChangeRelaxValue);
			if (strClassName == _T("调休") || strClassName == _T("补休"))
			{

			}
			else if(iClassIndexOneDay>=0 && iClassIndexOneDay< Class_ChangeRelax)
			{
				bool bFindSucess = false;
				//在当天查看是否有休息的人员能上其班次
				int iAllPersonNum = g_AllLevel_PersonList.size();
				for (int k = 0; k < iAllPersonNum; k++)
				{
					CString strPersonName_Peer = g_AllLevel_PersonList[k].strPersonName.c_str();
					int iPersonIndex_Peer = FindPesrsonIndexByName(strPersonName_Peer);
					if (iPersonIndex_Peer == iPersonIndex)
					{
						continue;
					}
					int iPersonLevel_Peer = FindPesrsonLevelByName(strPersonName_Peer);

					//查看该人员当天是否休息
					int iClassIndexOneDay_Peer = -1;
					CString strClassName_Peer;
					int iClass_ChangeRelaxValue_Peer = 0;
					strClassName_Peer = GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex_Peer, iClassIndexOneDay_Peer, iClass_ChangeRelaxValue_Peer);
					if (strClassName_Peer == _T("休") || strClassName_Peer == _T(""))
					{
						//该人员当天休息，查看是否能上班

						if (NurseCanWorkClassesLevel[iPersonLevel_Peer-1][iClassIndexOneDay] > 0)
						{
							int iClassIndex_Peer = iClassIndexOneDay + iDayIndex * OneDayClasssNum;
							ChangeCurrentSheduleToTempTable();
							main_table_temp[0][iPersonIndex_Peer][iClassIndex_Peer] = 1;

							//检查是否合格
							ClassCheckErroType iErrorType;
							CString strErroDes;
							bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex_Peer, iDayIndex, iClassIndexOneDay, iErrorType, strErroDes);

							if (bIsChekOK)//能给换
							{
								main_table_temp[0][iPersonIndex_Peer][iClassIndex_Peer] = 1;
								int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
								main_table_temp[0][iPersonIndex][iClassIndex_relax] = 0;
								main_table_temp[0][iPersonIndex][iClassIndex_Peer] = 0;

								if (1)
								{
									ChangeTempSheduleToCurrentTable();

									AddOneModifyToVect(iPersonIndex, iDayIndex);
									AddOneModifyToVect(iPersonIndex_Peer, iDayIndex);


									ShowAllPersonToList(false, false, true);
								}

								bFindSucess = true;
								break;
							}
						}
					}
				}


			}
		}
	}
	return bCanModify;
}


void CPIVASScheduleDlg::GridCtrlInit()
{
	m_pGrid.SetEditable(true);
	m_pGrid.SetTextBkColor(RGB(0xFF, 0xFF, 0xE0));//黄色背景
	m_pGrid.SetRowCount(8); //初始为10行
	m_pGrid.SetColumnCount(8); //初始化为11列
	m_pGrid.SetFixedRowCount(1); //表头为一行
	m_pGrid.SetFixedColumnCount(1); //表头为一列
	for (int row = 0; row < m_pGrid.GetRowCount(); row++)
	{
		for (int col = 0; col < m_pGrid.GetColumnCount(); col++)
		{
			//设置表格显示属性
			GV_ITEM Item;
			Item.mask = GVIF_TEXT | GVIF_FORMAT;
			Item.row = row;
			Item.col = col;
			m_pGrid.SetRowHeight(row, 25); //设置各行高          
			m_pGrid.SetColumnWidth(0, 64); //设置0列宽 
			m_pGrid.SetColumnWidth(col, 64); //设置各列宽
			if (row == 0 && col == 0) //第(0，0)格
			{
				Item.nFormat = DT_CENTER | DT_WORDBREAK;
				Item.strText.Format(_T("序号"), col);
			}
			else if (row < 1) //设置0行表头显示
			{
				Item.nFormat = DT_CENTER | DT_WORDBREAK;
				Item.strText.Format(_T(" 项目%d"), col);
			}
			else if (col < 1) //设置0列表头显示
			{
				if (row < m_pGrid.GetRowCount())
				{
					Item.nFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
					Item.strText.Format(_T("第%d次"), row);
				}
			}
			else
			{
				Item.nFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
				Item.strText.Format(_T("%d"), row + col);
			}
			m_pGrid.SetItem(&Item);
		}
	}
}


void CPIVASScheduleDlg::OnGridDblClick(NMHDR* pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;
	//Trace(_T("Double Clicked on row %d, col %d\n"), pItem->iRow, pItem->iColumn);
}

//判断是不是"级别受限、两三个人换不动"的班次。
//这类班次调休时要开三人轮换和N人联动，否则常常找不到方案。
//原来只写死了 A/B/C/D/T/Z，漏掉了 4-4 —— 4-4 只有级别2能上，跟 A、T 一样属于独苗班次，
//结果 9.17 金苗的 4-4 调休只做了两人直换，一直提示调休失败。
//现在除了原来那几个，再自动把"只有一个级别能上"的班次也算进来。
bool CPIVASScheduleDlg::JudgeIsSpecialClass(int iClassIndexOneDay)
{
	if (iClassIndexOneDay < 0 || iClassIndexOneDay >= OneDayClasssNum - 1)
	{
		return false;
	}

	if (iClassIndexOneDay == Class_A || iClassIndexOneDay == Class_B || iClassIndexOneDay == Class_C
		|| iClassIndexOneDay == Class_D || iClassIndexOneDay == Class_T || iClassIndexOneDay == Class_Z
		|| iClassIndexOneDay == Class_4_4)
	{
		return true;
	}

	//能上这个班的级别只有一个 -> 换班余地极小，按受限班次处理
	int iCanWorkLevelNum = 0;
	for (int iLevel = 0; iLevel < nurse_level; iLevel++)
	{
		if (NurseCanWorkClassesLevel[iLevel][iClassIndexOneDay] > 0)
		{
			iCanWorkLevelNum++;
		}
	}

	return (iCanWorkLevelNum == 1);
}
// GVN_ENDLABELEDIT
void CPIVASScheduleDlg::OnGridEndEdit(NMHDR* pNotifyStruct, LRESULT* pResult)
{

	if (m_bIsUpdatingGrid)
	{
		*pResult = 0;
		return;
	}

	char chDebug[255] = { 0 };
	int iTick1 = GetTickCount();
	sprintf(chDebug, "gao0425 OnGridEndEdit 1111");
	OutputDebugStringA(chDebug);

	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;

	int iRow = pItem->iRow;
	int iColumn = pItem->iColumn;

	if (iColumn < 3)
	{
		return;
	}

	// ====== 新增：拦截右侧的【姓名】和【剩余假期】列 ======
	if (iColumn >= 3 + AllDay)
	{
		// 如果修改的是“剩余假期”列 (3 + 14 + 1 = 18列)
		if (iColumn == 3 + AllDay + 1)
		{
			CString strPersonName = m_pGrid.GetItemText(iRow, 2); // 第2列是姓名
			CString itemText = m_pGrid.GetItemText(iRow, iColumn);
			int newRestDays = _ttoi(itemText);

			// 1. 更新内存
			// 注意：用户选的是剩余假期，所以总假期 = 修改后的剩余假期 + 已经使用的天数
			auto iter = g_mapPersonRestDayNum.find(strPersonName);
			if (iter != g_mapPersonRestDayNum.end())
			{
				iter->second.iAllRestDay = newRestDays + iter->second.iHasUsedDay;
			}
			else
			{
				PersonRestDay prd;
				prd.iAllRestDay = newRestDays;
				prd.iHasUsedDay = 0;
				g_mapPersonRestDayNum[strPersonName] = prd;
			}

			// 2. 将最新的假期数据立即保存到 CSV，防止重启后丢失
			FILE* fp;
			string filename = g_filePath + "\\人员假期信息.csv";
			fp = fopen(filename.c_str(), "w");
			if (fp != NULL)
			{
				char* chData = new char[200000];
				memset(chData, 0, 200000);
				char* chData_temp = chData;

				sprintf(chData, "序号, 级别, 人员, 年假剩余天数,\n");
				chData += strlen(chData);

				for (int k = 0; k < g_AllLevel_PersonList.size(); k++)
				{
					CString strName = g_AllLevel_PersonList[k].strPersonName.c_str();
					string strNameA;
					CStringtoString(strNameA, strName);
					int level = FindPesrsonLevelByName(strName);

					int allRest = 0;
					int hasUsed = 0;
					auto it = g_mapPersonRestDayNum.find(strName);
					if (it != g_mapPersonRestDayNum.end())
					{
						allRest = it->second.iAllRestDay;
						hasUsed = it->second.iHasUsedDay;
					}

					CString lockStr = g_AllLevel_PersonList[k].bShiftLocked ? _T("锁定") : _T("不锁定");
					string strLockA;
					CStringtoString(strLockA, lockStr);

					sprintf(chData, "%d,级别%d,%s,%d,%d,%s,\n", k + 1, level, strNameA.c_str(), allRest, hasUsed, strLockA.c_str());
					chData += strlen(chData);
				}
				fseek(fp, 0, SEEK_SET);
				fwrite(chData_temp, strlen(chData_temp) + 1, 1, fp);
				fclose(fp);
				delete[] chData_temp;
			}

			// 如果因为恢复了正数，需要把红字变回黑字，强制重绘一下
			m_pGrid.Invalidate();
			m_pGrid.RedrawWindow();
		}

		*pResult = 0;
		return; // 必须 return，坚决不能让它跑到下面的排班计算逻辑里去！
	}
	// =======================================================

	//CString strItemTextCurrent = m_pGrid.GetItemText(iRow, iColumn); //获取人员名称



	//CString strItemTextCurrent = m_pGrid.GetItemText(iRow, iColumn); //获取人员名称

	CString strPersonName = m_pGrid.GetItemText(iRow, 2); //获取人员名称
	int iPersonIndex = FindPesrsonIndexByName(strPersonName); //减去表头行

	int iCurrentPersonLevel = FindPesrsonLevelByName(strPersonName);
	int iDayIndex = iColumn - 3;

	int iClassIndexOneDay = -1;
	CString strClassName;
	int iClass_ChangeRelaxValue = 0;
	strClassName = GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex, iClassIndexOneDay, iClass_ChangeRelaxValue);

	CString itemText = m_pGrid.GetItemText(pItem->iRow, pItem->iColumn);

	std::map<CString, int>::iterator iterSpecialClass_3 = g_ModfifyResttMap_Ex.find(itemText);
	if (iterSpecialClass_3 != g_ModfifyResttMap_Ex.end())
	{
		return;
	}
	
	// 【新增修复 1】：如果用户仅仅是点开了下拉框但什么都没改，直接返回。避免触发多余的运算和弹窗！
	if (itemText == strClassName)
	{
		*pResult = 0;
		return;
	}

	// 【新增修复 2】：正式开始新的处理前，彻底清空上一次遗留的高亮缓存，保证页面刷新时标黄消失。
	g_vectorReturnClassPersonAndDay.clear();
	g_vectorPersonCanRest.clear();


	bool bHasChangeShedule = false;
	bool bIsHoliDayRst = false;
	CString strHoliDayRlexClassName = _T("");
	RelaxEnumType HoliDayRlexIndex = NO_Relax;


	bool bIsSpecialClass = false;
	std::map<CString, int>::iterator iterSpecialClass = g_ModfifyResttMap.find(strClassName);
	if (iterSpecialClass != g_ModfifyResttMap.end())
	{
		bIsSpecialClass = true;
	}


	//if (((strClassName == _T("")|| strClassName == _T("休") ) && itemText == _T("调休"))  || (strClassName == _T("调休") && itemText == _T("调休(调整)")))
	//{
	//	//休息改成调休直接改
	//	int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
	//	main_table[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
	//	main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
	//	bHasChangeShedule = true;

	//}

	if (itemText == HoliDayRlexClassName)
	{
		// 【修改1：防止DoModal引发ComboBox失去焦点导致的重入触发】
		m_bIsUpdatingGrid = true;

		//如果点击节假日调休，弹出节假日调休选择界面
		CString strHoliDayName = _T("");
		CHoliDayRestDlg dlgHolidayRelax(this);
		if (dlgHolidayRelax.DoModal() == IDOK)
		{
			strHoliDayName = dlgHolidayRelax.m_strHoliDayName;
		}

		m_bIsUpdatingGrid = false; // 弹窗结束，恢复标记

		std::map<CString, int>::iterator iterSpecialClass_temp = g_ModfifyResttMap.find(strHoliDayName);
		if (iterSpecialClass_temp != g_ModfifyResttMap.end())
		{
			//进行调休
			itemText = _T("调休");
			bIsHoliDayRst = true;
			strHoliDayRlexClassName = strHoliDayName ;
			HoliDayRlexIndex = (RelaxEnumType)(iterSpecialClass_temp->second+ BU_HOLIDAY_1_WORK_TO_Relax- BU_HOLIDAY_1);
		}
		else
		{
			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
			*pResult = -1; // 取消操作，拒绝修改
			return;
		}

		//return;
	}


	if ((strClassName == _T("") || strClassName == _T("休")) && itemText == _T("调休"))
	{
		// 1. 休息改成调休
		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		//选"节假日调休"时 itemText 已经被换成了"调休"，这里必须存节假日对应的值，
		//原来无条件存 Relax_INIT，会把"补五一""补端午"退化成普通调休。
		main_table[0][iPersonIndex][iClassIndex_relax] = bIsHoliDayRst ? HoliDayRlexIndex : Relax_INIT;

		// 只有当原先的班次不是 Class_ChangeRelax 时，才需要清零，防止覆盖！
		if (iClassIndexOneDay >= 0 && iClassIndexOneDay != Class_ChangeRelax)
		{
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
		}
		bHasChangeShedule = true;
		// 【新增修复】
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);
	}
	
	else if (strClassName == _T("调休") && bIsHoliDayRst || (strClassName == _T("调休(调整)") && bIsHoliDayRst))
	{
		// 2. 调休 改成 调休(调整)
		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		// 赋予正确的调整状态枚举值，并且不要去清零（否则就变空白了）
		main_table[0][iPersonIndex][iClassIndex_relax] = HoliDayRlexIndex;
		bHasChangeShedule = true;
		// 【新增修复】
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);
	}
	
	else if (strClassName == _T("调休") && itemText == _T("调休(调整)"))
	{
		// 2. 调休 改成 调休(调整)
		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		// 赋予正确的调整状态枚举值，并且不要去清零（否则就变空白了）
		main_table[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
		bHasChangeShedule = true;
		// 【新增修复】
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);
	}
	else if (bIsSpecialClass && itemText == _T("调休(调整)"))
	{
		// 3. 特殊班次改成 调休(调整)
		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		// 赋予正确的调整状态枚举值，并且不要去清零（否则就变空白了）
		main_table[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
		bHasChangeShedule = true;
		// 【新增修复】
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);
	}

	else if ((strClassName == _T("") || strClassName == _T("休")) && itemText == _T("补休"))
	{
		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		main_table[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
		//空白格/万能格的 iClassIndexOneDay 是 -1，原来不判断直接算下标，
		//会越界写到上一天(第0天时是上一个人)的调休位上去。
		if (iClassIndexOneDay >= 0 && iClassIndexOneDay != Class_ChangeRelax)
		{
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
		}
		bHasChangeShedule = true;
		// 【新增修复】
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);

	}
	//对某人进行调休
	else if (strClassName!=_T("调休") && itemText==_T("调休") ||( strClassName != _T("调休") && itemText == _T("补休")))
	{
		// 利用已有的 FindeClassIndex 动态获取 "Z" 班和 "3-1" 班的内部索引（该函数会自动过滤空格，十分安全）
		int iClassIndex_Z = FindeClassIndex(_T("Z"));
		int iClassIndex_3_1 = FindeClassIndex(_T("3-1"));

		if (iClassIndexOneDay == Class_3_4 || iClassIndexOneDay == Class_3_5 || iClassIndexOneDay == Class_3_7)
		{
			//需要还班
			CString strMessage;
			strMessage.Format(_T("（%s） 班次调休需要还班！"), strClassName);
			MessageBox(strMessage, _T("提示"), MB_OK | MB_ICONINFORMATION);

			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);


			bool bIsSucccess = OnAdjustRestAndNeeWorkPerson_NeedReturnClass_EX(iPersonIndex, iDayIndex, iClassIndexOneDay);
			if (!bIsSucccess)
			{
				MessageBox(_T("调休失败，请检查是否有人员可以调休！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
			}
		}
		else
		{
			//只有 A、B、C、D、T、Z 这几个班次级别限制死、两人换不动，才需要三人轮换和多人联动；
			//其余班次调休一律只做两人直换，不再列一堆多人方案。
			bool bIsSpecialClass_Adjust = JudgeIsSpecialClass(iClassIndexOneDay);
			bool bEnableThreePersonSwap = bIsSpecialClass_Adjust;

			vector<int> vecLevelOrder;

			if (iCurrentPersonLevel == 2)
			{
				vecLevelOrder.push_back(2);
				vecLevelOrder.push_back(4);
				vecLevelOrder.push_back(1);
				vecLevelOrder.push_back(3);
			}
			else if (iCurrentPersonLevel == 4)
			{
				vecLevelOrder.push_back(4);
				vecLevelOrder.push_back(2);
				vecLevelOrder.push_back(1);
				vecLevelOrder.push_back(3);
			}
			else if (iCurrentPersonLevel == 1)
			{
				vecLevelOrder.push_back(1);
				vecLevelOrder.push_back(3);
				vecLevelOrder.push_back(2);
				vecLevelOrder.push_back(4);
			}
			else if (iCurrentPersonLevel == 3)
			{
				vecLevelOrder.push_back(3);
				vecLevelOrder.push_back(1);
				vecLevelOrder.push_back(2);
				vecLevelOrder.push_back(4);
			}

			//A、B、C、D、T、Z：最多4人连环换班(申请人 -> 接班人 -> 接班人 -> 当天调休的人)；
			//其它班次只留2人，此时链式查找只会产出"把班直接给万能格"这一种两人方案，不做多人联动。
			int iMaxChainPersons = bIsSpecialClass_Adjust ? ChainMaxPersonNum_Default : 2;

			vector<RestAdjustPlan> outPlans;
			bool bDirectSuccess = OnAdjustRestAndNeeWorkPerson_EX_new(iPersonIndex, iDayIndex, iClassIndexOneDay, outPlans, vecLevelOrder, true, bEnableThreePersonSwap, iMaxChainPersons);

			if (outPlans.size() > 0)
			{

				// ?? === 新增修复：彻底隐藏下拉框并剥夺焦点 === ??
				// 1. 强制隐藏Grid上悬浮的ComboBox编辑控件
				CWnd* pEditWnd = m_pGrid.GetWindow(GW_CHILD);
				while (pEditWnd) {
					pEditWnd->ShowWindow(SW_HIDE);
					pEditWnd = pEditWnd->GetNextWindow();
				}
				// 2. 还原文本为编辑前的内容
				m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				// 3. 把焦点强行拿回给Grid，彻底剥夺下拉框的焦点
				m_pGrid.SetFocus();
				// 4. 强制立即重绘该单元格并更新窗口
				m_pGrid.RedrawCell(pItem->iRow, pItem->iColumn);
				m_pGrid.UpdateWindow();
				// ?? === 新增修复结束 === ??


				//CAdjustRestSwitchClass theAdjustRestSwitchClass;
				//theAdjustRestSwitchClass.InitData(outPlans);

				// 3. 动态创建并显示非模态对话框（内存自管理）
				//if (g_pAdjustRestDlg)
				//{
				//	delete g_pAdjustRestDlg;
				//	g_pAdjustRestDlg = nullptr;
				//}
				g_pAdjustRestDlg = new CAdjustRestSwitchClass(this);
				g_pAdjustRestDlg->InitData(outPlans, iPersonIndex, iDayIndex, iClassIndexOneDay, itemText, bIsHoliDayRst, HoliDayRlexIndex, pItem->iRow, pItem->iColumn);
				g_pAdjustRestDlg->Create(IDD_CAdjustRestSwitchClass, this);
				g_pAdjustRestDlg->ShowWindow(SW_SHOW);

				// 4. 彻底终止 OnGridEndEdit 的后续阻塞执行流程
				*pResult = -1;
				return;

				//// 弹框让用户选择方案
				//if (theAdjustRestSwitchClass.DoModal() == IDOK)
				//{
				//	int selIndex = theAdjustRestSwitchClass.m_iCurrentSelectIndex;
				//	if (selIndex >= 0 && selIndex < outPlans.size())
				//	{
				//		RestAdjustPlan selectedPlan = outPlans[selIndex];

				//		// 确定调休状态枚举值
				//		int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
				//		int iRelaxStatus = (itemText == _T("补休")) ? WORK_TO_REST_INIT : WORK_TO_Relax;
				//		if (bIsHoliDayRst)
				//		{
				//			iRelaxStatus = HoliDayRlexIndex;
				//		}

				//		// 执行换班
				//		if (selectedPlan.iPlanType == 2)
				//		{
				//			// 【两人直换】: B上A的班，A休息
				//			int iPerson_B = selectedPlan.iPersonIndex_B;

				//			main_table[0][iPerson_B][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 1;
				//			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;

				//			main_table[0][iPersonIndex][iClassIndex_relax] = iRelaxStatus;
				//			main_table[0][iPerson_B][iClassIndex_relax] = Relax_TO_WORK;

				//			g_vectorReturnClassPersonAndDay.clear();
				//			AddOneModifyToVect(iPersonIndex, iDayIndex);
				//			AddOneModifyToVect(iPerson_B, iDayIndex);
				//			bHasChangeShedule = true;

				//			bHasChangeShedule = true;

				//			////如果是补清明、补端午、补中秋，把界面改成补清明（调整）
				//			////m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strHoliDayRlexClassName);
				//			//if (bIsHoliDayRst)
				//			//{
				//			//	main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = HoliDayRlexIndex;
				//			//}
				//		}
				//		else if (selectedPlan.iPlanType == 3)
				//		{
				//			// 【三人轮换】: C上A的班，B上C的班，A休息
				//			int iPerson_B = selectedPlan.iPersonIndex_B;
				//			int iPerson_C = selectedPlan.iPersonIndex_C;

				//			int iClassIndexOneDay_C = GetPersonOneDayWorkClassIndex(iPerson_C, iDayIndex);

				//			// B接手C的班次
				//			main_table[0][iPerson_B][iDayIndex * OneDayClasssNum + iClassIndexOneDay_C] = 1;
				//			main_table[0][iPerson_C][iDayIndex * OneDayClasssNum + iClassIndexOneDay_C] = 0;

				//			// C接手A的班次
				//			main_table[0][iPerson_C][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 1;
				//			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;

				//			// 设置调休状态
				//			main_table[0][iPersonIndex][iClassIndex_relax] = iRelaxStatus;
				//			main_table[0][iPerson_B][iClassIndex_relax] = Relax_TO_WORK;

				//			g_vectorReturnClassPersonAndDay.clear();
				//			AddOneModifyToVect(iPersonIndex, iDayIndex);
				//			AddOneModifyToVect(iPerson_B, iDayIndex);
				//			AddOneModifyToVect(iPerson_C, iDayIndex);
				//			bHasChangeShedule = true;
				//		}
				//	}
				//}
				//else
				//{
				//	// 用户点击取消或关闭窗口，放弃修改
				//	m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				//	*pResult = -1;
				//	return;
				//}



			}
			else
			{
				m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				MessageBox(_T("调休失败，请检查是否有人员可以调休！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
			}
		}
		

		//// 当原先排的是特殊班次（A, B, C, D, T, Z）需要调休时
		//// 当原先排的是特殊班次（A, B, C, D, T, Z）需要调休时
		//if (JudgeIsSpecialClass(iClassIndexOneDay))
		//{
		//	int iPesonIndex_Rest = -1;

		//	// 第一步：先尝试直接将该特殊班次与申请调休的人进行调整
		//	vector<RestAdjustPlan> outPlans;
		//	bool bDirectSuccess = OnAdjustRestAndNeeWorkPerson_EX_new(iPersonIndex, iDayIndex, iClassIndexOneDay, outPlans, true, false);

		//	if (bDirectSuccess)
		//	{
		//		//弹框由用户选择一个方案
		//		CAdjustRestSwitchClass theCAdjustRestSwitchClass;
		//		theCAdjustRestSwitchClass.DoModal();

		//		g_vectorReturnClassPersonAndDay.clear();

		//		// 【新增记录】：记录直接互换涉及的两人，用于支持撤销
		//		AddOneModifyToVect(iPersonIndex, iDayIndex);
		//		if (iPesonIndex_Rest >= 0)
		//		{
		//			AddOneModifyToVect(iPesonIndex_Rest, iDayIndex);
		//		}
		//		//ShowAllPersonToList(false, false, true);
		//		bHasChangeShedule = true;
		//	}
		//	else
		//	{
		//		// 第二步：如果无法直接调整，执行降级方案
		//		int iPersonIndex_General = -1;
		//		int iClassIndex_General = -1;

		//		// 定义通用班次优先级数组（3-1 -> 3-2 -> 3-3）
		//		//int arrGeneralClasses[] = { Class_3_1, Class_3_2, Class_3_3 };

		//		// 遍历查找当天上通用班次的人员
		//		for (int i = 0; i < NormalClasss1Num; i++)
		//		{
		//			int tempPersonIndex = -1;
		//			GetPersonOneDayWorkInOneClase(iDayIndex, g_arrGeneral_1Classes[i], tempPersonIndex);

		//			if (tempPersonIndex >= 0)
		//			{
		//				iPersonIndex_General = tempPersonIndex;
		//				iClassIndex_General = g_arrGeneral_1Classes[i];
		//				break; // 找到第一个符合条件的通用班次人员就停止
		//			}
		//		}

		//		// 如果找到了上通用班次的人
		//		if (iPersonIndex_General >= 0)
		//		{

		//			//查看当天谁能补
		//			//找到当天调休的人
		//			vector<int> NoWorkPersonList;
		//			GetThisDayRestPersons(iDayIndex, NoWorkPersonList, false);
		//			//看看这些休息的人哪个能补这个班次
		//			vector<int> NoWorkPersonCanWorkInThisClass;
		//			GetThisDayCanWorPersons(iDayIndex, iClassIndex_General, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);

		//			int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();

		//			ChangeCurrentSheduleToTempTable();
		//			for (int iIndex1 = 0; iIndex1 < iFindPersonNum; iIndex1++)
		//			{
		//				int iPerson_A_Index = iPersonIndex; //申请调休的人
		//				int iPerson_B_Index = NoWorkPersonCanWorkInThisClass[iIndex1]; //当前是调休的人
		//				int iPerson_C_Index = iPersonIndex_General;
		//				int iClassIndexOneDay_C = iClassIndex_General;
		//				int iClassIndexOneDay_A = iClassIndexOneDay;

		//				//把C的班次给B
		//				int iClassIndex_C = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
		//				main_table_temp[0][iPerson_B_Index][iClassIndex_C] = 2;
		//				main_table_temp[0][iPerson_C_Index][iClassIndex_C] = 0;

		//				//检查是否合格
		//				ClassCheckErroType iErrorType;
		//				CString strErroDes;
		//				bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_C, iErrorType, strErroDes);
		//				if (!bIsChekOK)
		//				{
		//					//替换失败
		//					continue;
		//				}
		//				else
		//				{
		//					//把A的班次给C
		//					int iClassIndex_A = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
		//					main_table_temp[0][iPerson_C_Index][iClassIndex_A] = 2;
		//					main_table_temp[0][iPerson_A_Index][iClassIndex_A] = 0;

		//					//检查是否合格
		//					ClassCheckErroType iErrorType;
		//					CString strErroDes;
		//					bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A, iErrorType, strErroDes);
		//					if (!bIsChekOK)
		//					{
		//						//替换失败
		//						continue;
		//					}
		//					else
		//					{

		//						int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

		//						//

		//						main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
		//						main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;
		//						if (bIsHoliDayRst)
		//						{
		//							main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = HoliDayRlexIndex;
		//						}


		//						ChangeTempSheduleToCurrentTable();
		//						//ShowAllPersonToList(false, false, true);

		//						bHasChangeShedule = true;

		//						AddOneModifyToVect(iPersonIndex, iDayIndex);

		//						// 2. 原本休息，被抓来顶替通用班次的人 (周单)
		//						if (iPerson_B_Index >= 0)
		//						{
		//							AddOneModifyToVect(iPerson_B_Index, iDayIndex);
		//						}

		//						// 3. 原本上通用班次，现在被换成特殊班次的人 (张海琪)
		//						AddOneModifyToVect(iPersonIndex_General, iDayIndex);
		//						break;
		//						//iPesonIndex_Rest = iPerson_B_Index;
		//						//return bIsSucccess;
		//					}
		//				}
		//			}

		//		}
		//		else
		//		{
		//			// 兜底：既无法直接调整，当天也没有人上通用班次
		//			// 可在此处处理异常逻辑（如弹窗提示失败）
		//		}
		//	}
		//}
		//else
		//{			
		//	bool bHasCanRest = false;
		//	//若是红色的，他的班次已经有人上了，可以直接调休
		//	for (int mm = 0; mm < g_vectorPersonCanRest.size(); mm++)
		//	{
		//		ReturnClassPersonAndDay theReturnClassPersonAndDay = g_vectorPersonCanRest[mm];
		//		if (theReturnClassPersonAndDay.iPersonIndex == iPersonIndex && theReturnClassPersonAndDay.iDayIndex == iDayIndex)
		//		{
		//			//直接改成调休
		//			bHasCanRest = true;
		//			g_vectorPersonCanRest.erase(g_vectorPersonCanRest.begin() + mm);
		//			int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
		//			main_table[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
		//			main_table[0][iPersonIndex][iClassIndexOneDay + iDayIndex * OneDayClasssNum] = 0;
		//			//ShowAllPersonToList(false, false, true);
		//			bHasChangeShedule = true;
		//			
		//			// 【新增修复】
		//			g_vectorReturnClassPersonAndDay.clear();
		//			AddOneModifyToVect(iPersonIndex, iDayIndex);
		//			
		//			break;
		//		}
		//	}

		//	if (!bHasCanRest)
		//	{
		//		if (iClassIndexOneDay == Class_3_4 || iClassIndexOneDay == Class_3_5 || iClassIndexOneDay == Class_3_7)
		//		{
		//			//需要还班
		//			//



		//			CString strMessage;
		//			strMessage.Format(_T("（%s） 班次调休需要还班！"), strClassName);
		//			MessageBox(strMessage, _T("提示"), MB_OK | MB_ICONINFORMATION);

		//			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);


		//			bool bIsSucccess = OnAdjustRestAndNeeWorkPerson_NeedReturnClass_EX(iPersonIndex, iDayIndex, iClassIndexOneDay);
		//			if (!bIsSucccess)
		//			{
		//				MessageBox(_T("调休失败，请检查是否有人员可以调休！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		//			}
		//		}
		//		else
		//		{
		//			int iPesonIndex_Rest = -1;
		//			bool bIsRestClass = false;
		//			if (itemText == _T("补休"))
		//			{
		//				bIsRestClass = true;
		//			}
		//			bool bIsSucccess = OnAdjustRestAndNeeWorkPerson_EX(iPersonIndex, iDayIndex, iClassIndexOneDay, iPesonIndex_Rest, true, bIsRestClass);
		//			if (!bIsSucccess)
		//			{
		//				m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
		//				MessageBox(_T("调休失败，请检查是否有人员可以调休！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		//			}
		//			else
		//			{
		//				g_vectorReturnClassPersonAndDay.clear();
		//				AddOneModifyToVect(iPersonIndex, iDayIndex);
		//				AddOneModifyToVect(iPesonIndex_Rest, iDayIndex);

		//				//ShowAllPersonToList(false, false, true);
		//				bHasChangeShedule = true;

		//				//如果是补清明、补端午、补中秋，把界面改成补清明（调整）
		//				//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strHoliDayRlexClassName);
		//				if (bIsHoliDayRst)
		//				{
		//					main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = HoliDayRlexIndex;
		//				}

		//			}
		//		}
		//	}
		//}
	}
	//调休改成上班
		//删除班次
	else if (itemText == DeleteClassName)
	{
		if (strClassName != RelaxClassName && strClassName != ReleaseClassName)
		{
			//删除该班次
			if (iClassIndexOneDay >= 0) {
				main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
			}
			//main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
			// 【修复】：彻底清除调休状态，防止残留数据导致变成空白后还会生成错误的班次名
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = 0;

			// 【修复】自定义班次(补端毛、Z破之类)只存在 g_mapAllUserDefineClass 里，main_table 里没有记录，
			// 不把它一起删掉的话，格子删完还是显示原来的文字，也就永远变不成万能格。
			CString strKey_Del;
			strKey_Del.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
			g_mapAllUserDefineClass.erase(strKey_Del);

			AddOneModifyToVect(iPersonIndex, iDayIndex);

			//ShowAllPersonToList(false, false, true);
			bHasChangeShedule = true;

		}
		else if (strClassName == RelaxClassName)
		{
			//删除该班次
			//main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
			// 删除该班次
			if (iClassIndexOneDay >= 0) {
				main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
			}
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = 0;


			AddOneModifyToVect(iPersonIndex, iDayIndex);

			//ShowAllPersonToList(false, false, true);
			bHasChangeShedule = true;
		}
		else
		{
			CString strTempOut;
			strTempOut.Format(_T("第%d天(%s)班次不能删除！"), iDayIndex + 1, strClassName);
			MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
			return;
		}

	}
	//调休类格子改成一个正经班次。
	//这里原来只认字面量"调休"，认不出 补五一、补端午、补休、调休(调整) 这些同样是调休标记的格子，
	//它们会掉进下面通用的"更换班次"分支，而那边把 iClassIndexOneDay(此时是调休标记列31) 当成真班次用，
	//既不清调休标记，也会把人错当成"已经在上该班次"，结果就是自己跟自己换班、怎么选都没反应。
	//iClassIndexOneDay == Class_ChangeRelax 表示当天没有任何真实班次、只有调休标记，正是要走这里的情形。
	else if (iClassIndexOneDay == Class_ChangeRelax && itemText != _T("调休"))
	{

		CString strTempOut;
		CString strOutPut;

		//被替换的人员ID
		
		int iClass_OneDay_temp1 = FindeClassIndex(itemText);

		//输入的不是一个正经班次(补端午、年假这类自定义文字)。
		//原来这里直接把输入丢掉、一句提示都没有，客户只能先"删除班次"把格子清空再输入，
		//这就是"调休格子改不动"的由来。现在把它当成这个调休格子的自定义说明存起来，
		//调休标记原样保留，调休天数统计不受影响。
		if (iClass_OneDay_temp1 < 0 || iClass_OneDay_temp1 >= Class_ChangeRelax)
		{
			CString strFillText = itemText;
			strFillText.Trim();

			CString strKey_UserDefine;
			strKey_UserDefine.Format(_T("%d-%d"), iPersonIndex, iDayIndex);

			//和已经存着的说明一模一样就什么都不做，免得点进点出也记一步撤销
			map<CString, CString>::iterator iter_OldText = g_mapAllUserDefineClass.find(strKey_UserDefine);
			if (iter_OldText != g_mapAllUserDefineClass.end() && iter_OldText->second == strFillText)
			{
				*pResult = -1;
				return;
			}

			if (strFillText.IsEmpty() || strFillText == ReleaseClassName)
			{
				//清掉自定义说明，格子恢复显示原来的调休文字
				g_mapAllUserDefineClass.erase(strKey_UserDefine);
			}
			else
			{
				g_mapAllUserDefineClass[strKey_UserDefine] = strFillText;
			}

			g_vectorReturnClassPersonAndDay.clear();
			AddOneModifyToVect(iPersonIndex, iDayIndex);

			AddPlanToCurrentRedoArray();
			if (g_bAutoSave)
			{
				SetTimer(TIMER_AUTO_SAVE, 2000, NULL);
			}
			PostMessage(WM_REFRESH_GRID, 0, 0);

			*pResult = -1;		//文字由界面刷新统一写回，不让表格自己覆盖
			return;
		}

		//本人级别得上得了这个班次
		if (iCurrentPersonLevel <= 0 || iCurrentPersonLevel > nurse_level
			|| NurseCanWorkClassesLevel[iCurrentPersonLevel - 1][iClass_OneDay_temp1] <= 0
			|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName, iClass_OneDay_temp1))
		{
			TCHAR tcClassName_Dst[100] = { 0 };
			STRING2TCHAR(ClassName[iClass_OneDay_temp1], tcClassName_Dst, 100);
			CString strTipLevel;
			strTipLevel.Format(_T("%s（级别%d）上不了 %s 班次！"), strPersonName, iCurrentPersonLevel, tcClassName_Dst);
			MessageBox(strTipLevel, _T("提示"), MB_OK | MB_ICONINFORMATION);
			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
			*pResult = -1;
			return;
		}

		int iPersonIndex_temp = -1;
		GetPersonOneDayWorkInOneClase(iDayIndex, iClass_OneDay_temp1, iPersonIndex_temp);

		//当天没有人上这个班次(或者查出来的就是自己)，那就不存在跟谁换班的问题，
		//直接把调休改成上这个班即可。原来这里会弹"调休人员班次错误，请检查！"然后什么都不做。
		if (iPersonIndex_temp < 0 || iPersonIndex_temp == iPersonIndex)
		{
			int iClassIndex_relax_direct = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
			main_table[0][iPersonIndex][iClassIndex_relax_direct] = Relax_TO_WORK;
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClass_OneDay_temp1] = 1;

			g_vectorReturnClassPersonAndDay.clear();
			AddOneModifyToVect(iPersonIndex, iDayIndex);
			bHasChangeShedule = true;

			TCHAR tcClassName_Dst[100] = { 0 };
			STRING2TCHAR(ClassName[iClass_OneDay_temp1], tcClassName_Dst, 100);

			ClassCheckErroType iErroType_Direct = HasNO_ERROR;
			CString strErroDes_Direct;
			if (!bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClass_OneDay_temp1, iErroType_Direct, strErroDes_Direct))
			{
				CString strWarnMsg_Direct;
				strWarnMsg_Direct.Format(_T("提示：%s 上 %s 班后违反了排班规则限制！\n\n原因：%s"),
					strPersonName, tcClassName_Dst, strErroDes_Direct.GetString());
				MessageBox(strWarnMsg_Direct, _T("规则校验提示"), MB_OK | MB_ICONWARNING);
			}
			AppendClassDayNumWarn(iDayIndex, iClass_OneDay_temp1 % OneDayClasssNum);
			ShowSoftRuleWarnIfAny(this, strPersonName);
		}
		else
		{
		//int iPersonIndex_temp = FindPesrsonIndexByName(itemText);
		//获取班次
		int iClassIndex_OneDay_temp = -1;


		int iClass_ChangeRelaxValue_A = 0;
		GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex_temp, iClassIndex_OneDay_temp, iClass_ChangeRelaxValue_A);

		if (iClassIndex_OneDay_temp == -1)
		{
			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
			MessageBox(_T("调休人员班次错误，请检查！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
			return;
		}

		TCHAR strClassName[100] = { 0 };
		STRING2TCHAR(ClassName[iClassIndex_OneDay_temp], strClassName, 100);

		ChangeCurrentSheduleToTempTable();
		int iLineIndex = 0;

		//检查这个人当天上班是否满足连上5天，是否满足上了夜班不能上早班要求
		int iClassIndex = iClassIndex_OneDay_temp + iDayIndex * OneDayClasssNum;
		//main_table_temp[0][iPersonIndex_temp][iClassIndex] = 2;
		main_table_temp[0][iPersonIndex][iClassIndex] = 1;
		//检查是否合格
		ClassCheckErroType iErrorType;
		CString strErroDes;
		//bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClassIndex_OneDay_temp, iErrorType, strErroDes);

		bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClassIndex_OneDay_temp, iErrorType, strErroDes);


		if (!bIsChekOK)
		{
			main_table_temp[0][iPersonIndex][iClassIndex] = 1;
			main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;

			strTempOut.Format(_T("步骤1：将%s的调休给%s  第%d天（%s）班次，检查结果：不通过，错误:%d \r\n"), strPersonName, itemText,iDayIndex + 1, strClassName, iErrorType);

			strOutPut += strTempOut;
			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
			m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, _T("调休"));
			CString strErroMessage;

			//strErroMessage.Format(_T("调休失败，请检查是否满足批次规则限制（%s）！"), strErroDes);

			//MessageBox(strErroMessage, _T("提示"), MB_OK | MB_ICONINFORMATION);

			strErroMessage.Format(_T("调休失败，是否强行把该位置改成期望班次！"));
			if (MessageBox(strErroMessage, _T("提示"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				//强制更改
				main_table_temp[0][iPersonIndex][iClassIndex] = 1;
				//main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;
				int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
				main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_TO_WORK;
				//main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = WORK_TO_Relax;
				//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				ChangeTempSheduleToCurrentTable();
				g_vectorReturnClassPersonAndDay.clear();
				AddOneModifyToVect(iPersonIndex, iDayIndex);
				AddOneModifyToVect(iPersonIndex_temp, iDayIndex);
				//ShowAllPersonToList(false, false, true);

				bHasChangeShedule = true;
			}

		}
		else//能给换
		{

			//弹出提示询问是否是强制上该班次（不替换）
			CString strErroMessage;
			strErroMessage.Format(_T("是否是强制上该班次（不替换），选择“取消”进行两者班次替换！"));
			if (MessageBox(strErroMessage, _T("提示"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				//强制更改
				main_table_temp[0][iPersonIndex][iClassIndex] = 1;
				main_table_temp[0][iPersonIndex_temp][iClassIndex] = 1;
				int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
				main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_TO_WORK;
				
				ChangeTempSheduleToCurrentTable();
				g_vectorReturnClassPersonAndDay.clear();
				AddOneModifyToVect(iPersonIndex, iDayIndex);
				AddOneModifyToVect(iPersonIndex_temp, iDayIndex);
				//ShowAllPersonToList(false, false, true);
				bHasChangeShedule = true;
			}
			else 
			{
				main_table_temp[0][iPersonIndex][iClassIndex] = 1;
				main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;
				int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
				main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_TO_WORK;
				main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = WORK_TO_Relax;

				strTempOut.Format(_T("步骤1：将%s的调休给%s  第%d天（%s）班次，检查结果：通过\r\n"), strPersonName, itemText, iDayIndex + 1, strClassName, iErrorType);

				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

				//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				ChangeTempSheduleToCurrentTable();

				g_vectorReturnClassPersonAndDay.clear();
				AddOneModifyToVect(iPersonIndex, iDayIndex);
				AddOneModifyToVect(iPersonIndex_temp, iDayIndex);


				//ShowAllPersonToList(false, false, true);

				bHasChangeShedule = true;
			}


		}

		}	//if (iPersonIndex_temp < 0 || iPersonIndex_temp == iPersonIndex) ... else ... 结束
	}
	
	//休息改成上班
	else if (strClassName == _T("") && itemText != ReleaseClassName)
	{
		//识别输入的班次
		//查看当前班次属于哪个班
		int iClassIndex_OneDay_temp = FindeClassIndex(itemText);
		//将班次存入表中
		if (iClassIndex_OneDay_temp >= 0)
		{
			//main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = 1;

			//检查当天是否有人上该班次的人数
			int iPersonNum = 0;
			int iPersonIndex_HasWork = -1;
			for (int iPersonInex_temp = 0; iPersonInex_temp < g_AllLevel_PersonList.size(); iPersonInex_temp++)
			{
				if (main_table[0][iPersonInex_temp][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_temp] > 0)
				{
					//CString strTempOut;
					//strTempOut.Format(_T("第%d天(%s)班次已经有人上班，不能更改！"), iDayIndex + 1, ClassName[iClassIndex_OneDay]);
					//MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
					//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
					//return;
					iPersonNum++;
					iPersonIndex_HasWork = iPersonInex_temp;

				}
			}


			TCHAR strClass_dst[100] = { 0 };
			STRING2TCHAR(ClassName[iClassIndex_OneDay_temp], strClass_dst, 100);

			if (iPersonNum >= 1)
			{
				//目标班次当天已经有人在上。万能格既可以强制上去(两人同班)，
				//也可以直接把这个班换过来——对方当天空出来，变成新的万能格。
				//原先这里只有"强制上"一条路，没法主动换班。

				//万能格也只能上本级别能上的班次，手工输入班次名时这里兜底
				if (iCurrentPersonLevel <= 0 || iCurrentPersonLevel > nurse_level
					|| NurseCanWorkClassesLevel[iCurrentPersonLevel - 1][iClassIndex_OneDay_temp] <= 0
					|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName, iClassIndex_OneDay_temp))
				{
					CString strTempOut;
					strTempOut.Format(_T("%s（级别%d）上不了 %s 班次！"), strPersonName, iCurrentPersonLevel, strClass_dst);
					MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
					m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
					*pResult = -1;
					return;
				}

				CString strPersonName_Dst;
				if (iPersonIndex_HasWork >= 0 && iPersonIndex_HasWork < (int)g_AllLevel_PersonList.size())
				{
					strPersonName_Dst = g_AllLevel_PersonList[iPersonIndex_HasWork].strPersonName.c_str();
				}

				CString strAsk;
				strAsk.Format(_T("第%d天 %s 班次当天已经有 %d 人在上（%s）。\n\n是否强制上该班次（不替换，两人同班）？\n选择“否”则与 %s 换班，%s 当天变成空白的万能格。"),
					iDayIndex + 1, strClass_dst, iPersonNum, strPersonName_Dst, strPersonName_Dst, strPersonName_Dst);

				int iAnswer = MessageBox(strAsk, _T("提示"), MB_YESNOCANCEL | MB_ICONQUESTION);

				if (iAnswer == IDCANCEL)
				{
					m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
					*pResult = -1;
					return;
				}

				if (iAnswer == IDYES)
				{
					//【强制上】两人同班。3班一天最多3个人，满了就不让再加。
					if (iClassIndex_OneDay_temp == Class_3 && iPersonNum >= 3)
					{
						CString strTempOut;
						strTempOut.Format(_T("第%d天(%s)班次已经有%d人，不能再增加了！\n如果只是想顶替其中一个人，请重新选择并选“否”换班。"),
							iDayIndex + 1, strClass_dst, iPersonNum);
						MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
						m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
						*pResult = -1;
						return;
					}

					if (iPersonIndex_HasWork >= 0)
					{
						AddOnePersonCanRestToVect(iPersonIndex_HasWork, iDayIndex);
					}
				}
				else
				{
					//【换班】原来上这个班的人把班让出来，他当天就成了空白的万能格
					if (iPersonIndex_HasWork >= 0)
					{
						for (int iClassIndex_Clear = 0; iClassIndex_Clear < OneDayClasssNum; iClassIndex_Clear++)
						{
							main_table[0][iPersonIndex_HasWork][iDayIndex * OneDayClasssNum + iClassIndex_Clear] = 0;
						}

						CString strKey_Dst;
						strKey_Dst.Format(_T("%d-%d"), iPersonIndex_HasWork, iDayIndex);
						g_mapAllUserDefineClass.erase(strKey_Dst);
						g_mapAllRestClass.erase(strKey_Dst);

						AddOneModifyToVect(iPersonIndex_HasWork, iDayIndex);
					}
				}
			}
			else if (iClassIndex_OneDay_temp == Class_3 && iPersonNum >= 3)
			{
				CString strTempOut;
				strTempOut.Format(_T("第%d天(%s)班次已经有人上班，不能更改！"), iDayIndex + 1, strClass_dst);
				MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
				m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);
				return;
			}

			//万能格正式上班：把班次写进表里，并清掉这个格子的空白自定义记录
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_temp] = 1;

			CString strKey_Self;
			strKey_Self.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
			g_mapAllUserDefineClass.erase(strKey_Self);

			g_vectorReturnClassPersonAndDay.clear();
			AddOneModifyToVect(iPersonIndex, iDayIndex);
			bHasChangeShedule = true;

			//排上去之后照例做一次规则校验，违规只提示不拦截
			{
				ClassCheckErroType iErrorType_New = HasNO_ERROR;
				CString strErroDes_New;
				if (!bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClassIndex_OneDay_temp, iErrorType_New, strErroDes_New))
				{
					CString strWarnMsg;
					strWarnMsg.Format(_T("提示：%s 上 %s 班后违反了排班规则限制！\n\n原因：%s"),
						strPersonName, strClass_dst, strErroDes_New.GetString());
					MessageBox(strWarnMsg, _T("规则校验提示"), MB_OK | MB_ICONWARNING);
				}
				AppendClassDayNumWarn(iDayIndex, iClassIndex_OneDay_temp % OneDayClasssNum);
				ShowSoftRuleWarnIfAny(this, strPersonName);
			}
		}
		else
		{

			//MessageBox(_T("输入的班次错误，请检查！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
			CString strTempOut;
			//弹出确认取消弹框
			if (itemText != _T(""))
			{
				strTempOut.Format(_T("输入的班次不存在，是否仍要指定？"));
				if (MessageBox(strTempOut, _T("提示"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
				{
					::PostMessage(m_pGrid.GetSafeHwnd(), WM_LBUTTONUP, 0, 0);
					CString strKey;

					strKey.Format(_T("%d-%d"), iRow - 2, iDayIndex);
					g_mapAllUserDefineClass[strKey] = itemText;
					bHasChangeShedule = true;
					
					// 【新增修复】
					g_vectorReturnClassPersonAndDay.clear();
					AddOneModifyToVect(iPersonIndex, iDayIndex);
				}
				else
				{
					//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, ReleaseClassName);
					::PostMessage(m_pGrid.GetSafeHwnd(), WM_LBUTTONUP, 0, 0);
					*pResult = -1;
				}
			}
			else
			{
				m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, _T(""));


				CString strKey;

				strKey.Format(_T("%d-%d"), iRow - 2, iDayIndex);
				map<CString, CString>::iterator iter_temp = g_mapAllUserDefineClass.find(strKey);
				if (iter_temp != g_mapAllUserDefineClass.end())
				{
					g_mapAllUserDefineClass.erase(iter_temp);
				}

				//删除该班次
				main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
				ShowAllPersonToList(false, false, true);
				bHasChangeShedule = true;
				
				// 【新增修复】
				g_vectorReturnClassPersonAndDay.clear();
				AddOneModifyToVect(iPersonIndex, iDayIndex);
			}
			



			//return;
		}


		// ==================== 【新增代码开始】 ====================
		// 在分支最后，若成功修改了排班且属于标准班次，则校验规则但不限制修改
		if (bHasChangeShedule && iClassIndex_OneDay_temp >= 0)
		{

			CString strKey;
			strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
			g_mapAllRestClass.erase(strKey);

			ClassCheckErroType iErrorType;
			CString strErroDes;
			// 此时 main_table 已经更新过了，直接进行校验
			bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClassIndex_OneDay_temp, iErrorType, strErroDes);
			if (!bIsChekOK)
			{
				CString strWarnMsg;
				strWarnMsg.Format(_T("提示：该人员排班已修改，违反了排班规则限制！\n\n原因：%s"), strErroDes.GetString());
				MessageBox(strWarnMsg, _T("规则校验提示"), MB_OK | MB_ICONWARNING);
			}
			AppendClassDayNumWarn(iDayIndex, iClassIndex_OneDay_temp % OneDayClasssNum);
			ShowSoftRuleWarnIfAny(this, strPersonName);
		}

	}

	else if (strClassName == _T("") && itemText == ReleaseClassName)
	{
		// 1. 将其记录到显式休息的 Map 中，确保 UI 刷新时能显示“休”而不是空白
		CString strKey;
		strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
		g_mapAllRestClass[strKey] = 1;

		// 2. 清空底层排班数据表中的班次记录（0代表不上班）
		if (iClassIndexOneDay >= 0) {
			main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndexOneDay] = 0;
		}
		// 确保将可能存在的调休状态也一并清除
		main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = 0;

		// 3. 标记排班已发生改变，触发后续的撤销记录和自动保存
		bHasChangeShedule = true;

		// 4. 清理旧的高亮记录，并把当前修改的单元格加入高亮记录数组
		g_vectorReturnClassPersonAndDay.clear();
		AddOneModifyToVect(iPersonIndex, iDayIndex);
	}
	//更换班次
	else if (strClassName != _T("") && strClassName != RelaxClassName && strClassName != ReleaseClassName  && 
		itemText != RelaxClassName && itemText != ReleaseClassName && itemText != RepairRlexClassName && itemText != DeleteClassName)
	{

		bool bSwitchResult = SwitchOnePersonOneDayToCurrentClass(iPersonIndex, iDayIndex, itemText,iClassIndexOneDay);

		if (bSwitchResult)
		{
			bHasChangeShedule = true;
		}
	}


	int iTick2 = GetTickCount();
	sprintf(chDebug, "gao0425 OnGridEndEdit :%d 22222", iTick2- iTick1);
	OutputDebugStringA(chDebug);

	//上边要是成功做了修改，则将调整方案记录到Undo表中
	if (bHasChangeShedule)
	{
		AddPlanToCurrentRedoArray();

		int iTick3 = GetTickCount();
		sprintf(chDebug, "gao0425 OnGridEndEdit :%d 33333", iTick3 - iTick2);
		OutputDebugStringA(chDebug);

		if (g_bAutoSave)
		{
			//OnBnClickedButton_Save();
			// 每次编辑结束，重置并启动定时器。2000毫秒（2秒）内如果没有新的编辑，才会触发保存
			SetTimer(TIMER_AUTO_SAVE, 2000, NULL);

			int iTick4 = GetTickCount();
			sprintf(chDebug, "gao0425 OnGridEndEdit :%d 4444", iTick4 - iTick3);
			OutputDebugStringA(chDebug);
		}

	}
	if (bHasChangeShedule)
	{
		CGridCellBase* pCell = m_pGrid.GetCell(pItem->iRow, pItem->iColumn);
		if (pCell) {
			//pCell->EndEdit();
		}


	
		//ShowAllPersonToList(false, false, true);
	}

	// 【修改2：核心！拒绝 Grid 默认的覆写行为】
	if (bIsHoliDayRst)
	{
		//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strHoliDayRlexClassName);
		*pResult = -1;
	}


	if (g_bHasInitFinish)
	{
		SetFocus();
		// 强制刷新
		//m_pGrid.RedrawWindow();
		// 
		// 改为异步发送消息，让对话框在处理完当前的 EndEdit 事件后再去刷新表格
		PostMessage(WM_REFRESH_GRID, 0, 0);
	}


	int iTick5 = GetTickCount();
	sprintf(chDebug, "gao0425 OnGridEndEdit :%d 55555", iTick5 - iTick1);
	OutputDebugStringA(chDebug);
	//Trace(_T("End Edit on row %d, col %d\n"), pItem->iRow, pItem->iColumn);
	//*pResult = (m_bRejectEditChanges) ? -1 : 0;
}


bool CPIVASScheduleDlg::OnAdjustRestAndNeeWorkPerson_EX(int iPersonIndex, int iDayIndex, int iClass_OneDay,int &iPesonIndex_Rest, bool bChange, bool bIsRestClass)
{



	bool bIsSucccess = true;
	//对需要调休的人员进行处理	
	CString strPersonName_request = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
	int iPersonLevel_A = FindPesrsonLevelByName(strPersonName_request);

	CString strTempOut;
	CString strOutPut;
	TCHAR strClassName[100] = { 0 };
	STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
	strOutPut.Format(_T("步骤1：对(%s)调休进行处理，第%d天(%s)班次 \r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);

	GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][iPersonIndex][iDayIndex* OneDayClasssNum +Class_ChangeRelax]); //调休类型


	//查看当天谁能补
	//找到当天调休的人
	vector<int> NoWorkPersonList;
	GetThisDayRestPersons(iDayIndex, NoWorkPersonList,false);

	//看看这些休息的人哪个能补这个班次
	vector<int> NoWorkPersonCanWorkInThisClass;
	GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);

	int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();
	//若找不到则去找本来这天上班，但是他需要其他天需要调休的人
	if (iFindPersonNum == 0)
	{
		CString strTempOut;
		strTempOut.Format(_T("步骤2：对(%s)第%d天(%s)班次,调休进行处理，未找到能调休的人，不允许调休"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);
		strOutPut += strTempOut;
		GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

		bIsSucccess = false;
	}
	else
	{
		//找到某些人，尝试进行调整

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
				
		ChangeCurrentSheduleToTempTable();
		int iLineIndex = 0;
		bool bHasFindWorkPerson = false;
		int iPersonNum = NoWorkPersonCanWorkInThisClass.size();
		for (int i = 0; i < iPersonNum; i++)
		{

			//if (g_vectorPersonRestInfo[i].LastCale_ThisRoundNeedWorkDay > 0)
			{

				int iPersonIndex_temp = NoWorkPersonCanWorkInThisClass[i];
				CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();

				//检查这个人当天上班是否满足连上5天，是否满足上了夜班不能上早班要求
				int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
				main_table_temp[0][iPersonIndex_temp][iClassIndex] = 2;
				//检查是否合格
				ClassCheckErroType iErrorType;
				CString strErroDes;
				bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType, strErroDes);



				if (!bIsChekOK)
				{
					main_table_temp[0][iPersonIndex][iClassIndex] = 1;
					main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;

					strTempOut.Format(_T("步骤4：%s 第%d天（%s）班次，跟%s替换，检查结果：不通过，错误:%d \r\n"), strPersonName_request,iDayIndex + 1, strClassName, strPersonName_Relax_Init,iErrorType);

					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				}
				else//能给换
				{
					main_table_temp[0][iPersonIndex][iClassIndex] = 0;
					int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

					//if (iRelaxType == Relax_TO_WORK)
					//{
					//	main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
					//}
					//else
					if (bIsRestClass)
					{
						main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_REST_INIT;
					}
					else
					{
						main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
					}
					main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = Relax_TO_WORK;
					bHasFindWorkPerson = true;

					strTempOut.Format(_T("步骤4：%s-上第%d天（%s）班次，检查结果：通过\r\n"), strPersonName_Relax_Init.GetBuffer(), iDayIndex + 1, strClassName);

					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


					if (bChange)
					{
						ChangeTempSheduleToCurrentTable();
						//ShowAllPersonToList(false, false, true);
					}

					bIsSucccess = true;
					iPesonIndex_Rest = iPersonIndex_temp;
					break;

				}
			}
		}

		if (!bHasFindWorkPerson)
		{
			strTempOut.Format(_T("步骤5：对(%s)调休进行处理，第%d天(%s)班次,调休失败未找到合适上班的人\r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);
			strOutPut += strTempOut;
			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
			bIsSucccess = false;

		}
	}

	vector<int> RelaxToWorkPersonPersonList;
	GetThisDayRelaxToWorkPersons(iDayIndex, RelaxToWorkPersonPersonList);

	//若上述未找到，但是当前仍然有调休的人，则尝试进行三人之间的换班
	if (!bIsSucccess && NoWorkPersonList.size() > 0)
	{
		
		bool bCanSuccessChange = false;

		//继续检查是否有原来是调休，但是被指派班次的人，若存在这些人，判断这个人能不能上调休人的班，而剩余的调休班能不能上这个该人的班
		//即A-申请调休的人-RequestPerson  B-当前是调休的人-Relax_InitPerson  C-原来是调休但是被安排了班次的人-RelaxToWorkPerson--会有多个
		//若C能上A的班，且B能上C的班，则可以调休，B上C，C上A的班

		for (int n = 0; n < NoWorkPersonList.size(); n++)
		{
			int iPerson_A_Index = iPersonIndex; //申请调休的人
			int iPerson_B_Index = NoWorkPersonList[n]; //当前是调休的人
			CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPerson_B_Index].strPersonName.c_str();


			for (int m = 0; m < RelaxToWorkPersonPersonList.size(); m++)
			{
				int iPerson_C_Index = RelaxToWorkPersonPersonList[m]; //原先是调休但是被安排了班次的人
				CString strPersonName_RelaxToWork = g_AllLevel_PersonList[iPerson_C_Index].strPersonName.c_str();

				strTempOut.Format(_T("步骤5：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s \r\n"),
					strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

				//先判断B能不能上C的班次，再判断C能不能上A的班次
				int iClassIndexOneDay_A = -1;
				int iClassIndexOneDay_B = -1;
				int iClassIndexOneDay_C = -1;
				int iClass_ChangeRelaxValue_A = 0;
				int iClass_ChangeRelaxValue_B = 0;
				int iClass_ChangeRelaxValue_C = 0;

				int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_Relax_Init);
				int iPersonLevel_C = FindPesrsonLevelByName(strPersonName_RelaxToWork);

				GetClassNameOneDayOnePerson(iDayIndex, iPerson_A_Index, iClassIndexOneDay_A, iClass_ChangeRelaxValue_A);
				GetClassNameOneDayOnePerson(iDayIndex, iPerson_B_Index, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
				GetClassNameOneDayOnePerson(iDayIndex, iPerson_C_Index, iClassIndexOneDay_C, iClass_ChangeRelaxValue_C);

	
				ChangeCurrentSheduleToTempTable();

				//判断B能不能上C的班次
				//判断C能不能上A的班次
				if (NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_C] > 0 &&
					NurseCanWorkClassesLevel[iPersonLevel_C-1][iClassIndexOneDay_A] > 0)
				{
					//把C的班次给B
					int iClassIndex_C = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
					main_table_temp[0][iPerson_B_Index][iClassIndex_C] = 2;
					main_table_temp[0][iPerson_C_Index][iClassIndex_C] = 0;

					//检查是否合格
					ClassCheckErroType iErrorType;
					CString strErroDes;
					bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_C, iErrorType, strErroDes);
					if (!bIsChekOK)
					{
						//替换失败
						strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败 C->B失败 \r\n"),
							strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
						strOutPut += strTempOut;
						GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
					}
					else
					{
						//把A的班次给C
						int iClassIndex_A = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
						main_table_temp[0][iPerson_C_Index][iClassIndex_A] = 2;
						main_table_temp[0][iPerson_A_Index][iClassIndex_A] = 0;

						//检查是否合格
						ClassCheckErroType iErrorType;
						CString strErroDes;
						bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A, iErrorType, strErroDes);
						if (!bIsChekOK)
						{
							//替换失败
							strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败 A->C失败 \r\n"),
								strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
						}
						else
						{
							strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换成功 \r\n"),
								strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


							int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

							//
							main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
							main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

							//将班次替换
							if (bChange)
							{
								ChangeTempSheduleToCurrentTable();
								//ShowAllPersonToList(false, false, true);
							}

							bIsSucccess = true;

							iPesonIndex_Rest = iPerson_B_Index;
							return bIsSucccess;
						}
					}
				}
				else
				{
					strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败，班次不能上 \r\n"),
						strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				}


			}
		}

	}



	//若如上仍然没有找到合适的人员，则进行4人之间轮换
	//A想跟B换，C和D是调休的人员，A->C,C->D,D->B
	//找出所有可能的C和D，判断是否能够轮换
	//if (!bIsSucccess && NoWorkPersonList.size() > 0 && RelaxToWorkPersonPersonList.size() >= 2)
	//{
	//	for (int n = 0; n < NoWorkPersonList.size(); n++)
	//	{
	//		int iPerson_A_Index = iPersonIndex; //申请调休的人
	//		int iPerson_B_Index = NoWorkPersonList[n]; //当前是调休的人
	//		CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPerson_B_Index].strPersonName.c_str();

	//		for (int m = 0; m < RelaxToWorkPersonPersonList.size(); m++)
	//		{
	//			int iPerson_C_Index = RelaxToWorkPersonPersonList[m]; //原先是调休但是被安排了班次的人
	//			CString strPersonName_RelaxToWork_1 = g_AllLevel_PersonList[iPerson_C_Index].strPersonName.c_str();

	//			for (int ww = 0; ww < RelaxToWorkPersonPersonList.size(); ww++)
	//			{
	//				int iPerson_D_Index = RelaxToWorkPersonPersonList[ww]; //原先是调休但是被安排了班次的人
	//				CString strPersonName_RelaxToWork_2 = g_AllLevel_PersonList[iPerson_D_Index].strPersonName.c_str();

	//				if (iPerson_C_Index == iPerson_D_Index)
	//				{
	//					continue;
	//				}

	//				strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人1(C):%s  之前是调休被安排工作的人2(D):%s \r\n"),
	//					strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer());
	//				strOutPut += strTempOut;
	//				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	//				//判断C能不能接A  D能不能接C,B能不能接D
	//				int iClassIndexOneDay_A = -1;
	//				int iClassIndexOneDay_B = -1;
	//				int iClassIndexOneDay_C = -1;
	//				int iClassIndexOneDay_D = -1;
	//				int iClass_ChangeRelaxValue_A = 0;
	//				int iClass_ChangeRelaxValue_B = 0;
	//				int iClass_ChangeRelaxValue_C = 0;
	//				int iClass_ChangeRelaxValue_D = 0;

	//				int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_Relax_Init);
	//				int iPersonLevel_C = FindPesrsonLevelByName(strPersonName_RelaxToWork_1);
	//				int iPersonLevel_D = FindPesrsonLevelByName(strPersonName_RelaxToWork_1);

	//				GetClassNameOneDayOnePerson(iDayIndex, iPerson_A_Index, iClassIndexOneDay_A, iClass_ChangeRelaxValue_A);
	//				GetClassNameOneDayOnePerson(iDayIndex, iPerson_B_Index, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
	//				GetClassNameOneDayOnePerson(iDayIndex, iPerson_C_Index, iClassIndexOneDay_C, iClass_ChangeRelaxValue_C);
	//				GetClassNameOneDayOnePerson(iDayIndex, iPerson_D_Index, iClassIndexOneDay_D, iClass_ChangeRelaxValue_D);


	//				ChangeCurrentSheduleToTempTable();
	//				if (NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_D] > 0 &&
	//					NurseCanWorkClassesLevel[iPersonLevel_D-1][iClassIndexOneDay_C] > 0 &&
	//					NurseCanWorkClassesLevel[iPersonLevel_C-1][iClassIndexOneDay_A] > 0
	//					)
	//				{
	//					//

	//					//把D的班次给B
	//					int iClassIndex_D = iClassIndexOneDay_D + iDayIndex * OneDayClasssNum;
	//					main_table_temp[0][iPerson_B_Index][iClassIndex_D] = 2;
	//					main_table_temp[0][iPerson_D_Index][iClassIndex_D] = 0;

	//					//检查是否合格
	//					ClassCheckErroType iErrorType;
	//					CString strErroDes;
	//					bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_D, iErrorType, strErroDes);
	//					if (!bIsChekOK)
	//					{
	//						//替换失败
	//						strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人1(C)%s 之前是调休被安排工作的人2(D)%s 替换失败 D->B失败 \r\n"),
	//							strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
	//						strOutPut += strTempOut;
	//						GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
	//					}
	//					else
	//					{

	//						//把C的班次给D
	//						int iClassIndex_C = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
	//						main_table_temp[0][iPerson_D_Index][iClassIndex_C] = 2;
	//						main_table_temp[0][iPerson_C_Index][iClassIndex_C] = 0;

	//						//检查是否合格
	//						ClassCheckErroType iErrorType;
	//						CString strErroDes;
	//						bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_D_Index, iDayIndex, iClassIndex_C, iErrorType, strErroDes);
	//						if (!bIsChekOK)
	//						{
	//							//替换失败
	//							strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人%s 替换失败 C->D失败 \r\n"),
	//								strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
	//							strOutPut += strTempOut;
	//							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
	//						}
	//						else
	//						{

	//							//把A的班次给C
	//							int iClassIndex_A = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
	//							main_table_temp[0][iPerson_C_Index][iClassIndex_A] = 2;
	//							main_table_temp[0][iPerson_A_Index][iClassIndex_A] = 0;

	//							//检查是否合格
	//							ClassCheckErroType iErrorType;
	//							CString strErroDes;
	//							bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A, iErrorType, strErroDes);
	//							if (!bIsChekOK)
	//							{
	//								//替换失败
	//								strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人:%s 替换失败 A->C失败 \r\n"),
	//									strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer());
	//								strOutPut += strTempOut;
	//								GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
	//							}
	//							else
	//							{
	//								strTempOut.Format(_T("步骤6：尝试进行四人之间换，请求调休的人(A)%s、当前是调休的人(B):%s、之前是调休被安排工作的人_1:%s 之前是调休被安排工作的人_2:%s  替换成功 \r\n"),
	//									strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
	//								strOutPut += strTempOut;
	//								GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


	//								int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

	//								//
	//								main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
	//								main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

	//								//将班次替换
	//								if (bChange)
	//								{
	//									ChangeTempSheduleToCurrentTable();
	//									//ShowAllPersonToList(false, false, true);
	//								}

	//								bIsSucccess = true;
	//								iPesonIndex_Rest = iPerson_B_Index;
	//								return bIsSucccess;
	//							}
	//						}
	//					}
	//				}
	//				else
	//				{
	//					continue;
	//				}
	//			}

	//		}

	//	}
	//}

	return bIsSucccess;
}


//==================================================================================
//                          N 人链式联动换班
//
// 场景举例：级别4的吴礼宜上A班想调休，但当天调休的人(潘雪萍、任超…)级别都上不了A班，
//           两人直换、三人轮换都换不动。此时可以拉长链条：
//           吴雯婷接吴礼宜的A班 -> 方海利接吴雯婷的C班 -> 任超(补五一)接方海利的2(6)班，
//           任超原本的休息名额落到吴礼宜头上，一次操作全部完成。
//
// 实现要点：链条上除申请人外每个人都必须
//           (1) 级别上能上他要接的那个班；
//           (2) 自己原来的班允许被换走(夜1/夜2/帮夜/3-6/3(10)不许)；
//           (3) 顶上去之后仍然满足连上5天、夜转早、A/T次数等全部排班规则。
//==================================================================================

//取某人某天真正上的班次（不含第32列的调休标记列），当天不上班返回 -1
static int GetWorkClassIndexOfDay_Safe(int iPersonIndex, int iDayIndex)
{
	if (iPersonIndex < 0 || iDayIndex < 0 || iDayIndex >= AllDay)
	{
		return -1;
	}
	for (int iClassIndex_OneDay = 0; iClassIndex_OneDay < OneDayClasssNum - 1; iClassIndex_OneDay++)
	{
		if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndex_OneDay] != 0)
		{
			return iClassIndex_OneDay;
		}
	}
	return -1;
}

//按级别优先顺序重排候选人，让优先级别的人先被搜到，好方案排在前面
static void SortPersonListByLevelOrder(vector<int>& vecPersonList, const vector<int>& vecPersonLevel, const vector<int>& vecLevelOrder)
{
	if (vecLevelOrder.size() != 4)
	{
		return;
	}

	vector<int> vecSorted;
	for (int m = 0; m < (int)vecLevelOrder.size(); m++)
	{
		for (int k = 0; k < (int)vecPersonList.size(); k++)
		{
			int iPersonIndex = vecPersonList[k];
			if (iPersonIndex >= 0 && iPersonIndex < (int)vecPersonLevel.size()
				&& vecPersonLevel[iPersonIndex] == vecLevelOrder[m])
			{
				vecSorted.push_back(iPersonIndex);
			}
		}
	}

	//级别不在顺序表里的人兜底补到最后，保证一个都不会漏
	for (int k = 0; k < (int)vecPersonList.size(); k++)
	{
		bool bHasAdd = false;
		for (int j = 0; j < (int)vecSorted.size(); j++)
		{
			if (vecSorted[j] == vecPersonList[k])
			{
				bHasAdd = true;
				break;
			}
		}
		if (!bHasAdd)
		{
			vecSorted.push_back(vecPersonList[k]);
		}
	}

	vecPersonList = vecSorted;
}

//判断某人某天是不是"万能格"：用了"删除班次"之后留下的空白格
bool CPIVASScheduleDlg::bJudgeIsUniversalCell(int iPersonIndex, int iDayIndex)
{
	if (iPersonIndex < 0 || iPersonIndex >= (int)g_AllLevel_PersonList.size())
	{
		return false;
	}
	if (iDayIndex < 0 || iDayIndex >= AllDay)
	{
		return false;
	}

	//当天只要有班次、或者有调休/补休标记，就不是空白格
	for (int iClassIndex_OneDay = 0; iClassIndex_OneDay < OneDayClasssNum; iClassIndex_OneDay++)
	{
		if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndex_OneDay] != 0)
		{
			return false;
		}
	}

	CString strKey;
	strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);

	//模板里本来就写着"休"的格子不是万能格
	if (g_mapAllRestClass.find(strKey) != g_mapAllRestClass.end())
	{
		return false;
	}

	//写着其它自定义文字的格子也不是万能格，只有真正空白的才算
	map<CString, CString>::iterator iter_UserDefine = g_mapAllUserDefineClass.find(strKey);
	if (iter_UserDefine != g_mapAllUserDefineClass.end())
	{
		CString strUserDefineText = iter_UserDefine->second;
		strUserDefineText.Trim();
		if (!strUserDefineText.IsEmpty())
		{
			return false;
		}
	}

	return true;
}

//找出当天所有万能格人员
int CPIVASScheduleDlg::GetThisDayUniversalCellPersons(int iDayIndex, vector<int>& UniversalPersonList)
{
	int iAllPersonNum = (int)g_AllLevel_PersonList.size();
	for (int iPersonIndex = 0; iPersonIndex < iAllPersonNum; iPersonIndex++)
	{
		if (bJudgeIsUniversalCell(iPersonIndex, iDayIndex))
		{
			UniversalPersonList.push_back(iPersonIndex);
		}
	}
	return (int)UniversalPersonList.size();
}

//判断某人是否已经在当前链条上（同一个人不能在一条链里出现两次）
static bool IsPersonInChainPath(const vector<int>& vecPath, int iPersonIndex)
{
	for (int k = 0; k < (int)vecPath.size(); k++)
	{
		if (vecPath[k] == iPersonIndex)
		{
			return true;
		}
	}
	return false;
}

//把一条走通的链记录成一个可选方案
void CPIVASScheduleDlg::MakeOneChainPlan(ChainSearchCtx& ctx, int iEndPersonIndex, bool bIsUniversalCell)
{
	RestAdjustPlan plan;
	plan.iPlanType = ChainPlanType;
	plan.iPerson_Source = ctx.vecPath[0];
	plan.iDayIndex_Source = ctx.iDayIndex;
	plan.bIsRuleValid = true;
	plan.bEndIsUniversalCell = bIsUniversalCell;

	//链条 = 申请人 + 中间逐级接班的人 + 末端顶班上岗的人(原本调休，或者是万能格)
	plan.vecChainPersons = ctx.vecPath;
	plan.vecChainClassIdx = ctx.vecPathClass;
	plan.vecChainPersons.push_back(iEndPersonIndex);
	plan.vecChainClassIdx.push_back(-1);		//末端的人原本没有班次

	int iChainNum = (int)plan.vecChainPersons.size();

	for (int i = 0; i < iChainNum; i++)
	{
		CString strOneClassName = _T("休");
		if (i == iChainNum - 1 && bIsUniversalCell)
		{
			strOneClassName = _T("万能格");
		}
		int iClassIndex_OneDay = plan.vecChainClassIdx[i];
		if (iClassIndex_OneDay >= 0 && iClassIndex_OneDay < OneDayClasssNum)
		{
			TCHAR tcClassName[100] = { 0 };
			STRING2TCHAR(ClassName[iClassIndex_OneDay], tcClassName, 100);
			strOneClassName = tcClassName;
		}
		plan.vecChainClassName.push_back(strOneClassName);
	}

	//把链条上每一步的违规原因汇总起来，有一步违规整条方案就标成违规，交给用户自己判断
	CString strAllErroDes;		//硬性违规(会把方案标成"违规")
	CString strAllSoftDes;		//软规则提示(A/T数量这类，只提示不拦截)
	for (int i = 0; i < (int)ctx.vecPathErroDes.size() && i < iChainNum; i++)
	{
		CString strOneErroDes = ctx.vecPathErroDes[i];
		if (strOneErroDes.IsEmpty())
		{
			continue;
		}

		bool bIsSoftOnly = (strOneErroDes.Find(SoftRuleTipPrefix) == 0);
		if (bIsSoftOnly)
		{
			strOneErroDes = strOneErroDes.Mid((int)_tcslen(SoftRuleTipPrefix));
		}

		CString strOneLine;
		strOneLine.Format(_T("%s：%s"),
			g_AllLevel_PersonList[plan.vecChainPersons[i]].strPersonName.c_str(), strOneErroDes);

		if (bIsSoftOnly)
		{
			if (!strAllSoftDes.IsEmpty())
			{
				strAllSoftDes += _T("；");
			}
			strAllSoftDes += strOneLine;
		}
		else
		{
			if (!strAllErroDes.IsEmpty())
			{
				strAllErroDes += _T("；");
			}
			strAllErroDes += strOneLine;
		}
	}
	plan.strRuleErrorDesc = strAllErroDes;
	plan.bIsRuleValid = strAllErroDes.IsEmpty();

	//软提示不影响方案成立，但一定要显示出来
	if (!strAllSoftDes.IsEmpty())
	{
		if (!plan.strRuleErrorDesc.IsEmpty())
		{
			plan.strRuleErrorDesc += _T("；");
		}
		plan.strRuleErrorDesc += strAllSoftDes;
	}

	//顺带填好旧的 B / C 字段，保证只认两三人方案的老代码取值不会越界
	plan.strClassNameSrc = plan.vecChainClassName[0];
	plan.iPersonIndex_B = plan.vecChainPersons[1];
	plan.strClassName_B = plan.vecChainClassName[1];
	if (iChainNum > 2)
	{
		plan.iPersonIndex_C = plan.vecChainPersons[2];
		plan.strClassName_C = plan.vecChainClassName[2];
	}

	//文字描述：[吴雯婷] 接 [吴礼宜] 的 A 班 → [方海利] 接 [吴雯婷] 的 C 班 → …，[吴礼宜] 调休
	CString strDesc;
	if (bIsUniversalCell)
	{
		strDesc.Format(_T("%d人联动(末端万能格): "), iChainNum);
	}
	else
	{
		strDesc.Format(_T("%d人联动: "), iChainNum);
	}
	for (int i = 1; i < iChainNum; i++)
	{
		CString strOneStep;
		strOneStep.Format(_T("[%s] 接 [%s] 的 %s 班"),
			g_AllLevel_PersonList[plan.vecChainPersons[i]].strPersonName.c_str(),
			g_AllLevel_PersonList[plan.vecChainPersons[i - 1]].strPersonName.c_str(),
			plan.vecChainClassName[i - 1]);

		if (i > 1)
		{
			strDesc += _T(" → ");
		}
		strDesc += strOneStep;
	}

	CString strEnd;
	strEnd.Format(_T("，[%s] 调休"), g_AllLevel_PersonList[plan.vecChainPersons[0]].strPersonName.c_str());
	strDesc += strEnd;

	//违规方案照样列出来，只在标题上写清楚违反了什么，方便排班的人自己权衡
	if (!plan.bIsRuleValid)
	{
		CString strDescWithErro;
		strDescWithErro.Format(_T("[违规: %s] %s"), plan.strRuleErrorDesc, strDesc);
		strDesc = strDescWithErro;
	}

	plan.strPlanDesc = strDesc;

	ctx.pOutPlans->push_back(plan);
}

//判断把某人挪到新班次是不是"制造了新的违规"
//调用前 main_table 必须已经处于换班后的状态；iClassIndex_Old 为该人原班次，-1 表示原本休息
bool CPIVASScheduleDlg::bChainMoveMakeNewErro(int iPersonIndex, int iDayIndex, int iClassIndex_New, int iClassIndex_Old, CString& strErroDes_Out)
{
	strErroDes_Out = _T("");

	//先看换班后有没有问题
	ClassCheckErroType iErroType_After = HasNO_ERROR;
	CString strErroDes_After;
	bool bIsOk_After = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClassIndex_New, iErroType_After, strErroDes_After);

	//A/T 数量这类软规则不拦截方案，但要把提示带出去显示在方案说明里。
	//带 SoftRuleTipPrefix 前缀，MakeOneChainPlan 据此区分"硬违规"和"只是提示"。
	CString strSoftTip_After;
	if (!g_strSoftRuleWarn.IsEmpty())
	{
		strSoftTip_After = CString(SoftRuleTipPrefix) + g_strSoftRuleWarn;
	}

	if (bIsOk_After)
	{
		strErroDes_Out = strSoftTip_After;
		return false;		//换过去没有硬性违规
	}

	//换班后不合规，再退回换班前的状态校验一次，看这个问题是不是本来就存在。
	//像"本轮已经连上5天""3-4~3-7已经排满"这种历史遗留问题，与本次换班无关，
	//不能因此把这个人整个排除在换班之外。
	int iBaseIndex = iDayIndex * OneDayClasssNum;

	int iSaveValue_New = main_table[0][iPersonIndex][iBaseIndex + iClassIndex_New];
	int iSaveValue_Old = 0;
	if (iClassIndex_Old >= 0)
	{
		iSaveValue_Old = main_table[0][iPersonIndex][iBaseIndex + iClassIndex_Old];
	}

	main_table[0][iPersonIndex][iBaseIndex + iClassIndex_New] = 0;
	if (iClassIndex_Old >= 0)
	{
		main_table[0][iPersonIndex][iBaseIndex + iClassIndex_Old] = 1;
	}

	ClassCheckErroType iErroType_Before = HasNO_ERROR;
	CString strErroDes_Before;
	int iClassIndex_Check = (iClassIndex_Old >= 0) ? iClassIndex_Old : iClassIndex_New;
	bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClassIndex_Check, iErroType_Before, strErroDes_Before);

	//恢复换班后的状态，交还给调用方继续往下走
	main_table[0][iPersonIndex][iBaseIndex + iClassIndex_New] = iSaveValue_New;
	if (iClassIndex_Old >= 0)
	{
		main_table[0][iPersonIndex][iBaseIndex + iClassIndex_Old] = iSaveValue_Old;
	}

	if (iErroType_Before == iErroType_After)
	{
		strErroDes_Out = strSoftTip_After;		//换班前就有同样的问题，不算这次换班造成的，只把软提示带出去
		return false;
	}

	strErroDes_Out = strErroDes_After;
	if (!strSoftTip_After.IsEmpty())
	{
		strErroDes_Out += _T("；") + strSoftTip_After;
	}
	return true;
}

//链式查找的递归体：iClassNeedCover 是当前空出来、必须有人顶上的班次
void CPIVASScheduleDlg::DoChainSwapDfs(ChainSearchCtx& ctx, int iClassNeedCover)
{
	if (ctx.pOutPlans == nullptr)
	{
		return;
	}
	if ((int)ctx.pOutPlans->size() >= ctx.iMaxPlanNum || ctx.iCheckedNum >= ctx.iMaxCheckNum)
	{
		return;
	}
	if (iClassNeedCover < 0 || iClassNeedCover >= OneDayClasssNum - 1)
	{
		return;
	}

	int iDayIndex = ctx.iDayIndex;
	int iBaseIndex = iDayIndex * OneDayClasssNum;

	//------------------------------------------------------------------
	// 收尾之一(最优先)：把这个空班直接给当天的"万能格"
	// 万能格是用了"删除班次"腾出来的空位，本来就没班没调休名额，
	// 谁都不用还班，所以只要级别能上、规则过得去就直接成方案，
	// 而且允许两个人就闭合(申请人 + 万能格)，不需要中间人。
	//------------------------------------------------------------------
	for (int k = 0; k < (int)ctx.vecUniversalPersons.size(); k++)
	{
		if ((int)ctx.pOutPlans->size() >= ctx.iMaxPlanNum || ctx.iCheckedNum >= ctx.iMaxCheckNum)
		{
			return;
		}

		int iUniversalPerson = ctx.vecUniversalPersons[k];
		if (IsPersonInChainPath(ctx.vecPath, iUniversalPerson))
		{
			continue;
		}

		int iLevel = ctx.vecPersonLevel[iUniversalPerson];
		if (iLevel <= 0 || iLevel > nurse_level)
		{
			continue;
		}

		//人员特例规则：不参与自动联动的人、或规则里禁了这个班次的人，不参与排班链
		{
			CString strPersonName_Chain = g_AllLevel_PersonList[iUniversalPerson].strPersonName.c_str();
			if (!bJudgePersonJoinAutoAdjust(strPersonName_Chain)
				|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName_Chain, iClassNeedCover))
			{
				continue;
			}
		}

		if (NurseCanWorkClassesLevel[iLevel - 1][iClassNeedCover] <= 0)
		{
			continue;		//级别上就上不了这个班
		}

		int iOldValue = main_table[0][iUniversalPerson][iBaseIndex + iClassNeedCover];
		main_table[0][iUniversalPerson][iBaseIndex + iClassNeedCover] = 1;

		CString strErroDes;
		ctx.iCheckedNum++;
		bool bMakeNewErro = bChainMoveMakeNewErro(iUniversalPerson, iDayIndex, iClassNeedCover, -1, strErroDes);

		main_table[0][iUniversalPerson][iBaseIndex + iClassNeedCover] = iOldValue;

		if (!bMakeNewErro || ctx.bAllowRuleErro)
		{
			ctx.vecPathErroDes.push_back(strErroDes);		//不管是硬违规还是软规则提示，都带到方案说明里
			MakeOneChainPlan(ctx, iUniversalPerson, true);
			ctx.vecPathErroDes.pop_back();
		}
	}

	//------------------------------------------------------------------
	// 收尾之二：让当天调休/补休的人把这个空班顶掉，链条闭合
	// 链上至少要有一个中间接班的人，两人直换由阶段1负责，这里不重复产出
	//------------------------------------------------------------------
	if ((int)ctx.vecPath.size() >= 2)
	{
		for (int k = 0; k < (int)ctx.vecRestPersons.size(); k++)
		{
			if ((int)ctx.pOutPlans->size() >= ctx.iMaxPlanNum || ctx.iCheckedNum >= ctx.iMaxCheckNum)
			{
				return;
			}

			int iRestPerson = ctx.vecRestPersons[k];
			if (IsPersonInChainPath(ctx.vecPath, iRestPerson))
			{
				continue;
			}

			int iLevel = ctx.vecPersonLevel[iRestPerson];
			if (iLevel <= 0 || iLevel > nurse_level)
			{
				continue;
			}

			//人员特例规则：不参与自动联动的人、或规则里禁了这个班次的人，不参与排班链
			{
				CString strPersonName_Chain = g_AllLevel_PersonList[iRestPerson].strPersonName.c_str();
				if (!bJudgePersonJoinAutoAdjust(strPersonName_Chain)
					|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName_Chain, iClassNeedCover))
				{
					continue;
				}
			}

			if (NurseCanWorkClassesLevel[iLevel - 1][iClassNeedCover] <= 0)
			{
				continue;		//级别上就上不了这个班，直接跳过
			}

			//试着让他上这个班，再拿真实规则去校验（此时主表已是换班后的假设状态）
			int iOldValue = main_table[0][iRestPerson][iBaseIndex + iClassNeedCover];
			main_table[0][iRestPerson][iBaseIndex + iClassNeedCover] = 1;

			CString strErroDes;
			ctx.iCheckedNum++;
			bool bMakeNewErro = bChainMoveMakeNewErro(iRestPerson, iDayIndex, iClassNeedCover, -1, strErroDes);

			main_table[0][iRestPerson][iBaseIndex + iClassNeedCover] = iOldValue;

			if (!bMakeNewErro || ctx.bAllowRuleErro)
			{
				ctx.vecPathErroDes.push_back(strErroDes);		//不管是硬违规还是软规则提示，都带到方案说明里
				MakeOneChainPlan(ctx, iRestPerson, false);
				ctx.vecPathErroDes.pop_back();
			}
		}
	}

	//------------------------------------------------------------------
	// 延伸：让还在上班的人接下这个班，把他自己的班再空出来，继续往下找
	//------------------------------------------------------------------
	if ((int)ctx.vecPath.size() + 1 >= ctx.iMaxChainPersons)
	{
		return;		//再插一个中间人，就装不下末端顶班的那个人了
	}

	for (int k = 0; k < (int)ctx.vecWorkPersons.size(); k++)
	{
		if ((int)ctx.pOutPlans->size() >= ctx.iMaxPlanNum || ctx.iCheckedNum >= ctx.iMaxCheckNum)
		{
			return;
		}

		int iWorkPerson = ctx.vecWorkPersons[k];
		if (IsPersonInChainPath(ctx.vecPath, iWorkPerson))
		{
			continue;
		}

		int iLevel = ctx.vecPersonLevel[iWorkPerson];
		if (iLevel <= 0 || iLevel > nurse_level)
		{
			continue;
		}

		//人员特例规则：不参与自动联动的人、或规则里禁了这个班次的人，不参与排班链
		{
			CString strPersonName_Chain = g_AllLevel_PersonList[iWorkPerson].strPersonName.c_str();
			if (!bJudgePersonJoinAutoAdjust(strPersonName_Chain)
				|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName_Chain, iClassNeedCover))
			{
				continue;
			}
		}

		if (NurseCanWorkClassesLevel[iLevel - 1][iClassNeedCover] <= 0)
		{
			continue;		//级别上接不了这个班
		}

		int iOwnClass = GetWorkClassIndexOfDay_Safe(iWorkPerson, iDayIndex);
		if (iOwnClass < 0 || iOwnClass == iClassNeedCover)
		{
			continue;
		}
		if (!bJudgeCanRestByClassIndex(iOwnClass))
		{
			continue;		//夜1、夜2、帮夜、3-6、3(10) 这些班不许被换走
		}

		//让他交出自己的班，去顶上家空出来的班
		int iOldValue_Cover = main_table[0][iWorkPerson][iBaseIndex + iClassNeedCover];
		int iOldValue_Own = main_table[0][iWorkPerson][iBaseIndex + iOwnClass];
		main_table[0][iWorkPerson][iBaseIndex + iOwnClass] = 0;
		main_table[0][iWorkPerson][iBaseIndex + iClassNeedCover] = 1;

		CString strErroDes;
		ctx.iCheckedNum++;
		bool bMakeNewErro = bChainMoveMakeNewErro(iWorkPerson, iDayIndex, iClassNeedCover, iOwnClass, strErroDes);

		if (!bMakeNewErro || ctx.bAllowRuleErro)
		{
			ctx.vecPath.push_back(iWorkPerson);
			ctx.vecPathClass.push_back(iOwnClass);
			ctx.vecPathErroDes.push_back(strErroDes);		//不管是硬违规还是软规则提示，都带到方案说明里

			DoChainSwapDfs(ctx, iOwnClass);		//他空出来的班，交给下一层去找人

			ctx.vecPathErroDes.pop_back();
			ctx.vecPath.pop_back();
			ctx.vecPathClass.pop_back();
		}

		//回溯，恢复现场
		main_table[0][iWorkPerson][iBaseIndex + iClassNeedCover] = iOldValue_Cover;
		main_table[0][iWorkPerson][iBaseIndex + iOwnClass] = iOldValue_Own;
	}
}

//N人链式联动换班查找入口
bool CPIVASScheduleDlg::FindChainSwapPlans(int iPersonIndex, int iDayIndex, int iClass_OneDay, vector<int>& vecLevelOrder,
	int iMaxChainPersons, vector<RestAdjustPlan>& outPlans)
{
	outPlans.clear();

	int iAllPersonNum = (int)g_AllLevel_PersonList.size();
	if (iPersonIndex < 0 || iPersonIndex >= iAllPersonNum)
	{
		return false;
	}
	if (iDayIndex < 0 || iDayIndex >= AllDay)
	{
		return false;
	}
	if (iClass_OneDay < 0 || iClass_OneDay >= OneDayClasssNum - 1)
	{
		return false;
	}
	//iMaxChainPersons == 2 时只会产出"把班直接给万能格"这一种两人方案；
	//小于2就没有任何查找的意义了
	if (iMaxChainPersons < 2)
	{
		return false;
	}

	int iBaseIndex = iDayIndex * OneDayClasssNum;

	//查找过程会在主表上直接试算，先把当天整列原样存一份，结束后无条件还原
	vector<int> vecDaySnapshot;
	vecDaySnapshot.resize(iAllPersonNum * OneDayClasssNum, 0);
	for (int p = 0; p < iAllPersonNum; p++)
	{
		for (int c = 0; c < OneDayClasssNum; c++)
		{
			vecDaySnapshot[p * OneDayClasssNum + c] = main_table[0][p][iBaseIndex + c];
		}
	}

	ChainSearchCtx ctx;
	ctx.iDayIndex = iDayIndex;
	ctx.iMaxChainPersons = iMaxChainPersons;
	ctx.iMaxPlanNum = ChainMaxPlanNum_Default;
	ctx.iMaxCheckNum = ChainMaxCheckNum_Default;
	ctx.pOutPlans = &outPlans;

	//预先算好每个人的级别，避免递归内层反复按姓名查找
	ctx.vecPersonLevel.resize(iAllPersonNum, 0);
	for (int p = 0; p < iAllPersonNum; p++)
	{
		CString strPersonName = g_AllLevel_PersonList[p].strPersonName.c_str();
		ctx.vecPersonLevel[p] = FindPesrsonLevelByName(strPersonName);
	}

	//当天的万能格（删除班次腾出来的空位），接班的最优先人选
	GetThisDayUniversalCellPersons(iDayIndex, ctx.vecUniversalPersons);

	//当天调休/补休、可以被拉出来顶班的人（链条的末端）
	GetThisDayRestPersons(iDayIndex, ctx.vecRestPersons, false);

	//当天正常上班、且班次允许被换走的人（链条的中间环节，申请人自己除外）
	for (int p = 0; p < iAllPersonNum; p++)
	{
		if (p == iPersonIndex)
		{
			continue;
		}
		int iOwnClass = GetWorkClassIndexOfDay_Safe(p, iDayIndex);
		if (iOwnClass < 0)
		{
			continue;
		}
		if (!bJudgeCanRestByClassIndex(iOwnClass))
		{
			continue;
		}
		ctx.vecWorkPersons.push_back(p);
	}

	SortPersonListByLevelOrder(ctx.vecUniversalPersons, ctx.vecPersonLevel, vecLevelOrder);
	SortPersonListByLevelOrder(ctx.vecRestPersons, ctx.vecPersonLevel, vecLevelOrder);
	SortPersonListByLevelOrder(ctx.vecWorkPersons, ctx.vecPersonLevel, vecLevelOrder);

	//申请人先把自己的班腾空，链条就从这个空位开始一级级往下找
	//vecPathErroDes[0] 对应申请人本人，他只是变调休，不存在违规，占个空位保证下标对齐
	ctx.vecPath.push_back(iPersonIndex);
	ctx.vecPathClass.push_back(iClass_OneDay);
	ctx.vecPathErroDes.push_back(_T(""));
	main_table[0][iPersonIndex][iBaseIndex + iClass_OneDay] = 0;

	//第一遍严格找：一步都不许新增违规
	ctx.bAllowRuleErro = false;
	DoChainSwapDfs(ctx, iClass_OneDay);

	//一个方案都没找到时放宽再找一遍：违规的也列出来，把原因写在方案标题上，
	//让排班的人自己决定用不用，避免出现"什么都选不了"的死路
	if (outPlans.size() == 0)
	{
		ctx.bAllowRuleErro = true;
		ctx.iCheckedNum = 0;
		DoChainSwapDfs(ctx, iClass_OneDay);
	}

	//无条件还原当天排班，绝不把试算的中间状态留在主表里
	for (int p = 0; p < iAllPersonNum; p++)
	{
		for (int c = 0; c < OneDayClasssNum; c++)
		{
			main_table[0][p][iBaseIndex + c] = vecDaySnapshot[p * OneDayClasssNum + c];
		}
	}

	return (outPlans.size() > 0);
}

bool CPIVASScheduleDlg::OnAdjustRestAndNeeWorkPerson_EX_new(int iPersonIndex, int iDayIndex, int iClass_OneDay, vector<RestAdjustPlan>& outPlans,vector<int>& vecLevelOrder,
	bool bIsRestClass, bool bEnableThreePersonSwap, int iMaxChainPersons)
{
	outPlans.clear();

	CString strPersonName_request = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
	int iPersonLevel_A = FindPesrsonLevelByName(strPersonName_request);

	CString strTempOut;
	CString strOutPut;
	TCHAR strClassName[100] = { 0 };
	STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
	strOutPut.Format(_T("步骤1：对(%s)调休进行处理，第%d天(%s)班次 \r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);
	GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax]);

	// 找到当天调休的人
	vector<int> NoWorkPersonList;
	GetThisDayRestPersons(iDayIndex, NoWorkPersonList, false);

	//当天的万能格（用"删除班次"腾出来的空位）也要当候选人。
	//原来这里只找当天调休/补休的人，万能格只能靠链式查找兑底，
	//客户看到的就是"万能格接不到旁边的班"。
	{
		vector<int> vecUniversalPersons;
		GetThisDayUniversalCellPersons(iDayIndex, vecUniversalPersons);
		for (int k = 0; k < (int)vecUniversalPersons.size(); k++)
		{
			int iUniversalPerson = vecUniversalPersons[k];
			if (iUniversalPerson == iPersonIndex)
			{
				continue;
			}

			bool bHasAdd = false;
			for (int j = 0; j < (int)NoWorkPersonList.size(); j++)
			{
				if (NoWorkPersonList[j] == iUniversalPerson)
				{
					bHasAdd = true;
					break;
				}
			}
			if (!bHasAdd)
			{
				NoWorkPersonList.push_back(iUniversalPerson);
			}
		}
	}

	// 看看这些休息的人哪个能补这个班次
	vector<int> NoWorkPersonCanWorkInThisClass;
	GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);

	int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();


	vector<int> NoWorkPersonCanWorkInThisClass_temp;
	//按照级别顺序找
	if (vecLevelOrder.size() == 4)
	{
		for (int mm = 0; mm < vecLevelOrder.size(); mm++)
		{
			int iLevel_tem = vecLevelOrder[mm];
			for (int jj = 0; jj < NoWorkPersonCanWorkInThisClass.size(); jj++)
			{
				int iPerson_Index_temp = NoWorkPersonCanWorkInThisClass[jj];
				CString strPersonName_temp = g_AllLevel_PersonList[iPerson_Index_temp].strPersonName.c_str();
				int iPersonLevel_temp2 = FindPesrsonLevelByName(strPersonName_temp);
				if (iPersonLevel_temp2 == iLevel_tem)
				{
					NoWorkPersonCanWorkInThisClass_temp.push_back(iPerson_Index_temp);
				}
			}
		}
	}
	else
	{
		NoWorkPersonCanWorkInThisClass_temp = NoWorkPersonCanWorkInThisClass;
	}

	// ======================================================================
	// 阶段 1：寻找【两人直换】的可能方案 (A 和 B 互换)
	// ======================================================================
	if (iFindPersonNum > 0)
	{
		strOutPut.Format(_T("步骤2：进行两人直换 \r\n")); // 去掉多余格式化参数
		GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

		//按照级别顺序进行
		for (int i = 0; i < iFindPersonNum; i++)
		{
			int iPersonIndex_temp = NoWorkPersonCanWorkInThisClass_temp[i];
			CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();

			// 获取B原先的班次状态(通常为调休/休)
			int iClassIndexOneDay_B = -1;
			int iClass_ChangeRelaxValue_B = 0;
			CString strClassName_B = GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex_temp, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
			if (strClassName_B.IsEmpty()) {
				strClassName_B = _T("休");
			}

			// 每次测试前，初始化一次干净的临时表
			ChangeCurrentSheduleToTempTable();

			int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
			main_table_temp[0][iPersonIndex_temp][iClassIndex] = 2; // B试着上A的班

			ClassCheckErroType iErrorType;
			CString strErroDes;
			bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType, strErroDes);
			CString strSoftWarn_B = g_strSoftRuleWarn;		//软规则(A/T数量)提示，不拦截方案

			//if (bIsChekOK)
			{
				// 记录成功的方案
				RestAdjustPlan plan;
				plan.iPlanType = 2;
				plan.iPersonIndex_B = iPersonIndex_temp;
				plan.iPersonIndex_C = -1;

				// 赋值完整的上下文数据
				plan.iPerson_Source = iPersonIndex;
				plan.iDayIndex_Source = iDayIndex;
				plan.strClassNameSrc = strClassName;
				plan.strClassName_B = strClassName_B;
				plan.strClassName_C = _T("");

				//接班的是不是万能格：万能格本来就没有调休名额，接了班就是正常上班，
				//不能像调休人员那样被标成"调休转上班"
				plan.bEndIsUniversalCell = bJudgeIsUniversalCell(iPersonIndex_temp, iDayIndex);

				// 存入合规状态和具体违规描述
				plan.bIsRuleValid = bIsChekOK;
				plan.strRuleErrorDesc = strErroDes;

				//A/T 数量这类"只提示不拦截"的软规则：方案照样成立，但提示要带到界面上。
				//客户反馈的"把T班换给已经有3个T的人、系统一声不吭说成功了"，就是漏了这一步。
				CString strSoftDes_B = MakeSoftWarnDesc(strPersonName_Relax_Init, strSoftWarn_B);
				if (!strSoftDes_B.IsEmpty())
				{
					if (plan.strRuleErrorDesc.IsEmpty())
					{
						plan.strRuleErrorDesc = strSoftDes_B;
					}
					else
					{
						plan.strRuleErrorDesc += _T("；") + strSoftDes_B;
					}
				}

				//plan.strPlanDesc.Format(_T("两人换班: [%s] 顶替 [%s] 上 %s 班"),
				//	strPersonName_Relax_Init.GetBuffer(), strPersonName_request.GetBuffer(), strClassName);

				CString strUniversalTip = plan.bEndIsUniversalCell ? _T("(万能格)") : _T("");

				if (bIsChekOK)
				{
					plan.strPlanDesc.Format(_T("两人换班: [%s]%s 顶替 [%s] 上 %s 班"),
						strPersonName_Relax_Init.GetBuffer(), strUniversalTip.GetBuffer(), strPersonName_request.GetBuffer(), strClassName);
					strTempOut.Format(_T("步骤4：发现可行方案 - %s\r\n"), plan.strPlanDesc.GetBuffer());
				}
				else
				{
					// 若违规，在描述前端加上显眼的违规提示，方便UI直接展示给排班人员
					plan.strPlanDesc.Format(_T("[违规: %s] 两人换班: [%s] 顶替 [%s] 上 %s 班"),
						strErroDes.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_request.GetBuffer(), strClassName);
					strTempOut.Format(_T("步骤4：发现违规方案 - %s\r\n"), plan.strPlanDesc.GetBuffer());
				}

				outPlans.push_back(plan);

				strTempOut.Format(_T("步骤4：发现可行方案 - %s\r\n"), plan.strPlanDesc.GetBuffer());
				strOutPut += strTempOut;
			}
		}
	}

	// ======================================================================
	// 阶段 2：寻找【三人轮换】的可能方案 (A -> C -> B)
	// 根据外部传参决定是否进行这种深度的搜索
	// ======================================================================
// ======================================================================
	// 阶段 2：寻找【三人轮换】的可能方案 (A -> C -> B)
	// ======================================================================
	if (bEnableThreePersonSwap)
	{
		vector<int> CandidateCList;
		bool bIsSpecial = JudgeIsSpecialClass(iClass_OneDay);

		if (bIsSpecial)
		{
			// 如果是特殊班次，寻找当天上普通班的人员作为 C
			for (int i = 0; i < g_AllLevel_PersonList.size(); i++)
			{
				if (i == iPersonIndex) continue; // 跳过申请人自身
				int cClass = GetPersonOneDayWorkClassIndex(i, iDayIndex);
				bool isNormal = false;
				for (int k = 0; k < NormalClasss1Num; k++)
				{
					if (cClass == g_arrGeneral_1Classes[k])
					{
						isNormal = true;
						break;
					}
				}
				if (isNormal)
				{
					CandidateCList.push_back(i);
				}
			}
		}
		else
		{
			// 如果不是特殊班次，走原有逻辑：寻找原本调休但被指派了工作的人作为 C
			GetThisDayRelaxToWorkPersons(iDayIndex, CandidateCList);
		}

		if (NoWorkPersonList.size() > 0)
		{
			// 使用 pass=0 (同级别) 和 pass=1 (不同级别) 来分优先级提取方案
			for (int pass = 0; pass < 2; pass++)
			{
				// 如果不是特殊班次，无需做优先级拆分，仅执行一遍即可
				if (!bIsSpecial && pass == 1) break;

				// 遍历所有当前休息的人 (B)
				for (int n = 0; n < NoWorkPersonList.size(); n++)
				{
					int iPerson_A_Index = iPersonIndex;
					int iPerson_B_Index = NoWorkPersonList[n];
					CString strPersonName_B = g_AllLevel_PersonList[iPerson_B_Index].strPersonName.c_str();
					int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_B);

					// 遍历所有候选的 C
					for (int m = 0; m < CandidateCList.size(); m++)
					{
						int iPerson_C_Index = CandidateCList[m];
						CString strPersonName_C = g_AllLevel_PersonList[iPerson_C_Index].strPersonName.c_str();
						int iPersonLevel_C = FindPesrsonLevelByName(strPersonName_C);

						// 级别优先级过滤：针对特殊班次寻找C时生效
						if (bIsSpecial)
						{
							bool isSameLevel = (iPersonLevel_C == iPersonLevel_A);
							if (pass == 0 && !isSameLevel) continue; // 第一遍只找同级别
							if (pass == 1 && isSameLevel) continue;  // 第二遍找不同级别
						}

						int iClassIndexOneDay_A = -1, iClass_ChangeRelaxValue_A = 0;
						int iClassIndexOneDay_B = -1, iClass_ChangeRelaxValue_B = 0;
						int iClassIndexOneDay_C = -1, iClass_ChangeRelaxValue_C = 0;

						GetClassNameOneDayOnePerson(iDayIndex, iPerson_A_Index, iClassIndexOneDay_A, iClass_ChangeRelaxValue_A);

						CString strClassName_B = GetClassNameOneDayOnePerson(iDayIndex, iPerson_B_Index, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
						if (strClassName_B.IsEmpty()) strClassName_B = _T("休");

						CString strClassName_C = GetClassNameOneDayOnePerson(iDayIndex, iPerson_C_Index, iClassIndexOneDay_C, iClass_ChangeRelaxValue_C);

						// 初步等级拦截：B必须能上C的班，C必须能上A的班
						if (NurseCanWorkClassesLevel[iPersonLevel_B - 1][iClassIndexOneDay_C] > 0 &&
							NurseCanWorkClassesLevel[iPersonLevel_C - 1][iClassIndexOneDay_A] > 0)
						{
							// 每次测试前，初始化干净的临时表
							ChangeCurrentSheduleToTempTable();

							// 1. 让 B 上 C 的班次
							int iClassIndex_C_Full = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
							main_table_temp[0][iPerson_B_Index][iClassIndex_C_Full] = 2;
							main_table_temp[0][iPerson_C_Index][iClassIndex_C_Full] = 0;

							ClassCheckErroType errType1;
							CString errDesc1;
							bool bIsChekOK_B = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_C_Full, errType1, errDesc1);
							CString strSoftWarn_B = g_strSoftRuleWarn;		//软规则提示，不拦截方案

							if (bIsChekOK_B)
							{
								// 2. 既然B没问题，继续让 C 上 A 的班次
								int iClassIndex_A_Full = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
								main_table_temp[0][iPerson_C_Index][iClassIndex_A_Full] = 2;
								main_table_temp[0][iPerson_A_Index][iClassIndex_A_Full] = 0;

								ClassCheckErroType errType2;
								CString errDesc2;
								bool bIsChekOK_C = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A_Full, errType2, errDesc2);
								CString strSoftWarn_C = g_strSoftRuleWarn;		//软规则提示，不拦截方案

								if (bIsChekOK_C)
								{
									// 记录成功的三人方案
									RestAdjustPlan plan;
									plan.iPlanType = 3;
									plan.iPersonIndex_B = iPerson_B_Index;
									plan.iPersonIndex_C = iPerson_C_Index;

									// 赋值完整的上下文数据
									plan.iPerson_Source = iPersonIndex;
									plan.iDayIndex_Source = iDayIndex;
									plan.strClassNameSrc = strClassName;
									plan.strClassName_B = strClassName_B;
									plan.strClassName_C = strClassName_C;

									//A/T 数量这类"只提示不拦截"的软规则：方案照样成立，但要把提示带到界面上，
									//否则客户把 T 班换给已经有3个T的人，系统一声不吭就说成功了。
									plan.bIsRuleValid = true;
									plan.strRuleErrorDesc = MakeSoftWarnDesc(strPersonName_B, strSoftWarn_B);
									CString strSoftDes_C = MakeSoftWarnDesc(strPersonName_C, strSoftWarn_C);
									if (!strSoftDes_C.IsEmpty())
									{
										if (!plan.strRuleErrorDesc.IsEmpty())
										{
											plan.strRuleErrorDesc += _T("；");
										}
										plan.strRuleErrorDesc += strSoftDes_C;
									}

									// 如果是特殊班次的方案，添加直观前缀标识便于UI展示
									CString strPrefix = (pass == 0 && bIsSpecial) ? _T("[优先同级] ") : _T("");

									plan.strPlanDesc.Format(_T("%s三人换班: [%s] 顶替 [%s](%s), [%s] 顶替 [%s](%s)"),
										strPrefix.GetBuffer(),
										strPersonName_B.GetBuffer(), strPersonName_C.GetBuffer(), strClassName_C.GetBuffer(),
										strPersonName_C.GetBuffer(), strPersonName_request.GetBuffer(), strClassName);
									outPlans.push_back(plan);

									strTempOut.Format(_T("步骤6：发现可行三人方案 - %s\r\n"), plan.strPlanDesc.GetBuffer());
									strOutPut += strTempOut;
								}
							}
						}
					}
				}
			}
		}
	}

	// ======================================================================
	// 阶段 3：寻找【N人链式联动】的可能方案 (A -> B -> C -> ... -> 当天调休的人/万能格)
	// 3人以上的链条专治 A、T、B、C、D、Z 这类级别受限、两三人换不动的班次；
	// iMaxChainPersons == 2 时这里只会产出"把班直接给万能格"这一种两人方案，
	// 所以普通班次也必须进来，否则万能格根本不会被搜索到。
	// ======================================================================
	if (iMaxChainPersons >= 2)
	{
		strTempOut.Format(_T("步骤7：进行N人链式联动查找(最多%d人一条链) \r\n"), iMaxChainPersons);
		strOutPut += strTempOut;
		GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

		vector<RestAdjustPlan> vecChainPlans;
		FindChainSwapPlans(iPersonIndex, iDayIndex, iClass_OneDay, vecLevelOrder, iMaxChainPersons, vecChainPlans);

		int iUniversalPlanNum = 0;		//已经插到最前面的万能格方案个数
		for (int i = 0; i < (int)vecChainPlans.size(); i++)
		{
			RestAdjustPlan& theChainPlan = vecChainPlans[i];

			//三人链有可能和阶段2已经找到的三人轮换重复，重复的就不再列一遍
			bool bHasSamePlan = false;
			if (theChainPlan.vecChainPersons.size() == 3)
			{
				for (int j = 0; j < (int)outPlans.size(); j++)
				{
					if (outPlans[j].iPlanType == 3
						&& outPlans[j].iPersonIndex_C == theChainPlan.vecChainPersons[1]
						&& outPlans[j].iPersonIndex_B == theChainPlan.vecChainPersons[2])
					{
						bHasSamePlan = true;
						break;
					}
				}
			}
			if (bHasSamePlan)
			{
				continue;
			}

			if (theChainPlan.bEndIsUniversalCell)
			{
				//万能格是专门腾出来接班的空位，优先级最高，插到方案列表最前面
				outPlans.insert(outPlans.begin() + iUniversalPlanNum, theChainPlan);
				iUniversalPlanNum++;
			}
			else
			{
				outPlans.push_back(theChainPlan);
			}

			strTempOut.Format(_T("步骤8：发现可行联动方案 - %s\r\n"), theChainPlan.strPlanDesc.GetBuffer());
			strOutPut += strTempOut;
		}
	}

	GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	// 只要找到至少一种方案即返回 true
	return (outPlans.size() > 0);
}
void CPIVASScheduleDlg::GetOnePersonRelaxDays(int iPersonIndex, vector<int> &vecRelaxDays)
{
	for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		bool bHasWork = false;
		for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum - 1; iClassIndex++)
		{
			if (main_table[0][iPersonIndex][iClassIndex] >0 )
			{
				bHasWork = true;
				break;
			}
		}
		if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == Relax_INIT) //如果是调休人员
		{
			bHasWork = true;
		}
		if (!bHasWork)
		{
			vecRelaxDays.push_back(iDayIndex);
		}
	}
}


bool CPIVASScheduleDlg::ChangeTwoDayClass(int iPersonIndex, CString strPersonName_request, int iDayIndex, int iClass_OneDay, CString strClassName, int iPersonIndex_temp, vector<int>& vecRelaxDays, CString &strOutPut, RelaxEnumType iRelaxType)
{
	bool bIsSucccess = false;
	CString strTempOut;

	
	CString strPersonName_temp = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();
	for (int mm = 0; mm < vecRelaxDays.size(); mm++)
	{
		int iDayIndex_temp = vecRelaxDays[mm];

		int iClass_ChangeRelaxValue_temp = 0;
		int iClassIndexOneDay_temp = 0;
		GetClassNameOneDayOnePerson(iDayIndex_temp, iPersonIndex_temp, iClassIndexOneDay_temp, iClass_ChangeRelaxValue_temp);
		if (iClassIndexOneDay_temp == iClass_OneDay)
		{
			
			int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
			main_table_temp[0][iPersonIndex][iClassIndex] = 0;
			main_table_temp[0][iPersonIndex_temp][iClassIndex] = 1;

			int iClassIndex_temp = iClassIndexOneDay_temp + iDayIndex_temp * OneDayClasssNum;

			main_table_temp[0][iPersonIndex][iClassIndex_temp] = 1;
			main_table_temp[0][iPersonIndex_temp][iClassIndex_temp] = 0;


			//
			ClassCheckErroType iErrorType1;
			ClassCheckErroType iErrorType2;
			CString strErroDes1;
			CString strErroDes2;
			bool bIsChekOK1 = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClass_OneDay, iErrorType1, strErroDes1);
			bool bIsChekOK2 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType2, strErroDes2);

			CString strErroDes3;
			CString strErroDes4;
			bool bIsChekOK3 = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType1, strErroDes3);
			bool bIsChekOK4 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2, strErroDes4);

			if (bIsChekOK1 && bIsChekOK2 && bIsChekOK3 && bIsChekOK4)
			{
				//调休成功
				int iClassIndex_relax = Class_ChangeRelax + iPersonIndex * OneDayClasssNum;

				if (iRelaxType == Relax_TO_WORK)
				{
					main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
				}
				else
				{
					main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
				}
				main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = Relax_TO_WORK;

				//ChangeTempSheduleToCurrentTable();


				strTempOut.Format(_T("步骤3：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,更换成功\r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp);
				//ShowAllPersonToList(false, false, true);

				bIsSucccess = true;
				return bIsSucccess;
			}
			else
			{
				//替换失败
				strTempOut.Format(_T("步骤3：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,更换失败\r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp);
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				bIsSucccess = false;
			}
		}
		else if (iClassIndexOneDay_temp >= 0 && bJudgeCanRestByClassIndex(iClassIndexOneDay_temp))
		{
			//若不是期望的班次，则需要找到当天上该班次的人，将调休申请者(A)、当天班次者(B)，还有调休的人(C)，进行更换，调休的人的班次给当天班次者，当天班次者给调休申请者
			//iDayIndex_temp :B->A  C->B
			//iDayIndex :A->C
			int iPersonIndex_A = iPersonIndex; //调休申请者
			CString strPersonName_A = strPersonName_request;


			int iPersonIndex_B = -1; //当天班次者
			CString strPersonIndex_B;
			GetPersonOneDayWorkInOneClase(iDayIndex_temp, iClass_OneDay, iPersonIndex_B);
			if (iPersonIndex_B == -1)
			{
				continue;
			}
			int iPersonLevel_B = FindPesrsonLevelByName(g_AllLevel_PersonList[iPersonIndex_B].strPersonName.c_str());
			//判断B能不能上C的班次
			if (NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_temp] <= 0)
			{
				continue; //B不能上C的班次
			}
			strPersonIndex_B = g_AllLevel_PersonList[iPersonIndex_B].strPersonName.c_str();

			int iPersonIndex_C = iPersonIndex_temp; //调休的人
			CString strPersonName_C = strPersonName_temp;




			//如果找到了尝试更换
			ChangeCurrentSheduleToTempTable();
			//iDayIndex :A->C
			int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
			main_table_temp[0][iPersonIndex_A][iClassIndex] = 0;
			main_table_temp[0][iPersonIndex_C][iClassIndex] = 1;


			//iDayIndex_temp :B->A  C->B
			int iClassIndex_temp_B = iClass_OneDay + iDayIndex_temp * OneDayClasssNum;
			int iClassIndex_temp_C = iClassIndexOneDay_temp + iDayIndex_temp * OneDayClasssNum;
			main_table_temp[0][iPersonIndex_A][iClassIndex_temp_B] = 1;
			main_table_temp[0][iPersonIndex_B][iClassIndex_temp_B] = 0;
			main_table_temp[0][iPersonIndex_B][iClassIndex_temp_C] = 1;
			main_table_temp[0][iPersonIndex_C][iClassIndex_temp_C] = 0;

			//
			ClassCheckErroType iErrorType1;
			ClassCheckErroType iErrorType2;
			CString strErroDes1;
			CString strErroDes2;
			bool bIsChekOK1 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex, iClass_OneDay, iErrorType1, strErroDes1);
			bool bIsChekOK2 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_C, iDayIndex, iClass_OneDay, iErrorType2, strErroDes2);
			CString strErroDes3;
			CString strErroDes4;
			CString strErroDes5;
			bool bIsChekOK3 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex_temp, iClass_OneDay, iErrorType1, strErroDes3);
			bool bIsChekOK4 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_C, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2, strErroDes4);
			bool bIsChekOK5 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_B, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2, strErroDes5);

			bool bChange_Result = bIsChekOK1 && bIsChekOK2 && bIsChekOK3 && bIsChekOK4 && bIsChekOK5;

			if (bChange_Result)
			{

				strTempOut.Format(_T("步骤4：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,第%d天还班时，与%s(%s)进行更换，更换成功\r\n"),
					strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp, iDayIndex_temp, strPersonIndex_B.GetBuffer(), strClassName);
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


				//调休成功
				int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

				if (iRelaxType == Relax_TO_WORK)
				{
					main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
				}
				else
				{
					main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
				}
				
				
				//main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;



				//调休的人状态置成Relax_TO_WORK
				main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = Relax_TO_WORK;

				int iClassIndex_relax_2 = Class_ChangeRelax + iDayIndex_temp * OneDayClasssNum;
				//调休的人还班那天改成调休
				main_table_temp[0][iPersonIndex_temp][iClassIndex_relax_2] = Relax_INIT;



				ChangeTempSheduleToCurrentTable();

				//ShowAllPersonToList(false, false, true);
				bIsSucccess = true;
				return bIsSucccess;
			}
			else
			{
				//替换失败
				strTempOut.Format(_T("步骤4：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,第%d天还班时，与%s(%s)进行更换，更换失败\r\n"),
					strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp, iDayIndex_temp, strPersonIndex_B.GetBuffer(), strClassName);
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				bIsSucccess = false;
			}
		}
	}

	return bIsSucccess;
}


bool CPIVASScheduleDlg::OnAdjustRestAndNeeWorkPerson_NeedReturnClass(int iPersonIndex, int iDayIndex, int iClass_OneDay)
{
	//首先看看当前有没有空闲地调休人员
	bool bIsSucccess = true;
	//对需要调休的人员进行处理	
	CString strPersonName_request = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
	int iPersonLevel_A = FindPesrsonLevelByName(strPersonName_request);

	CString strTempOut;
	CString strOutPut;
	TCHAR strClassName[100] = { 0 };
	STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
	strOutPut.Format(_T("步骤1：对(%s)调休进行处理，第%d天(%s)班次 需要还班 \r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);

	GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax]); //调休类型


	//查找该人的休息日期
	vector<int> vecRelaxDays;
	GetOnePersonRelaxDays(iPersonIndex, vecRelaxDays);


	//查看当天谁能补
	//找到当天调休的人
	vector<int> NoWorkPersonList;
	GetThisDayRestPersons(iDayIndex, NoWorkPersonList);

	if (NoWorkPersonList.size() == 0)
	{
		strTempOut.Format(_T("步骤2：对(%s)第%d天(%s)班次,调休进行处理，未找到能调休的人，不允许调休"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);
		strOutPut += strTempOut;
		GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
		bIsSucccess = false;
		return bIsSucccess;
	}


	//看看这些休息的人哪个能补这个班次
	vector<int> NoWorkPersonCanWorkInThisClass;
	GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, NoWorkPersonList, NoWorkPersonCanWorkInThisClass);


	//剩余调休的人不能上该班次，则继续找已经调休的人是否能上该班次
	vector<int> RelaxToWorkPersonPersonList;
	GetThisDayRelaxToWorkPersons(iDayIndex, RelaxToWorkPersonPersonList);

	vector<int>  RelaxToWorkPersonCanWorkInThisClass;
	GetThisDayCanWorPersons(iDayIndex, iClass_OneDay, RelaxToWorkPersonPersonList, RelaxToWorkPersonCanWorkInThisClass);


	int iFindPersonNum = NoWorkPersonCanWorkInThisClass.size();
	if (iFindPersonNum == 0)
	{
		if (RelaxToWorkPersonCanWorkInThisClass.size() == 0)
		{
			strTempOut.Format(_T("步骤3：对(%s)第%d天(%s)班次,调休进行处理，尝试对当天所有调休的人更换，未找到能调休的人，不允许调休"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);
			strOutPut += strTempOut;
			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
			bIsSucccess = false;
			return bIsSucccess;
		}
	}




	//判断能上该班次的人，在被调休的人休息的那天是不是上3-4、3-5、3-7班次

	int iChangeOK = false;
	for (int kk = 0; kk < NoWorkPersonCanWorkInThisClass.size(); kk++)
	{
		int iPersonIndex_temp = NoWorkPersonCanWorkInThisClass[kk];
		CString strPersonName_temp = g_AllLevel_PersonList[iPersonIndex_temp].strPersonName.c_str();
		
		//如果找到了尝试更换
		ChangeCurrentSheduleToTempTable();

		bIsSucccess = ChangeTwoDayClass(iPersonIndex, strPersonName_request, iDayIndex, iClass_OneDay, strClassName, iPersonIndex_temp, vecRelaxDays, strOutPut, iRelaxType);
		
		if (bIsSucccess)
		{

			//将班次替换
			ChangeTempSheduleToCurrentTable();
			//ShowAllPersonToList(false, false, true);

			return bIsSucccess;
		}
		
		//for (int mm = 0; mm < vecRelaxDays.size(); mm++)
		//{
		//	int iDayIndex_temp = vecRelaxDays[mm];

		//	int iClass_ChangeRelaxValue_temp = 0;
		//	int iClassIndexOneDay_temp = 0;
		//	GetClassNameOneDayOnePerson(iDayIndex_temp, iPersonIndex_temp, iClassIndexOneDay_temp, iClass_ChangeRelaxValue_temp);
		//	if (iClassIndexOneDay_temp == iClass_OneDay)
		//	{
		//		//如果找到了尝试更换
		//		ChangeCurrentSheduleToTempTable();
		//		int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
		//		main_table_temp[0][iPersonIndex][iClassIndex] = 0;
		//		main_table_temp[0][iPersonIndex_temp][iClassIndex] = 1;

		//		int iClassIndex_temp = iClassIndexOneDay_temp + iDayIndex_temp * OneDayClasssNum;

		//		main_table_temp[0][iPersonIndex][iClassIndex_temp] = 1;
		//		main_table_temp[0][iPersonIndex_temp][iClassIndex] = 0;


		//		//
		//		ClassCheckErroType iErrorType1;
		//		ClassCheckErroType iErrorType2;
		//		bool bIsChekOK1 = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClass_OneDay, iErrorType1);
		//		bool bIsChekOK2 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex, iClass_OneDay, iErrorType2);


		//		bool bIsChekOK3 = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType1);
		//		bool bIsChekOK4 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_temp, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2);

		//		if (bIsChekOK1 && bIsChekOK2 && bIsChekOK3 && bIsChekOK4)
		//		{
		//			//调休成功
		//			int iClassIndex_relax = Class_ChangeRelax + iPersonIndex * OneDayClasssNum;

		//			if (iRelaxType == Relax_TO_WORK)
		//			{
		//				main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
		//			}
		//			else
		//			{
		//				main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
		//			}
		//			main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = Relax_TO_WORK;

		//			ChangeTempSheduleToCurrentTable();


		//			strTempOut.Format(_T("步骤3：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,更换成功"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp);
		//			ShowAllPersonToList(false, false, true);

		//			iChangeOK = true;
		//			bIsSucccess = true;
		//			return bIsSucccess;
		//		}
		//		else
		//		{
		//			//替换失败
		//			strTempOut.Format(_T("步骤3：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,更换失败"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp);
		//			strOutPut += strTempOut;
		//			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
		//			bIsSucccess = false;
		//		}
		//	}
		//	else
		//	{
		//		//若不是期望的班次，则需要找到当天上该班次的人，将调休申请者(A)、当天班次者(B)，还有调休的人(C)，进行更换，调休的人的班次给当天班次者，当天班次者给调休申请者
		//		//iDayIndex_temp :B->A  C->B
		//		//iDayIndex :A->C
		//		int iPersonIndex_A	= iPersonIndex; //调休申请者
		//		CString strPersonName_A = strPersonName_request;


		//		int iPersonIndex_B = -1; //当天班次者
		//		CString strPersonIndex_B;
		//		GetPersonOneDayWorkInOneClase(iDayIndex, iClass_OneDay, iPersonIndex_B);
		//		if (iPersonIndex_B == -1 )
		//		{
		//			continue;
		//		}
		//		int iPersonLevel_B = FindPesrsonLevelByName(g_AllLevel_PersonList[iPersonIndex_B].c_str());
		//		//判断B能不能上C的班次
		//		if (NurseCanWorkClassesLevel[iPersonLevel_B][iClassIndexOneDay_temp] <= 0)
		//		{
		//			continue; //B不能上C的班次
		//		}
		//		strPersonIndex_B = g_AllLevel_PersonList[iPersonIndex_B].c_str();

		//		int iPersonIndex_C = iPersonIndex_temp; //调休的人
		//		CString strPersonName_C = strPersonName_temp;

		//		


		//		//如果找到了尝试更换
		//		ChangeCurrentSheduleToTempTable();
		//		//iDayIndex :A->C
		//		int iClassIndex = iClass_OneDay + iDayIndex * OneDayClasssNum;
		//		main_table_temp[0][iPersonIndex_A][iClassIndex] = 0;
		//		main_table_temp[0][iPersonIndex_C][iClassIndex] = 1;


		//		//iDayIndex_temp :B->A  C->B
		//		int iClassIndex_temp_B = iClass_OneDay + iDayIndex_temp * OneDayClasssNum;
		//		int iClassIndex_temp_C = iClassIndexOneDay_temp + iDayIndex_temp * OneDayClasssNum;
		//		main_table_temp[0][iPersonIndex_A][iClassIndex_temp_B] = 1;
		//		main_table_temp[0][iPersonIndex_B][iClassIndex_temp_C] = 1;
		//		main_table_temp[0][iPersonIndex_C][iClassIndex_temp_C] = 0;

		//		//
		//		ClassCheckErroType iErrorType1;
		//		ClassCheckErroType iErrorType2;
		//		bool bIsChekOK1 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex, iClass_OneDay, iErrorType1);
		//		bool bIsChekOK2 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_C, iDayIndex, iClass_OneDay, iErrorType2);


		//		bool bIsChekOK3 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex_temp, iClass_OneDay, iErrorType1);
		//		bool bIsChekOK4 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_C, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2);
		//		bool bIsChekOK5 = bCheckOnePersonOneDayClassIsOk(iPersonIndex_B, iDayIndex_temp, iClassIndexOneDay_temp, iErrorType2);

		//		bool bChange_Result = bIsChekOK1&& bIsChekOK2 && bIsChekOK3 && bIsChekOK4 && bIsChekOK5;

		//		if (bChange_Result)
		//		{

		//			strTempOut.Format(_T("步骤4：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,第%d天还班时，与%s(%s)进行更换，更换成功"),
		//				strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp, iDayIndex_temp, strPersonIndex_B.GetBuffer(), strClassName);
		//			strOutPut += strTempOut;
		//			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


		//			//调休成功
		//			int iClassIndex_relax = Class_ChangeRelax + iPersonIndex * OneDayClasssNum;

		//			if (iRelaxType == Relax_TO_WORK)
		//			{
		//				main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_INIT;
		//			}
		//			else
		//			{
		//				main_table_temp[0][iPersonIndex][iClassIndex_relax] = WORK_TO_Relax;
		//			}
		//			main_table_temp[0][iPersonIndex_temp][iClassIndex_relax] = Relax_TO_WORK;
		//			ChangeTempSheduleToCurrentTable();

		//			ShowAllPersonToList(false, false, true);

		//		}
		//		else
		//		{
		//			//替换失败
		//			strTempOut.Format(_T("步骤4：对(%s)第%d天(%s)班次,调休进行处理，与%s进行更换,第%d天还班时，与%s(%s)进行更换，更换失败"), 
		//				strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName, strPersonName_temp, iDayIndex_temp, strPersonIndex_B.GetBuffer(), strClassName);
		//			strOutPut += strTempOut;
		//			GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
		//			bIsSucccess = false;
		//		}
		//	}
		//}

	}



	
	//若上述未找到，但是当前仍然有调休的人，则尝试进行三人之间的换班
	if (!bIsSucccess && NoWorkPersonList.size() > 0)
	{

		bool bCanSuccessChange = false;

		//继续检查是否有原来是调休，但是被指派班次的人，若存在这些人，判断这个人能不能上调休人的班，而剩余的调休班能不能上这个该人的班
		//即A-申请调休的人-RequestPerson  B-当前是调休的人-Relax_InitPerson  C-原来是调休但是被安排了班次的人-RelaxToWorkPerson--会有多个
		//若C能上A的班，且B能上C的班，则可以调休，B上C，C上A的班

		for (int n = 0; n < NoWorkPersonList.size(); n++)
		{
			int iPerson_A_Index = iPersonIndex; //申请调休的人
			int iPerson_B_Index = NoWorkPersonList[n]; //当前是调休的人
			CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPerson_B_Index].strPersonName.c_str();


			for (int m = 0; m < RelaxToWorkPersonPersonList.size(); m++)
			{
				int iPerson_C_Index = RelaxToWorkPersonPersonList[m]; //原先是调休但是被安排了班次的人
				CString strPersonName_RelaxToWork = g_AllLevel_PersonList[iPerson_C_Index].strPersonName.c_str();

				strTempOut.Format(_T("步骤5：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s \r\n"),
					strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
				strOutPut += strTempOut;
				GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

				//先判断B能不能上C的班次，再判断C能不能上A的班次
				int iClassIndexOneDay_A = -1;
				int iClassIndexOneDay_B = -1;
				int iClassIndexOneDay_C = -1;
				int iClass_ChangeRelaxValue_A = 0;
				int iClass_ChangeRelaxValue_B = 0;
				int iClass_ChangeRelaxValue_C = 0;

				int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_Relax_Init);
				int iPersonLevel_C = FindPesrsonLevelByName(strPersonName_RelaxToWork);

				GetClassNameOneDayOnePerson(iDayIndex, iPerson_A_Index, iClassIndexOneDay_A, iClass_ChangeRelaxValue_A);
				GetClassNameOneDayOnePerson(iDayIndex, iPerson_B_Index, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
				GetClassNameOneDayOnePerson(iDayIndex, iPerson_C_Index, iClassIndexOneDay_C, iClass_ChangeRelaxValue_C);


				ChangeCurrentSheduleToTempTable();

				//判断B能不能上C的班次
				//判断C能不能上A的班次
				if (NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_C] > 0 &&
					NurseCanWorkClassesLevel[iPersonLevel_C-1][iClassIndexOneDay_A] > 0)
				{
					//把C的班次给B
					int iClassIndex_C = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
					main_table_temp[0][iPerson_B_Index][iClassIndex_C] = 2;
					main_table_temp[0][iPerson_C_Index][iClassIndex_C] = 0;

					//检查是否合格
					ClassCheckErroType iErrorType;
					CString strErroDes;
					bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_C, iErrorType, strErroDes);
					if (!bIsChekOK)
					{
						//替换失败
						strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败 C->B失败 \r\n"),
							strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
						strOutPut += strTempOut;
						GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
					}
					else
					{
						//把A的班次给C
						int iClassIndex_A = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
						main_table_temp[0][iPerson_C_Index][iClassIndex_A] = 2;
						main_table_temp[0][iPerson_A_Index][iClassIndex_A] = 0;

						//检查是否合格
						ClassCheckErroType iErrorType;
						CString strErroDes;
						bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A, iErrorType, strErroDes);
						if (!bIsChekOK)
						{
							//替换失败
							strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败 A->C失败 \r\n"),
								strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
						}
						else
						{

							bIsSucccess = ChangeTwoDayClass(iPersonIndex, strPersonName_request, iDayIndex, iClass_OneDay, strClassName, iPerson_B_Index, vecRelaxDays, strOutPut, iRelaxType);

							if (bIsSucccess)
							{

								int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

								main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
								main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

								//将班次替换
								ChangeTempSheduleToCurrentTable();
								//ShowAllPersonToList(false, false, true);
								return bIsSucccess;
							}
							else
							{

							}



							//strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换成功 \r\n"),
							//	strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
							//strOutPut += strTempOut;
							//GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


							//int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

							////
							//main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
							//main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

							////将班次替换
							//ChangeTempSheduleToCurrentTable();
							//ShowAllPersonToList(false, false, true);

							//bIsSucccess = true;
							//return bIsSucccess;
						}
					}
				}
				else
				{
					strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人(C):%s 替换失败，班次不能上 \r\n"),
						strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork.GetBuffer());
					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
				}


			}
		}

	}



	//若如上仍然没有找到合适的人员，则进行4人之间轮换
	//A想跟B换，C和D是调休的人员，A->C,C->D,D->B
	//找出所有可能的C和D，判断是否能够轮换
	if (!bIsSucccess && NoWorkPersonList.size() > 0 && RelaxToWorkPersonPersonList.size() >= 2)
	{
		for (int n = 0; n < NoWorkPersonList.size(); n++)
		{
			int iPerson_A_Index = iPersonIndex; //申请调休的人
			int iPerson_B_Index = NoWorkPersonList[n]; //当前是调休的人
			CString strPersonName_Relax_Init = g_AllLevel_PersonList[iPerson_B_Index].strPersonName.c_str();

			for (int m = 0; m < RelaxToWorkPersonPersonList.size(); m++)
			{
				int iPerson_C_Index = RelaxToWorkPersonPersonList[m]; //原先是调休但是被安排了班次的人
				CString strPersonName_RelaxToWork_1 = g_AllLevel_PersonList[iPerson_C_Index].strPersonName.c_str();

				for (int ww = 0; ww < RelaxToWorkPersonPersonList.size(); ww++)
				{
					int iPerson_D_Index = RelaxToWorkPersonPersonList[ww]; //原先是调休但是被安排了班次的人
					CString strPersonName_RelaxToWork_2 = g_AllLevel_PersonList[iPerson_D_Index].strPersonName.c_str();

					if (iPerson_C_Index == iPerson_D_Index)
					{
						continue;
					}

					strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人1(C):%s  之前是调休被安排工作的人2(D):%s \r\n"),
						strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer());
					strOutPut += strTempOut;
					GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

					//判断C能不能接A  D能不能接C,B能不能接D
					int iClassIndexOneDay_A = -1;
					int iClassIndexOneDay_B = -1;
					int iClassIndexOneDay_C = -1;
					int iClassIndexOneDay_D = -1;
					int iClass_ChangeRelaxValue_A = 0;
					int iClass_ChangeRelaxValue_B = 0;
					int iClass_ChangeRelaxValue_C = 0;
					int iClass_ChangeRelaxValue_D = 0;

					int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_Relax_Init);
					int iPersonLevel_C = FindPesrsonLevelByName(strPersonName_RelaxToWork_1);
					int iPersonLevel_D = FindPesrsonLevelByName(strPersonName_RelaxToWork_1);

					GetClassNameOneDayOnePerson(iDayIndex, iPerson_A_Index, iClassIndexOneDay_A, iClass_ChangeRelaxValue_A);
					GetClassNameOneDayOnePerson(iDayIndex, iPerson_B_Index, iClassIndexOneDay_B, iClass_ChangeRelaxValue_B);
					GetClassNameOneDayOnePerson(iDayIndex, iPerson_C_Index, iClassIndexOneDay_C, iClass_ChangeRelaxValue_C);
					GetClassNameOneDayOnePerson(iDayIndex, iPerson_D_Index, iClassIndexOneDay_D, iClass_ChangeRelaxValue_D);


					ChangeCurrentSheduleToTempTable();
					if (NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_D] > 0 &&
						NurseCanWorkClassesLevel[iPersonLevel_D-1][iClassIndexOneDay_C] > 0 &&
						NurseCanWorkClassesLevel[iPersonLevel_C-1][iClassIndexOneDay_A] > 0
						)
					{
						//

						//把D的班次给B
						int iClassIndex_D = iClassIndexOneDay_D + iDayIndex * OneDayClasssNum;
						main_table_temp[0][iPerson_B_Index][iClassIndex_D] = 2;
						main_table_temp[0][iPerson_D_Index][iClassIndex_D] = 0;

						//检查是否合格
						ClassCheckErroType iErrorType;
						CString strErroDes;
						bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_B_Index, iDayIndex, iClassIndex_D, iErrorType, strErroDes);
						if (!bIsChekOK)
						{
							//替换失败
							strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人1(C)%s 之前是调休被安排工作的人2(D)%s 替换失败 D->B失败 \r\n"),
								strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
							strOutPut += strTempOut;
							GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
						}
						else
						{

							//把C的班次给D
							int iClassIndex_C = iClassIndexOneDay_C + iDayIndex * OneDayClasssNum;
							main_table_temp[0][iPerson_D_Index][iClassIndex_C] = 2;
							main_table_temp[0][iPerson_C_Index][iClassIndex_C] = 0;

							//检查是否合格
							ClassCheckErroType iErrorType;
							CString strErroDes;
							bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_D_Index, iDayIndex, iClassIndex_C, iErrorType, strErroDes);
							if (!bIsChekOK)
							{
								//替换失败
								strTempOut.Format(_T("步骤7：尝试进行四人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人%s 替换失败 C->D失败 \r\n"),
									strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
								strOutPut += strTempOut;
								GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
							}
							else
							{

								//把A的班次给C
								int iClassIndex_A = iClassIndexOneDay_A + iDayIndex * OneDayClasssNum;
								main_table_temp[0][iPerson_C_Index][iClassIndex_A] = 2;
								main_table_temp[0][iPerson_A_Index][iClassIndex_A] = 0;

								//检查是否合格
								ClassCheckErroType iErrorType;
								CString strErroDes;
								bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPerson_C_Index, iDayIndex, iClassIndex_A, iErrorType, strErroDes);
								if (!bIsChekOK)
								{
									//替换失败
									strTempOut.Format(_T("步骤6：尝试进行三人之间换，请求调休的人(A):%s、当前是调休的人(B):%s、之前是调休被安排工作的人:%s 替换失败 A->C失败 \r\n"),
										strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer());
									strOutPut += strTempOut;
									GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);
								}
								else
								{





									strTempOut.Format(_T("步骤6：尝试进行四人之间换，请求调休的人(A)%s、当前是调休的人(B):%s、之前是调休被安排工作的人_1:%s 之前是调休被安排工作的人_2:%s  替换成功 \r\n"),
										strPersonName_request.GetBuffer(), strPersonName_Relax_Init.GetBuffer(), strPersonName_RelaxToWork_1.GetBuffer(), strPersonName_RelaxToWork_2.GetBuffer());
									strOutPut += strTempOut;
									GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);


									bIsSucccess = ChangeTwoDayClass(iPersonIndex, strPersonName_request, iDayIndex, iClass_OneDay, strClassName, iPerson_B_Index, vecRelaxDays, strOutPut, iRelaxType);

									if (bIsSucccess)
									{

										int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

										main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
										main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

										//将班次替换
										ChangeTempSheduleToCurrentTable();
										//ShowAllPersonToList(false, false, true);
										return bIsSucccess;
									}
									else
									{

									}



									//int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;

									////
									//main_table_temp[0][iPerson_A_Index][iClassIndex_relax] = WORK_TO_Relax;
									//main_table_temp[0][iPerson_B_Index][iClassIndex_relax] = Relax_TO_WORK;

									////将班次替换
									//ChangeTempSheduleToCurrentTable();
									//ShowAllPersonToList(false, false, true);

									//bIsSucccess = true;
									//return bIsSucccess;
								}
							}
						}
					}
					else
					{
						continue;
					}
				}

			}

		}
	}



}



bool CPIVASScheduleDlg::OnAdjustRestAndNeeWorkPerson_NeedReturnClass_EX(int iPersonIndex, int iDayIndex, int iClass_OneDay)
{



	vector<ReturnClassStruct> vecReturnClassArray;

	//首先看看当前有没有空闲地调休人员
	bool bIsSucccess = true;
	//对需要调休的人员进行处理	
	CString strPersonName_request = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
	int iPersonLevel_A = FindPesrsonLevelByName(strPersonName_request);

	CString strTempOut;
	CString strOutPut;
	TCHAR strClassName[100] = { 0 };
	STRING2TCHAR(ClassName[iClass_OneDay], strClassName, 100);
	strOutPut.Format(_T("步骤1：对(%s)调休进行处理，第%d天(%s)班次 需要还班 \r\n"), strPersonName_request.GetBuffer(), iDayIndex + 1, strClassName);

	GetDlgItem(IDC_EDIT1)->SetWindowText(strOutPut);

	RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax]); //调休类型




	//先将其班次跟当天的人互换，然后对其进行调休，只有这两种都满足的时候才算调休成功
	for (int iPersonIndex_Temp = 0; iPersonIndex_Temp < g_AllLevel_PersonList.size(); iPersonIndex_Temp++)
	{
		//非冻结人员
		if(!g_AllLevel_PersonList[iPersonIndex_Temp].bIsFreeze)
		{
			int iClassIndexOneDay_B = 0;
			int iClass_ChangeRelaxValue = 0;
			//查看当前班次是否能调休，夜班不允许调 是否是3-7和3-4 3-5
			GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex_Temp, iClassIndexOneDay_B, iClass_ChangeRelaxValue);
			bool bCanRest = bJudgeCanRestByClassIndex(iClassIndexOneDay_B);
			if(!bCanRest)
			{
				continue;
			}
			//查看本轮该人其他天的班次是否有3-7和3-4 3-5
			if (iClassIndexOneDay_B == Class_3_4 || iClassIndexOneDay_B == Class_3_5 || iClassIndexOneDay_B == Class_3_7
				|| iClassIndexOneDay_B == -1)
			{
				continue;
			}

			CString strPersonName_B = g_AllLevel_PersonList[iPersonIndex_Temp].strPersonName.c_str();
			int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_B);
			//俩人是否能更换
			if (NurseCanWorkClassesLevel[iPersonLevel_A-1][iClassIndexOneDay_B] > 0 &&
				NurseCanWorkClassesLevel[iPersonLevel_B-1][iClass_OneDay] > 0)
			{
				//二人能换
				//需要看下A换完后能不能有机会调休
				int iPesonIndex_Rest = -1;
				bool bIsSucccess = false;

				if (iClassIndexOneDay_B == Class_ChangeRelax)
				{
					bIsSucccess = true;
					iPesonIndex_Rest = iPersonIndex_Temp;
				}
				else
				{
					bIsSucccess = OnAdjustRestAndNeeWorkPerson_EX(iPersonIndex, iDayIndex, iClassIndexOneDay_B, iPesonIndex_Rest, false);
				}


				if (!bIsSucccess)
				{
					//无调休机会
				}
				else
				{
					//查看还班
					for (int iDayIndex_temp = 0; iDayIndex_temp < AllDay; iDayIndex_temp++)
					{
						if (iDayIndex_temp == iDayIndex)
						{
							continue;
						}
						int iClassIndexOneDay_B_temp = 0;
						int iClass_ChangeRelaxValue_B_temp = 0;
						GetClassNameOneDayOnePerson(iDayIndex_temp, iPersonIndex_Temp, iClassIndexOneDay_B_temp, iClass_ChangeRelaxValue_B_temp);
						
						int iClassIndexOneDay_A_temp = 0;
						int iClass_ChangeRelaxValue_A_temp = 0;
						GetClassNameOneDayOnePerson(iDayIndex_temp, iPersonIndex, iClassIndexOneDay_A_temp, iClass_ChangeRelaxValue_A_temp);
						bCanRest = bJudgeCanRestByClassIndex(iClassIndexOneDay_A_temp);
						if (!bCanRest || iClassIndexOneDay_A_temp == -1 || iClassIndexOneDay_B_temp == -1 || iClassIndexOneDay_A_temp == OneDayClasssNum-1)
						{
							continue;
						}

						if (iClassIndexOneDay_B_temp == Class_3_4 || iClassIndexOneDay_B_temp == Class_3_5 || iClassIndexOneDay_B_temp == Class_3_7)
						{
							//俩人是否能更换
							if (NurseCanWorkClassesLevel[iPersonLevel_A-1][iClassIndexOneDay_B_temp] > 0 &&
								NurseCanWorkClassesLevel[iPersonLevel_B-1][iClassIndexOneDay_A_temp] > 0)
							{
								//二人能换，该方案可行，存储下来

								ReturnClassStruct theReturnClassStruct;

								theReturnClassStruct.iPerson_Source = iPersonIndex;
								theReturnClassStruct.iPerson_Dst = iPersonIndex_Temp;
								theReturnClassStruct.iPerson_Rest = iPesonIndex_Rest;

								theReturnClassStruct.iClass_OneDay_SrcPerson_SrcClass = iClass_OneDay;
								theReturnClassStruct.iClass_OneDay_DstPerson_SrcClass = iClassIndexOneDay_B;
								theReturnClassStruct.iClass_OneDay_SrcPerson_DstClass = iClassIndexOneDay_A_temp;
								theReturnClassStruct.iClass_OneDay_DstPerson_DstClass = iClassIndexOneDay_B_temp;

								theReturnClassStruct.iDayIndex_Source = iDayIndex;
								theReturnClassStruct.iDayIndex_Dst = iDayIndex_temp;

								//如果方案是2开头的则最低优先级
								if ((iClassIndexOneDay_B >= 0 && iClassIndexOneDay_B <= 5)  || (iClassIndexOneDay_A_temp >= 0 && iClassIndexOneDay_A_temp <= 5))
								{
									theReturnClassStruct.iLevel_Plan = 3;
								}
								else if (iPersonIndex_Temp == iPesonIndex_Rest)
								{
									theReturnClassStruct.iLevel_Plan = 1;
								}
								else
								{
									theReturnClassStruct.iLevel_Plan = 2;
								}


								vecReturnClassArray.push_back(theReturnClassStruct);

								break;

							}
						}
					}
				}

			}
			else
			{
				continue;
			}
		}
	}

	vector<ReturnClassStruct> vecReturnClassArray_temp;

	//对方案进行排序，优先级1的在前面，2的在后面，3的在最后
	while (vecReturnClassArray.size() > 0)
	{
		int iMinLevel = 10;
		int iMinIndex = -1;
		for (int i = 0; i < vecReturnClassArray.size(); i++)
		{
			if (vecReturnClassArray[i].iLevel_Plan < iMinLevel)
			{
				iMinLevel = vecReturnClassArray[i].iLevel_Plan;
				iMinIndex = i;
			}
		}
		if (iMinIndex >= 0)
		{
			vecReturnClassArray_temp.push_back(vecReturnClassArray[iMinIndex]);
			vecReturnClassArray.erase(vecReturnClassArray.begin() + iMinIndex);
		}
		else
		{
			break;
		}
	}

	if (vecReturnClassArray_temp.size() > 0)
	{
		bIsSucccess = true;
		if (g_theDlgRequstRestDay)
		{
			delete g_theDlgRequstRestDay;
			g_theDlgRequstRestDay = nullptr;
		}
		g_theDlgRequstRestDay = new CDiagRetunClassSelect;
		g_theDlgRequstRestDay->m_theReturnClassInfo = vecReturnClassArray_temp;

		//弹非模态框显示
		g_theDlgRequstRestDay->Create(IDD_CDiagRetunClassSelect, this);
		g_theDlgRequstRestDay->ShowWindow(SW_SHOW);

		////弹框由用户选择使用哪个方案
		//if (theDlgRequstRestDay.DoModal() == IDOK)
		//{
		//}
	}
	else
	{
		bIsSucccess = false;
	}


	return bIsSucccess;
}


void CPIVASScheduleDlg::OnBnSavePersonOrder()
{
	// TODO: 在此添加控件通知处理程序代码
	//将人员名称和级别存到CSV文件中

	TCHAR strFileName[100] = { 0 };
	STRING2TCHAR(g_filePath, strFileName, 100);
	CString strFilePath_temp = strFileName;
	strFilePath_temp += _T("\\排班起始时间.ini");

	CTime cThisDay = CTime(g_tStartTime);
	time_t tCurrent = g_tStartTime;
	tm* ltm = localtime(&tCurrent);
	int weekday = ltm->tm_wday;
	CString strDateInfo;
	strDateInfo.Format(_T("%d.%02d"), cThisDay.GetMonth(), cThisDay.GetDay());

	WritePrivateProfileString(_T("Schedule_STARTTIME"), _T("Init_StartTime"), strDateInfo, strFilePath_temp);
	WritePrivateProfileString(_T("Schedule_STARTTIME"), _T("Last_StartTime"), strDateInfo, strFilePath_temp);


	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员顺序.csv";
	fp = fopen(filename.c_str(), "w");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}

	char *chData = new char[200000];
	memset(chData, 0, 200000);
	char *chData_temp = chData;

	int iSize_Level1 = g_Level1_PersonList.size();

	int iPersonIndex = 0;
	for (int k = 0; k < iSize_Level1; k++)
	{
		g_Level1_PersonList[k];
		string strPersonName;
		//点名
		CStringtoString(strPersonName, g_Level1_PersonList[k].strPersonName.c_str());
		sprintf(chData, "%d,%d,%s,\n",k,1, strPersonName.c_str());
		chData += strlen(chData);
		iPersonIndex++;
	}



	vector<wstring> g_Level2_PersonList_temp;
	int iSize_Level2 = g_Level2_PersonList.size();
	for (int k = 0; k < iSize_Level2; k++)
	{
		string strPersonName;
		//点名
		CStringtoString(strPersonName, g_Level2_PersonList[k].strPersonName.c_str());
		sprintf(chData, "%d,%d,%s,\n", k,2, strPersonName.c_str());
		chData += strlen(chData);
		iPersonIndex++;
	}
	
	int iSize_Level3 = g_Level3_PersonList.size();
	for (int k = 0; k < iSize_Level3; k++)
	{
		string strPersonName;
		//点名
		CStringtoString(strPersonName, g_Level3_PersonList[k].strPersonName.c_str());
		sprintf(chData, "%d,%d,%s,\n", k,3, strPersonName.c_str());
		chData += strlen(chData);
		iPersonIndex++;
	}
	int iSize_Level4 = g_Level4_PersonList.size();
	for (int k = 0; k < iSize_Level4; k++)
	{
		string strPersonName;
		//点名
		CStringtoString(strPersonName, g_Level4_PersonList[k].strPersonName.c_str());
		sprintf(chData, "%d,%d,%s,\n", k, 4, strPersonName.c_str());
		chData += strlen(chData);
		iPersonIndex++;
	}
	//加一行空行
	sprintf(chData, "%d,%d,,\n", 0, 0);
	chData += strlen(chData);

	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_SET);
		fwrite(chData_temp, strlen(chData_temp)+1, 1, fp);
		fclose(fp);
	}
	if (chData_temp)
	{
		delete chData_temp;
	}

}

BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam)
{


	CWnd* pWnd = CWnd::FromHandle(hwnd);
	CSize* pTranslate = (CSize*)lParam;

	CPIVASScheduleDlg* pDlg = (CPIVASScheduleDlg*)pWnd->GetParent();
	if (!pDlg) return FALSE;

	CRect rect;
	pWnd->GetWindowRect(rect);
	//if (hwnd == pDlg->m_Grid.GetSafeHwnd())
	//	TRACE(_T("Wnd rect: %d,%d - %d,%d\n"),rect.left,rect.top, rect.right, rect.bottom);

	pDlg->ScreenToClient(rect);
	//if (hwnd == pDlg->m_Grid.GetSafeHwnd())
	//	TRACE(_T("Scr rect: %d,%d - %d,%d\n"),rect.left,rect.top, rect.right, rect.bottom);

	if (hwnd == pDlg->m_pGrid.GetSafeHwnd())
	{
		if (((rect.top >= 7 && pTranslate->cy > 0) || rect.Height() > 20) &&
			((rect.left >= 7 && pTranslate->cx > 0) || rect.Width() > 20))
			pWnd->MoveWindow(rect.left, rect.top,
				rect.Width() + pTranslate->cx,
				rect.Height() + pTranslate->cy, FALSE);
		else
			pWnd->MoveWindow(rect.left + pTranslate->cx, rect.top + pTranslate->cy,
				rect.Width(), rect.Height(), FALSE);
	}
	else if (hwnd == pDlg->m_TraceWnd.GetSafeHwnd())
	{
		if (((rect.top >= 7 && pTranslate->cy > 0) || rect.Height() > 20) &&
			((rect.left >= 7 && pTranslate->cx > 0) || rect.Width() > 20))
			pWnd->MoveWindow(rect.left, rect.top + pTranslate->cy,
				rect.Width() + pTranslate->cx,
				rect.Height(), FALSE);
		else
			pWnd->MoveWindow(rect.left + pTranslate->cx, rect.top + pTranslate->cy,
				rect.Width(), rect.Height(), FALSE);
	}
	else
	{
		if (pWnd->GetDlgCtrlID() == IDC_SIZEBOX)
			pWnd->MoveWindow(rect.left + pTranslate->cx, rect.top + pTranslate->cy,
				rect.Width(), rect.Height(), FALSE);
		else
			pWnd->MoveWindow(rect.left + pTranslate->cx, rect.top, rect.Width(), rect.Height(), FALSE);
	}
	pDlg->Invalidate();

	return TRUE;
}



void CPIVASScheduleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	if (cx <= 1 || cy <= 1)
		return;

#ifdef _WIN32_WCE
	m_Grid.MoveWindow(0, 0, cx, cy, FALSE);
#else
	CSize Translate(cx - m_OldSize.cx, cy - m_OldSize.cy);
	::EnumChildWindows(GetSafeHwnd(), EnumProc, (LPARAM)&Translate);
	m_OldSize = CSize(cx, cy);
#endif


	// 工具栏靠左对齐
	if (m_Toolbar.GetSafeHwnd())
	{
		CRect rect;
		GetClientRect(&rect);
		m_Toolbar.MoveWindow(0, 0, rect.Width(), 70); // 70为工具栏高度，可根据实际调整
	}

	// 重新布局其它控件
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

	if (m_pGrid.GetSafeHwnd())
	{
		// 这里假设工具栏高度为70，留出顶部空间
		CRect rect;
		GetClientRect(&rect);
		int toolbarHeight = 70;
		m_pGrid.MoveWindow(0, toolbarHeight, rect.Width(), rect.Height() - toolbarHeight);
	}



	CWnd* pWnd = GetDlgItem(IDC_SIZEBOX);
	if (pWnd)
		pWnd->ShowWindow((nType == SIZE_MAXIMIZED) ? SW_HIDE : SW_SHOW);

}
void CPIVASScheduleDlg::OnBnClickedButtonRule()
{
	// TODO: 在此添加控件通知处理程序代码

	CRuleManageDlg RuleManageDlg;

	if (RuleManageDlg.DoModal() == IDOK)
	{

	}


}

//批量删除班次用的菜单命令号。
//这里用 TPM_RETURNCMD 自己接返回值，不走消息映射，所以不需要往资源里加ID，
//只要避开系统保留的 0xF000 以上即可。
#define ID_BATCH_DELETE_RELAX		50001		//批量删除调休类班次
#define ID_BATCH_DELETE_USERDEFINE	50002		//批量删除自定义班次
#define ID_BATCH_DELETE_ALLINRANGE	50003		//批量删除选中区域内的全部班次
#define ID_CLEAR_OPERATE_TRACE		50004		//清除操作痕迹底色
#define ID_RESET_COMPARE_BASE		50005		//以当前排班为新的比较基准
#define ID_IMPORT_REST_DAY			50006		//从人员假期信息重新导入剩余年假


//判断某个班次是不是"谁也上不了"的班次。
//像 QA 这种所有级别的权限都是0，任何人的下拉框里都选不出来，
//只可能是从旧表里带过来的标记，性质上和自定义班次一样，批量删除时一并处理。
static bool bJudgeIsNobodyCanWorkClass(int iClassIndex_OneDay)
{
	if (iClassIndex_OneDay < 0 || iClassIndex_OneDay >= OneDayClasssNum - 1)
	{
		return false;
	}
	for (int iLevel = 0; iLevel < nurse_level; iLevel++)
	{
		if (NurseCanWorkClassesLevel[iLevel][iClassIndex_OneDay] > 0)
		{
			return false;
		}
	}
	return true;
}

//上一轮排班表带进来的"外来班次"名单。
//QA 这类班次本系统并不排，只是从旧表里带进来的标记，批量删除时要和
//自定义班次(补端毛、Z破)一样处理。
//原来只靠 bJudgeIsNobodyCanWorkClass 判断，而 QA 在级别3那一行被配成了"能上"，
//于是"批量删除整表的自定义班次"从头到尾都没把 QA 算进去，删完 QA 还在。
//以后再遇到别的外来班次，往 s_szForeignClassName 里加名字即可。
//把当前这张排班表记成新的"原始表"基准：
//此后只有再改动的格子才会标红，操作痕迹底色也一并清掉。
//对应客户要的"清除标记/重新取基准"按钮，做在表格右键菜单里。
static void ResetInitTableBaseFromGrid(CPIVASScheduleDlg* pDlg)
{
	if (pDlg == nullptr)
	{
		return;
	}

	g_myClassVector.clear();

	int iPersonNum = (int)g_AllLevel_PersonList.size();
	for (int iRow = 2; iRow < iPersonNum + 2; iRow++)
	{
		for (int iCol = PersonName + 1; iCol <= PersonName + AllDay; iCol++)
		{
			g_myClassVector[MakeInitTableKey(iRow, iCol)] = pDlg->m_pGrid.GetItemText(iRow, iCol);
		}
	}

	g_mapOperatedCells.clear();
}

static bool bJudgeIsForeignClass(int iClassIndex_OneDay)
{
	if (iClassIndex_OneDay < 0 || iClassIndex_OneDay >= OneDayClasssNum - 1)
	{
		return false;
	}

	static const char* s_szForeignClassName[] = { "QA" };
	int iForeignNum = (int)(sizeof(s_szForeignClassName) / sizeof(s_szForeignClassName[0]));

	for (int k = 0; k < iForeignNum; k++)
	{
		if (strcmp(ClassName[iClassIndex_OneDay], s_szForeignClassName[k]) == 0)
		{
			return true;
		}
	}
	return false;
}


//批量删除班次，删完的格子变成空白的万能格
int CPIVASScheduleDlg::BatchDeleteClass(int iDeleteType, bool bUseSelection, bool bOnlyCount)
{
	int iPersonNum = (int)g_AllLevel_PersonList.size();

	//默认整张表
	int iRowBegin = 2;
	int iRowEnd = iPersonNum + 1;
	int iColBegin = PersonName + 1;
	int iColEnd = PersonName + AllDay;

	if (bUseSelection)
	{
		CCellRange theRange = m_pGrid.GetSelectedCellRange();
		if (!theRange.IsValid())
		{
			return 0;
		}

		//选中区域可能包含表头行和左边的序号/级别/姓名列，先夹到有效范围里
		iRowBegin = max(2, theRange.GetMinRow());
		iRowEnd = min(iPersonNum + 1, theRange.GetMaxRow());
		iColBegin = max(PersonName + 1, theRange.GetMinCol());
		iColEnd = min(PersonName + AllDay, theRange.GetMaxCol());
	}

	int iDeleteNum = 0;

	for (int iRow = iRowBegin; iRow <= iRowEnd; iRow++)
	{
		//和别处保持一致：人员索引一律按第2列的姓名去查，不靠行号推算
		CString strPersonName = m_pGrid.GetItemText(iRow, PersonName);
		int iPersonIndex = FindPesrsonIndexByName(strPersonName);
		if (iPersonIndex < 0 || iPersonIndex >= iPersonNum)
		{
			continue;
		}

		//冻结的人员界面上本来就不让编辑，批量删除也跳过
		if (g_AllLevel_PersonList[iPersonIndex].bIsFreeze)
		{
			continue;
		}

		for (int iCol = iColBegin; iCol <= iColEnd; iCol++)
		{
			int iDayIndex = iCol - PersonName - 1;
			if (iDayIndex < 0 || iDayIndex >= AllDay)
			{
				continue;
			}

			CString strKey;
			strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);

			//模板里写死的"休"不动，跟单个"删除班次"的规矩保持一致
			if (g_mapAllRestClass.find(strKey) != g_mapAllRestClass.end())
			{
				continue;
			}

			//自定义班次(补端毛、Z破之类)只存在这个map里，main_table里没有记录
			bool bIsUserDefineClass = false;
			map<CString, CString>::iterator iter_UserDefine = g_mapAllUserDefineClass.find(strKey);
			if (iter_UserDefine != g_mapAllUserDefineClass.end())
			{
				CString strUserDefineText = iter_UserDefine->second;
				strUserDefineText.Trim();
				if (!strUserDefineText.IsEmpty())
				{
					bIsUserDefineClass = true;
				}
			}

			//当天有没有真正的班次，以及有没有调休/补休标记
			int iWorkClassIndex = GetWorkClassIndexOfDay_Safe(iPersonIndex, iDayIndex);
			int iRelaxValue = main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax];

			//没上班又带着调休标记，格子显示出来就是"调休""补休""补五一"这类
			bool bIsRelaxClass = (iWorkClassIndex < 0 && iRelaxValue != 0);

			//QA 这类谁也上不了的班次，任何人的下拉框里都选不出来，只可能是旧表带过来的，
			//和自定义班次归为一类
			bool bIsNobodyCanWorkClass = bJudgeIsNobodyCanWorkClass(iWorkClassIndex)
				|| bJudgeIsForeignClass(iWorkClassIndex);		//QA 即使被配成某个级别能上，也照样当外来班次删掉

			bool bNeedDelete = false;
			if (iDeleteType == 1)
			{
				bNeedDelete = bIsRelaxClass;
			}
			else if (iDeleteType == 2)
			{
				bNeedDelete = (bIsUserDefineClass || bIsNobodyCanWorkClass);
			}
			else
			{
				bNeedDelete = (bIsRelaxClass || bIsUserDefineClass || iWorkClassIndex >= 0);
			}

			if (!bNeedDelete)
			{
				continue;
			}

			iDeleteNum++;

			if (bOnlyCount)
			{
				continue;
			}

			//把这一天的记录整个清掉，格子就成了空白的万能格
			for (int iClassIndex_OneDay = 0; iClassIndex_OneDay < OneDayClasssNum; iClassIndex_OneDay++)
			{
				main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + iClassIndex_OneDay] = 0;
			}
			if (iter_UserDefine != g_mapAllUserDefineClass.end())
			{
				g_mapAllUserDefineClass.erase(strKey);
			}

			AddOneModifyToVect(iPersonIndex, iDayIndex);
		}
	}

	return iDeleteNum;
}


//右键菜单里"批量删除"各项的入口
void CPIVASScheduleDlg::OnBatchDeleteClass(int iDeleteType, bool bUseSelection)
{
	CString strWhat;
	if (iDeleteType == 1)		strWhat = _T("调休类班次(调休/补休/补五一…)");
	else if (iDeleteType == 2)	strWhat = _T("自定义班次(补端毛/Z破/QA…)");
	else						strWhat = _T("全部班次");

	CString strWhere = bUseSelection ? _T("选中区域") : _T("整张排班表");

	int iCount = BatchDeleteClass(iDeleteType, bUseSelection, true);
	if (iCount <= 0)
	{
		CString strTip;
		strTip.Format(_T("%s里没有找到可删除的%s。"), strWhere, strWhat);
		MessageBox(strTip, _T("提示"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	CString strAsk;
	strAsk.Format(_T("将删除%s里的 %d 个%s。\n\n删除后这些格子会变成空白的万能格，可以重新排任何本级别能上的班次。\n\n确定要删除吗？"),
		strWhere, iCount, strWhat);
	if (MessageBox(strAsk, _T("批量删除班次"), MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
	{
		return;
	}

	g_vectorReturnClassPersonAndDay.clear();
	g_vectorPersonCanRest.clear();

	int iDeleted = BatchDeleteClass(iDeleteType, bUseSelection, false);

	//整批算一步撤销，撤销一次即可全部回退
	AddPlanToCurrentRedoArray();

	if (g_bAutoSave)
	{
		SetTimer(TIMER_AUTO_SAVE, 2000, NULL);
	}

	ShowAllPersonToList(false, false, true);
	m_pGrid.Invalidate();
	m_pGrid.RedrawWindow();

	CString strDone;
	strDone.Format(_T("已删除 %d 个班次，均已变成空白的万能格。\n如需还原，点工具栏的“撤销”一次即可。"), iDeleted);
	MessageBox(strDone, _T("提示"), MB_OK | MB_ICONINFORMATION);
}


void CPIVASScheduleDlg::OnGridRClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/)
{
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;

	CPoint point;
	GetCursorPos(&point);

	//菜单直接用代码建，不再走资源里的 IDR_POPUP_TABLE_DBEDITOR。
	//原来那个菜单只有"插入一行""删除一行"两项，已按要求去掉，
	//现在右键菜单里只保留"批量删除班次"：下一轮会把上一轮的调休、自定义班次
	//整片带过来，这些这轮多半用不上，一个个删太费事。
	CMenu menuPopup;
	if (!menuPopup.CreatePopupMenu())
	{
		return;
	}

	menuPopup.AppendMenu(MF_STRING, ID_BATCH_DELETE_RELAX, _T("批量删除整表的调休班次(调休/补休/补五一…)"));
	menuPopup.AppendMenu(MF_STRING, ID_BATCH_DELETE_USERDEFINE, _T("批量删除整表的自定义班次(补端毛/Z破/QA…)"));

	CCellRange theSelectRange = m_pGrid.GetSelectedCellRange();
	UINT uFlagSelection = theSelectRange.IsValid() ? MF_STRING : (MF_STRING | MF_GRAYED);
	menuPopup.AppendMenu(uFlagSelection, ID_BATCH_DELETE_ALLINRANGE, _T("批量删除选中区域内的班次"));

	menuPopup.AppendMenu(MF_SEPARATOR);
	menuPopup.AppendMenu(MF_STRING, ID_CLEAR_OPERATE_TRACE, _T("清除操作痕迹底色(不动排班数据)"));
	menuPopup.AppendMenu(MF_STRING, ID_RESET_COMPARE_BASE, _T("以当前排班为新基准(清除所有红字标记)"));
	menuPopup.AppendMenu(MF_SEPARATOR);
	menuPopup.AppendMenu(MF_STRING, ID_IMPORT_REST_DAY, _T("从人员假期信息重新导入剩余年假"));

	//用 TPM_RETURNCMD 自己接命令号，不用往资源和消息映射里加ID
	int nCmd = menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this);

	menuPopup.DestroyMenu();

	if (nCmd == ID_BATCH_DELETE_RELAX)
	{
		OnBatchDeleteClass(1, false);
	}
	else if (nCmd == ID_BATCH_DELETE_USERDEFINE)
	{
		OnBatchDeleteClass(2, false);
	}
	else if (nCmd == ID_BATCH_DELETE_ALLINRANGE)
	{
		OnBatchDeleteClass(3, true);
	}
	else if (nCmd == ID_CLEAR_OPERATE_TRACE)
	{
		if (g_mapOperatedCells.empty())
		{
			MessageBox(_T("当前没有操作痕迹。"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		}
		else
		{
			CString strAsk;
			strAsk.Format(_T("将清除 %d 个格子的操作痕迹底色。\n\n注意：只清颜色标记，排班数据不会变。"), (int)g_mapOperatedCells.size());
			if (MessageBox(strAsk, _T("清除操作痕迹"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
			{
				g_mapOperatedCells.clear();
				ShowAllPersonToList(false, false, true);
				m_pGrid.Invalidate();
				m_pGrid.RedrawWindow();
			}
		}
	}
	else if (nCmd == ID_IMPORT_REST_DAY)
	{
		//把"人员假期信息.csv"里的年假数据重新读一遍，刷到表格右侧的"剩余假期"列上。
		//客户在假期面板里改完不用重启程序。
		ReadPersonHasUsedRestDayFromFile();
		ShowAllPersonToList(false, false, true);
		m_pGrid.Invalidate();
		m_pGrid.RedrawWindow();
		MessageBox(_T("已从人员假期信息重新导入剩余年假。"), _T("提示"), MB_OK | MB_ICONINFORMATION);
	}
	else if (nCmd == ID_RESET_COMPARE_BASE)
	{
		CString strAsk;
		strAsk.Format(_T("将把当前这张排班表记为新的比较基准：\n\n· 所有红色字体(与原始表不一致)的标记会清空\n· 操作痕迹底色也会一并清除\n· 排班数据本身不会有任何改动\n\n确定吗？"));
		if (MessageBox(strAsk, _T("以当前排班为新基准"), MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
		{
			ResetInitTableBaseFromGrid(this);
			ShowAllPersonToList(false, false, true);
			m_pGrid.Invalidate();
			m_pGrid.RedrawWindow();
			MessageBox(_T("已重新取基准，后续再改动的格子才会标红。"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		}
	}
}

void CPIVASScheduleDlg::OnDBEditorInsert()
{
	// TODO: 在此添加命令处理程序代码
	CCellID cell;
	int insertAfter = 0;
	CCellRange cellRange = m_pGrid.GetSelectedCellRange();
	if (cellRange.IsValid())
	{
		cell.row = cellRange.GetMinRow();
		cell.col = cellRange.GetMinCol();
		int iPersonIndex = cell.row;
		
		CString strPersonName = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
		int iPersonLevel = FindPesrsonLevelByName(strPersonName);

		CDlgInsertOnePerson theInsertOnePerson;

		theInsertOnePerson.m_iCurrentLevel = iPersonLevel - 1;
		if (theInsertOnePerson.DoModal() == IDOK)
		{
			//增加一名人员
			//先查看 
			int iSelectLevel = theInsertOnePerson.m_iSelectLevel;
			if (iSelectLevel == iPersonLevel)
			{
				//在当前位置插入人员
				iPersonIndex;
				//对iSelectLevel级别增加一人
				
				AddOnePersonByLevel(theInsertOnePerson.m_strPersonName.GetBuffer(), iPersonIndex, iSelectLevel, false, theInsertOnePerson.m_bIsFreeze);
				
			}
			else
			{
				//在该级别最后插入人员
				AddOnePersonByLevel(theInsertOnePerson.m_strPersonName.GetBuffer(), iPersonIndex, iSelectLevel, true, theInsertOnePerson.m_bIsFreeze);
			}
			//InsertRow(DlgInsertTag.GetRow());
		}
	}
}


void CPIVASScheduleDlg::AddOnePersonByLevel(wstring strPesonName, int iPersonIndex, int iSelectLevel, bool bIsEnd,bool bIsFreeze)
{
	int iLevel1Num = g_Level1_PersonList.size();
	int iLevel2Num = g_Level2_PersonList.size();
	int iLevel3Num = g_Level3_PersonList.size();

	PersonBasicInfo thePersonBasicInfo;
	thePersonBasicInfo.strPersonName = strPesonName;
	thePersonBasicInfo.bIsFreeze = bIsFreeze;
	int iInsertPosition = iPersonIndex;
	if (iPersonIndex < iLevel1Num)
	{

		//在一级人员中插入
		if (bIsEnd)
		{
			g_Level1_PersonList.push_back(thePersonBasicInfo);
			iInsertPosition = iLevel1Num - 1;
		}
		else
		{
			g_Level1_PersonList.insert(g_Level1_PersonList.begin() + iPersonIndex, thePersonBasicInfo);			
		}
	}
	else if (iPersonIndex < iLevel1Num + iLevel2Num)
	{
		//在二级人员中插入
		if (bIsEnd)
		{
			g_Level2_PersonList.push_back(thePersonBasicInfo);
			iInsertPosition = iLevel1Num + iLevel2Num - 1;
		}
		else
		{
			g_Level2_PersonList.insert(g_Level2_PersonList.begin() + iPersonIndex - iLevel1Num, thePersonBasicInfo);
		}

	}
	else if (iPersonIndex < iLevel1Num + iLevel2Num + iLevel3Num)
	{
		if (bIsEnd)
		{
			g_Level3_PersonList.push_back(thePersonBasicInfo);
			iInsertPosition = iLevel1Num + iLevel2Num + iLevel3Num - 1;
		}
		else
		{
			g_Level3_PersonList.insert(g_Level3_PersonList.begin() + iPersonIndex - iLevel1Num - iLevel2Num, thePersonBasicInfo);
		}
	}


	g_AllLevel_PersonList.clear();
	g_AllLevel_PersonList = g_Level1_PersonList;
	for (int k = 0; k < g_Level2_PersonList.size(); k++)
	{
		g_AllLevel_PersonList.push_back(g_Level2_PersonList[k]);
	}

	for (int k = 0; k < g_Level3_PersonList.size(); k++)
	{
		g_AllLevel_PersonList.push_back(g_Level3_PersonList[k]);
	}

	//在iInsertPosition位置插入一个人
	for (int iPerson_Pos = g_AllLevel_PersonList.size() - 2; iPerson_Pos >= iInsertPosition; iPerson_Pos--)
	{
		for (int iDay = 0; iDay < AllDay; iDay++)
		{
			for (int iClass = 0; iClass < OneDayClasssNum; iClass++)
			{
				main_table[0][iPerson_Pos+1][iDay * OneDayClasssNum + iClass] = main_table[0][iPerson_Pos][iDay * OneDayClasssNum + iClass];
			}
		}

	}
	for (int iDay = 0; iDay < AllDay; iDay++)
	{
		for (int iClass = 0; iClass < OneDayClasssNum; iClass++)
		{
			main_table[0][iInsertPosition][iDay * OneDayClasssNum + iClass] = 0;
		}

		main_table[0][iInsertPosition][iDay * OneDayClasssNum + Class_ChangeRelax] = Relax_INIT; //调休班次初始化
	}


	ShowAllPersonToList(true, true, true);
}


void CPIVASScheduleDlg::OnGridClick(NMHDR* pNotifyStruct, LRESULT* /*pResult*/)
{//根据新的cell，更新grid 的第一行，title。
	NM_GRIDVIEW* pItem = (NM_GRIDVIEW*)pNotifyStruct;

	if (nullptr == pItem)
	{
		return;
	}
	if (pItem->iRow < 0 || pItem->iColumn < 0)
	{
		m_pGrid.SetFocus();
		return;
	}
	//更新 Grid的title行.
	//UpdateGridTitle(pItem->iRow, 2);

	//更新属性,选中行非数据行，则返回
	if (pItem->iRow < 1)
	{
		return;
	}
	m_pGrid.SetFocus();
	//CPropertiesWnd* pPropertiesWnd = ((CMainFrame*)AfxGetApp()->m_pMainWnd)->GetProperitiesWnd();
	//if (nullptr != pPropertiesWnd)
	//{
	//	pPropertiesWnd->Reload(this);
	//}
}

bool CPIVASScheduleDlg::SwitchOnePersonOneDayToCurrentClass(int iPersonIndex_A, int iDayIndex, CString itemText, int iClassIndexOneDay_A)
{

	int iPersonIndex_B = -1;
	//识别输入的班次
//查看当前班次属于哪个班
	int iClassIndex_OneDay_B = FindeClassIndex(itemText);
	//将班次存入表中
	if (iClassIndex_OneDay_B >= 0)
	{
		//main_table[0][iRowIndex - 1][iDayIndex * OneDayClasssNum + iClassIndex] = 1;
		TCHAR strClass_dst[100] = { 0 };
		STRING2TCHAR(ClassName[iClassIndex_OneDay_B], strClass_dst, 100);
		//检查当天是否有人上该班次的人数
		int iPersonNum = 0;
		for (int iPersonInex_temp = 0; iPersonInex_temp < g_AllLevel_PersonList.size(); iPersonInex_temp++)
		{
			//自己不能算成"已经有人在上"，否则会弹出自己跟自己换班，选是选否都没有任何效果
			if (iPersonInex_temp == iPersonIndex_A)
			{
				continue;
			}
			if (main_table[0][iPersonInex_temp][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_B] > 0)
			{
				iPersonNum++;
				iPersonIndex_B = iPersonInex_temp;
			}
		}
		if (iPersonNum >= 1 )
		{


			//判断俩人是否可以互换：A要能上B的班，B也要能上A的班，两边都得查
			CString strPersonName_A = g_AllLevel_PersonList[iPersonIndex_A].strPersonName.c_str();
			CString strPersonName_B = g_AllLevel_PersonList[iPersonIndex_B].strPersonName.c_str();
			int iPersonLevel_A = FindPesrsonLevelByName(strPersonName_A);
			int iPersonLevel_B = FindPesrsonLevelByName(strPersonName_B);

			//A原来上的班次名，报错时要说清楚是哪个班换不动
			TCHAR strClass_src[100] = { 0 };
			if (iClassIndexOneDay_A >= 0 && iClassIndexOneDay_A < OneDayClasssNum)
			{
				STRING2TCHAR(ClassName[iClassIndexOneDay_A], strClass_src, 100);
			}

			if (iPersonLevel_A <= 0 || iPersonLevel_A > nurse_level
				|| iPersonLevel_B <= 0 || iPersonLevel_B > nurse_level)
			{
				MessageBox(_T("人员级别异常，无法互换，请检查人员级别设置！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
				ShowAllPersonToList(false, false, true);
				return false;
			}

			//A能不能上目标班次，是两种做法(强制上、两人互换)共同的前提
			//（原来漏了这一步，级别不够的人也能被换上Z、A、T这类班）
			if (NurseCanWorkClassesLevel[iPersonLevel_A - 1][iClassIndex_OneDay_B] <= 0
				|| !bJudgePersonCanWorkClassBySpecialRule(strPersonName_A, iClassIndex_OneDay_B))
			{
				CString strTempOut;
				strTempOut.Format(_T("第%d天，%s（级别%d）上不了 %s 班次，不能更改！"),
					iDayIndex + 1, strPersonName_A, iPersonLevel_A, strClass_dst);
				MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);

				ShowAllPersonToList(false, false, true);

				return false;
			}

			//B能不能上A原来的班次，只影响"两人互换"这条路，不影响强制上
			bool bCanSwitchTwoPerson = (NurseCanWorkClassesLevel[iPersonLevel_B - 1][iClassIndexOneDay_A] > 0)
				&& bJudgePersonCanWorkClassBySpecialRule(strPersonName_B, iClassIndexOneDay_A);

			//======================= 一次性的"换班校验" =======================
			//原来是一层套一层：先问强制还是互换 -> 再弹违规确认 -> 再弹不能互换，
			//客户要点好几次才知道到底怎么回事。现在先把两条路各自的问题都算出来，
			//一个弹窗列清楚，选完就直接执行，不再二次追问。
			CString strForceErroDes;		//强制上(两人同班)会踩到什么
			CString strSwitchErroDes;		//两人互换会踩到什么

			{
				int iBaseIndex_Try = iDayIndex * OneDayClasssNum;
				int iSave_A_Old = main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndexOneDay_A];
				int iSave_A_New = main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndex_OneDay_B];
				int iSave_B_Old = main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndexOneDay_A];
				int iSave_B_New = main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndex_OneDay_B];

				//试算一：强制上。A 挤到目标班次，B 不动
				main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndexOneDay_A] = 0;
				main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndex_OneDay_B] = 1;

				ClassCheckErroType iErroType_Force = HasNO_ERROR;
				CString strErroDes_Force;
				bool bForceOk = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex, iClassIndex_OneDay_B, iErroType_Force, strErroDes_Force);
				CString strSoftDes_Force = g_strSoftRuleWarn;

				if (!bForceOk)
				{
					strForceErroDes.Format(_T("%s %s"), strPersonName_A, strErroDes_Force);
				}
				if (!strSoftDes_Force.IsEmpty())
				{
					if (!strForceErroDes.IsEmpty())
					{
						strForceErroDes += _T("；");
					}
					strForceErroDes += strSoftDes_Force;
				}

				main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndexOneDay_A] = iSave_A_Old;
				main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndex_OneDay_B] = iSave_A_New;

				//试算二：两人互换
				if (bCanSwitchTwoPerson)
				{
					main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndexOneDay_A] = 0;
					main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndexOneDay_A] = 1;
					main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndex_OneDay_B] = 0;
					main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndex_OneDay_B] = 1;

					ClassCheckErroType iErroType_A = HasNO_ERROR;
					ClassCheckErroType iErroType_B = HasNO_ERROR;
					CString strErroDes_TryA;
					CString strErroDes_TryB;
					bool bOk_A = bCheckOnePersonOneDayClassIsOk(iPersonIndex_A, iDayIndex, iClassIndex_OneDay_B, iErroType_A, strErroDes_TryA);
					CString strSoftDes_TryA = g_strSoftRuleWarn;
					bool bOk_B = bCheckOnePersonOneDayClassIsOk(iPersonIndex_B, iDayIndex, iClassIndexOneDay_A, iErroType_B, strErroDes_TryB);
					CString strSoftDes_TryB = g_strSoftRuleWarn;

					if (!bOk_A)
					{
						strSwitchErroDes.Format(_T("%s %s"), strPersonName_A, strErroDes_TryA);
					}
					if (!bOk_B)
					{
						if (!strSwitchErroDes.IsEmpty())
						{
							strSwitchErroDes += _T("；");
						}
						CString strOneDes;
						strOneDes.Format(_T("%s %s"), strPersonName_B, strErroDes_TryB);
						strSwitchErroDes += strOneDes;
					}
					if (!strSoftDes_TryA.IsEmpty() || !strSoftDes_TryB.IsEmpty())
					{
						if (!strSwitchErroDes.IsEmpty())
						{
							strSwitchErroDes += _T("；");
						}
						strSwitchErroDes += strSoftDes_TryA.IsEmpty() ? strSoftDes_TryB : strSoftDes_TryA;
					}

					main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndexOneDay_A] = iSave_A_Old;
					main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndexOneDay_A] = iSave_B_Old;
					main_table[0][iPersonIndex_B][iBaseIndex_Try + iClassIndex_OneDay_B] = iSave_B_New;
					main_table[0][iPersonIndex_A][iBaseIndex_Try + iClassIndex_OneDay_B] = iSave_A_New;
				}
			}

			CString strForceTip = strForceErroDes.IsEmpty() ? CString(_T("无冲突")) : strForceErroDes;
			CString strSwitchTip;
			if (!bCanSwitchTwoPerson)
			{
				strSwitchTip.Format(_T("不可行，%s 上不了 %s 班"), strPersonName_B, strClass_src);
			}
			else
			{
				strSwitchTip = strSwitchErroDes.IsEmpty() ? CString(_T("无冲突")) : strSwitchErroDes;
			}

			CString strTempOut;
			strTempOut.Format(_T("第%d天 %s 班次已经有 %s 在上。\n\n【是】强制上该班次（两人同班）：%s\n【否】与 %s 互换班次：%s\n【取消】不做任何改动"),
				iDayIndex + 1, strClass_dst, strPersonName_B,
				strForceTip, strPersonName_B, strSwitchTip);

			int iAnswer = MessageBox(strTempOut, _T("换班校验"), MB_YESNOCANCEL | MB_ICONQUESTION);

			if (iAnswer == IDCANCEL)
			{
				//放弃操作，什么都不改
				ShowAllPersonToList(false, false, true);
				return false;
			}

			if (iAnswer == IDYES)
			{
				//【强制上该班次·不替换】A上目标班次，B保持不动，两人同班；A原来的班次空出来
				main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndexOneDay_A] = 0;
				main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_B] = 1;

				//规则问题已经在上面的"换班校验"弹窗里说清楚了，这里不再二次追问

				g_vectorReturnClassPersonAndDay.clear();
				AddOneModifyToVect(iPersonIndex_A, iDayIndex);
				AddOneModifyToVect(iPersonIndex_B, iDayIndex);

				ShowAllPersonToList(false, false, true);
				return true;
			}

			//【两人互换】选了"否"，把A和B的班次对调
			if (!bCanSwitchTwoPerson)
			{
				//原来这里把人名和班次都写反了：判断的是B上不了A的班，提示却说成A上不了目标班次
				strTempOut.Format(_T("第%d天，%s（级别%d）上不了 %s 的 %s 班次，两人无法互换！\n如果确实要上该班次，请重新选择并选“是”强制上班。"),
					iDayIndex + 1, strPersonName_B, iPersonLevel_B, strPersonName_A, strClass_src);
				MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);

				ShowAllPersonToList(false, false, true);
				return false;
			}

			// 先在底层数组将A和B的班次互换，以便进行规则校验
			main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndexOneDay_A] = 0; //设置为不上班
			main_table[0][iPersonIndex_B][iDayIndex * OneDayClasssNum + iClassIndexOneDay_A] = 1; //设置为上班
			main_table[0][iPersonIndex_B][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_B] = 0; //设置为不上班
			main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_B] = 1; //设置为上班

			//互换后的规则问题已经在上面的"换班校验"弹窗里说清楚了，这里不再二次追问

			g_vectorReturnClassPersonAndDay.clear();
			AddOneModifyToVect(iPersonIndex_A, iDayIndex);
			AddOneModifyToVect(iPersonIndex_B, iDayIndex);

			ShowAllPersonToList(false, false, true);

			return true;
		}
		else if (iPersonNum >= 3 && iClassIndex_OneDay_B == Class_3)
		{
			CString strTempOut;
			strTempOut.Format(_T("第%d天(%s)班次已经有人上班，不能更改！"), iDayIndex + 1, strClass_dst);
			MessageBox(strTempOut, _T("提示"), MB_OK | MB_ICONINFORMATION);
			//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, strClassName);

			ShowAllPersonToList(false, false, true);

			return false;
		}
		else
		{
			//目标班次当天没有人上，直接排上去。
			//注意必须先把原来那一格清掉：原先漏了这一步，同一个人会同时挂着旧班次和新班次，
			//显示出来还是旧的，看着像"没反应"，再操作一次就会查到"已经有人在上"而其实是他自己。
			if (iClassIndexOneDay_A >= 0 && iClassIndexOneDay_A != iClassIndex_OneDay_B)
			{
				main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndexOneDay_A] = 0;
			}

			//将班次存入表中
			main_table[0][iPersonIndex_A][iDayIndex * OneDayClasssNum + iClassIndex_OneDay_B] = 1; //设置为上班


			g_vectorReturnClassPersonAndDay.clear();
			AddOneModifyToVect(iPersonIndex_A, iDayIndex);

			ShowAllPersonToList(false, false, true);
			return true;
		}
	}
	else
	{

		MessageBox(_T("输入的班次错误，请检查！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		//m_pGrid.SetItemText(pItem->iRow, pItem->iColumn, ReleaseClassName);
		::PostMessage(m_pGrid.GetSafeHwnd(), WM_LBUTTONUP, 0, 0);
		//*pResult = -1;
		return false;
	}

}

void CPIVASScheduleDlg::TakeRetunrClassToCurentTable(ReturnClassStruct &theReturnClassInfo)
{
	
	ChangeCurrentSheduleToTempTable();

	int iPersonIndex_A = theReturnClassInfo.iPerson_Source;
	int iPersonIndex_B = theReturnClassInfo.iPerson_Dst;

	int iPersonIndex_Rest = theReturnClassInfo.iPerson_Rest;


	int iClass_OneDay_A = theReturnClassInfo.iClass_OneDay_SrcPerson_SrcClass;
	int iClass_OneDay_B = theReturnClassInfo.iClass_OneDay_DstPerson_SrcClass;

	int iDayIndex_Source = theReturnClassInfo.iDayIndex_Source;
	RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][iPersonIndex_A][iDayIndex_Source * OneDayClasssNum + Class_ChangeRelax]); //调休类型

	//将A的班次给B A不上班，调休的人上A的班次
	main_table_temp[0][iPersonIndex_A][iClass_OneDay_A + iDayIndex_Source * OneDayClasssNum] = 0;
	main_table_temp[0][iPersonIndex_B][iClass_OneDay_B + iDayIndex_Source * OneDayClasssNum] = 0;
	main_table_temp[0][iPersonIndex_B][iClass_OneDay_A + iDayIndex_Source * OneDayClasssNum] = 1;
	main_table_temp[0][iPersonIndex_Rest][iClass_OneDay_B + iDayIndex_Source * OneDayClasssNum] = 1;


	int iClassIndex_relax = Class_ChangeRelax + iDayIndex_Source * OneDayClasssNum;

	if (iRelaxType == Relax_TO_WORK)
	{
		main_table_temp[0][iPersonIndex_A][iClassIndex_relax] = Relax_INIT;
	}
	else
	{
		main_table_temp[0][iPersonIndex_A][iClassIndex_relax] = WORK_TO_Relax;
	}

	int iDayIndex_Dst = theReturnClassInfo.iDayIndex_Dst;
	int iClass_OneDay_A_DST = theReturnClassInfo.iClass_OneDay_SrcPerson_DstClass;
	int iClass_OneDay_B_DST = theReturnClassInfo.iClass_OneDay_DstPerson_DstClass;

	//将A的班次给B A不上班，调休的人上A的班次
	main_table_temp[0][iPersonIndex_A][iClass_OneDay_A_DST + iDayIndex_Dst * OneDayClasssNum] = 0;
	main_table_temp[0][iPersonIndex_B][iClass_OneDay_B_DST + iDayIndex_Dst * OneDayClasssNum] = 0;
	main_table_temp[0][iPersonIndex_A][iClass_OneDay_B_DST + iDayIndex_Dst * OneDayClasssNum] = 1;
	main_table_temp[0][iPersonIndex_B][iClass_OneDay_A_DST + iDayIndex_Dst * OneDayClasssNum] = 1;



	ChangeTempSheduleToCurrentTable();

	ShowAllPersonToList(false, false, true);

}

void CPIVASScheduleDlg::SaveCurrentDataToModuleFile(CString strFullFilePath)
{

	CString strDateInfo;
	//CString filepath;
	CTime cThisDay = CTime(g_tStartTime);
	time_t tCurrent = g_tStartTime;
	tm* ltm = localtime(&tCurrent);
	int weekday = ltm->tm_wday;
	strDateInfo.Format(_T("%d.%02d"), cThisDay.GetMonth(), cThisDay.GetDay());

	string filename;
	CStringtoString(filename, strFullFilePath);
	TCHAR strFilepath_temp[100] = { 0 };
	STRING2TCHAR(filename, strFilepath_temp, 100);
	CString filepath = strFilepath_temp;

	vector<vector<CString>> vecTableHeadData;
	vector<vector<CString>> vecTableData;
	int iSize_Level1 = g_AllLevel_PersonList.size();
	int iPersonIndex = 0;
	for (int RowNum = 2; RowNum < iSize_Level1 + 2; RowNum++)
	{
		vector < CString> vecOneRowData;
		for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
		{

			CString strData = m_pGrid.GetItemText(RowNum, iColNum);
			//string strOneCellData;
			//CStringtoString(strOneCellData, strData);
			vecOneRowData.push_back(strData);
		}

		vecTableData.push_back(vecOneRowData);

		iPersonIndex++;
	}
	for (int RowNum = 0; RowNum < 2; RowNum++)
	{
		vector < CString> vecOneRowData;
		for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
		{
			CString strData = m_pGrid.GetItemText(RowNum, iColNum);
			vecOneRowData.push_back(strData);
		}
		vecTableHeadData.push_back(vecOneRowData);
	}
	ExportDataToExcel(vecTableData, filepath, strDateInfo, vecTableHeadData);
	//FILE* fp;
	//string filename;
	//CStringtoString(filename, strFullFilePath);
	//fp = fopen(filename.c_str(), "w");
	//if (fp == NULL)
	//{
	//	cout << "无法打开文件" << endl;
	//	return;
	//}

	//char* chData = new char[200000];
	//memset(chData, 0, 200000);
	//char* chData_temp = chData;

	//sprintf(chData, "序号, 级别, 人员\\日期, 一14, 二15, 三15, 四15, 五15, 六15, , 日14, 一14, 二15, 三15, 四15, 五15, 六15, 日14,\n");
	//chData += strlen(chData);



	//int iSize_Level1 = g_AllLevel_PersonList.size();

	//int iPersonIndex = 0;
	//for (int RowNum = 2; RowNum < iSize_Level1 + 2; RowNum++)
	//{
	//	for (int iColNum = 0; iColNum < 3 + AllDay; iColNum++)
	//	{

	//		CString strData = m_pGrid.GetItemText(RowNum, iColNum);
	//		if (strData.IsEmpty())
	//		{
	//			strData = _T(" ");
	//		}

	//		////若当前班次是休，那将“休”保存到文件中
	//		//CString strKey = _T("");
	//		//strKey.Format(_T("%d-%d"), iPersonIndex, iColNum - 3);
	//		//auto it = g_mapAllRestClass.find(strKey);
	//		//if (it != g_mapAllRestClass.end())
	//		//{
	//		//	strData = _T("休");
	//		//}


	//		string strOneCellData;
	//		CStringtoString(strOneCellData, strData);
	//		strcat(chData, strOneCellData.c_str());
	//		strcat(chData, ",");
	//	}
	//	//CStringtoString(strPersonName, g_Level1_PersonList[k].strPersonName.c_str());
	//	//sprintf(chData, "%d,%d,%s,\n", k, 1, strPersonName.c_str());
	//	strcat(chData, "\n");
	//	chData += strlen(chData);
	//	iPersonIndex++;
	//}




	//if (fp != NULL)
	//{
	//	fseek(fp, 0, SEEK_SET);
	//	fwrite(chData_temp, strlen(chData_temp) + 1, 1, fp);
	//	fclose(fp);
	//}
	//if (chData_temp)
	//{
	//	delete chData_temp;
	//}
}



void CPIVASScheduleDlg::OnBnClickedButton_Save()
{
	{
		CString strDateInfo;
		//CString filepath;
		CTime cThisDay = CTime(g_tStartTime);
		time_t tCurrent = g_tStartTime;
		tm* ltm = localtime(&tCurrent);
		int weekday = ltm->tm_wday;
		strDateInfo.Format(_T("%d.%02d"), cThisDay.GetMonth(), cThisDay.GetDay());

		string filename = g_filePath;
		filename += "\\模板_Last.xlsx";

		TCHAR strFilepath_temp[100] = { 0 };
		STRING2TCHAR(filename, strFilepath_temp, 100);
		CString filepath = strFilepath_temp;

		vector<vector<CString>> vecTableHeadData;
		vector<vector<CString>> vecTableData;
		int iSize_Level1 = g_AllLevel_PersonList.size();
		int iPersonIndex = 0;
		for (int RowNum = 2; RowNum < iSize_Level1 + 2; RowNum++)
		{
			vector < CString> vecOneRowData;
			for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
			{

				CString strData = m_pGrid.GetItemText(RowNum, iColNum);
				//string strOneCellData;
				//CStringtoString(strOneCellData, strData);
				vecOneRowData.push_back(strData);
			}

			vecTableData.push_back(vecOneRowData);

			iPersonIndex++;
		}
		for (int RowNum = 0; RowNum < 2; RowNum++)
		{
			vector < CString> vecOneRowData;
			for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
			{
				CString strData = m_pGrid.GetItemText(RowNum, iColNum);
				vecOneRowData.push_back(strData);
			}
			vecTableHeadData.push_back(vecOneRowData);
		}
		ExportDataToExcel(vecTableData, filepath, strDateInfo, vecTableHeadData);



		TCHAR strFileName[100] = { 0 };
		STRING2TCHAR(g_filePath, strFileName, 100);
		////将当前排班结果也拷贝一份到排班历史文件文件夹中
		//CString strDateInfo;
		//CTime cThisDay = CTime(g_tStartTime);

		//time_t tCurrent = g_tStartTime;
		//tm* ltm = localtime(&tCurrent);
		//int weekday = ltm->tm_wday;

		//strDateInfo.Format(_T("%d-%02d.xlsx"), cThisDay.GetMonth(), cThisDay.GetDay());

		CString Fname = _T("排班结果");
		Fname += strDateInfo;
		CString strHistoryFilePath = strFileName;
		strHistoryFilePath += _T("\\排班历史文件\\");
		strHistoryFilePath += Fname;

		TCHAR strfilename_Source[100] = { 0 };
		STRING2TCHAR(filename, strfilename_Source, 100);

		CopyFile(strfilename_Source, strHistoryFilePath, FALSE);

	}






	//// TODO: 在此添加控件通知处理程序代码
	////将表格上数据存到CSV里面
	////将人员名称和级别存到CSV文件中
	//FILE* fp;
	//string filename = g_filePath;
	//filename += "\\模板_Last.csv";
	//fp = fopen(filename.c_str(), "w");
	//if (fp == NULL)
	//{
	//	cout << "无法打开文件" << endl;
	//	return;
	//}

	//char* chData = new char[200000];
	//memset(chData, 0, 200000);
	//char* chData_temp = chData;

	//sprintf(chData, "序号, 级别, 人员\\日期, 一14, 二15, 三15, 四15, 五15, 六15, , 日14, 一14, 二15, 三15, 四15, 五15, 六15, 日14,\n");
	//chData += strlen(chData);
	//


	//int iSize_Level1 = g_AllLevel_PersonList.size();

	//int iPersonIndex = 0;
	//for (int RowNum = 2; RowNum < iSize_Level1+2; RowNum++)
	//{
	//	for (int iColNum = 0; iColNum < 3 + AllDay; iColNum++)
	//	{

	//		CString strData = m_pGrid.GetItemText(RowNum, iColNum);
	//		if (strData.IsEmpty())
	//		{
	//			strData = _T(" ");
	//		}

	//		////若当前班次是休，那将“休”保存到文件中
	//		//CString strKey = _T("");
	//		//strKey.Format(_T("%d-%d"), iPersonIndex, iColNum - 3);
	//		//auto it = g_mapAllRestClass.find(strKey);
	//		//if (it != g_mapAllRestClass.end())
	//		//{
	//		//	strData = _T("休");
	//		//}


	//		string strOneCellData;
	//		CStringtoString(strOneCellData, strData);
	//		strcat(chData, strOneCellData.c_str());
	//		strcat(chData, ",");
	//	}
	//	//CStringtoString(strPersonName, g_Level1_PersonList[k].strPersonName.c_str());
	//	//sprintf(chData, "%d,%d,%s,\n", k, 1, strPersonName.c_str());
	//	strcat(chData, "\n");
	//	chData += strlen(chData);
	//	iPersonIndex++;
	//}




	//if (fp != NULL)
	//{
	//	fseek(fp, 0, SEEK_SET);
	//	fwrite(chData_temp, strlen(chData_temp) + 1, 1, fp);
	//	fclose(fp);
	//}
	//if (chData_temp)
	//{
	//	delete chData_temp;
	//}


	//TCHAR strFileName[100] = { 0 };
	//STRING2TCHAR(g_filePath, strFileName, 100);

	//CString strFilePath_temp = strFileName;
	//strFilePath_temp += _T("\\排班起始时间.ini");
	//CTime LastStartTime = g_tStartTime;
	//CString strLastTime;
	//strLastTime = LastStartTime.Format(_T("%m.%d"));

	//WritePrivateProfileString(_T("Schedule_STARTTIME"), _T("Last_StartTime"), strLastTime, strFilePath_temp);



	////将当前排班结果也拷贝一份到排班历史文件文件夹中
	//CString strDateInfo;
	//CTime cThisDay = CTime(g_tStartTime);

	//time_t tCurrent = g_tStartTime;
	//tm* ltm = localtime(&tCurrent);
	//int weekday = ltm->tm_wday;

	//strDateInfo.Format(_T("%d-%02d.csv"), cThisDay.GetMonth(), cThisDay.GetDay());

	//CString Fname = _T("排班结果");
	//Fname += strDateInfo;
	//CString strHistoryFilePath = strFileName;
	//strHistoryFilePath += _T("\\排班历史文件\\");
	//strHistoryFilePath += Fname;

	//TCHAR strfilename_Source[100] = { 0 };
	//STRING2TCHAR(filename, strfilename_Source, 100);

	//CopyFile(strfilename_Source, strHistoryFilePath, FALSE);


}

void CPIVASScheduleDlg::OnBnClickedButtonExportExcel()
{
	// TODO: 在此添加控件通知处理程序代码

	CString strDateInfo;
	CTime cThisDay = CTime(g_tStartTime);

	time_t tCurrent = g_tStartTime ;
	tm* ltm = localtime(&tCurrent);
	int weekday = ltm->tm_wday;

	strDateInfo.Format(_T("%d.%02d"), cThisDay.GetMonth(), cThisDay.GetDay());

	CString Fname = _T("排班结果");
	Fname += strDateInfo;
	CString filepath;
	CFileDialog fileDlg(FALSE, _T("xlsx"), Fname, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY, _T("Excel Files (*.xlsx)|*.xlsx|"));
	if (IDOK == fileDlg.DoModal())
	{
		filepath = fileDlg.GetPathName();//全路径
		CString fileName = fileDlg.GetFileExt();//.bmp
	}

	vector<vector<CString>> vecTableHeadData;

	vector<vector<CString>> vecTableData;
	int iSize_Level1 = g_AllLevel_PersonList.size();

	int iPersonIndex = 0;
	for (int RowNum = 2; RowNum < iSize_Level1 + 2; RowNum++)
	{
		vector < CString> vecOneRowData;
		for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
		{

			CString strData = m_pGrid.GetItemText(RowNum, iColNum);
			//string strOneCellData;
			//CStringtoString(strOneCellData, strData);
			vecOneRowData.push_back(strData);
		}

		vecTableData.push_back(vecOneRowData);

		iPersonIndex++;
	}

	for (int RowNum = 0; RowNum < 2; RowNum++)
	{
		vector < CString> vecOneRowData;
		for (int iColNum = 2; iColNum < 3 + AllDay; iColNum++)
		{
			CString strData = m_pGrid.GetItemText(RowNum, iColNum);
			vecOneRowData.push_back(strData);
		}
		vecTableHeadData.push_back(vecOneRowData);
	}



	ExportDataToExcel(vecTableData, filepath, strDateInfo, vecTableHeadData);
	
	if (0)
	{
		//打开Excel
		ShellExecute(NULL, _T("open"), filepath, NULL, NULL, SW_SHOWNORMAL);
	}
	else
	{
		CString strTempOut;
		strTempOut.Format(_T("导出成功，文件名:%s"), filepath);
		AfxMessageBox(strTempOut, MB_OK | MB_ICONINFORMATION);
	}
}


void CPIVASScheduleDlg::ExportDataToExcel(vector<vector<CString>> &vecTableData, CString filepath, CString strTitle, vector<vector<CString>>& vecTableHeadData)
{

	char chDebug[255] = { 0 };
	sprintf(chDebug, "gao1119 ExportDataToExcel 11111");
	OutputDebugStringA(chDebug);

	int iBodyData_RowNum = vecTableData.size();
	int iBodyData_ColNum = vecTableData[0].size();

	int iRowNum = iBodyData_RowNum + 3; //导出的行数
	int iColNum = iBodyData_ColNum; //导出的列数
	int iMaterialNum = 0;



	//创建二维的安全数组 填充数据
	SAFEARRAY FAR* pPropVals = NULL;
	SAFEARRAYBOUND Bound[2];
	Bound[0].lLbound = 0;//二维的起始下标
	Bound[0].cElements = iRowNum;//二维的元素个数

	Bound[1].lLbound = 0;//一维的
	Bound[1].cElements = iColNum;//一维的

	pPropVals = SafeArrayCreate(VT_VARIANT, 2, Bound);
	CComVariant* pData = NULL;
	SafeArrayAccessData(pPropVals, (void**)&pData);

	//int iRow_temp = 0;
	//int iStartCol = 0;

	CString strTitle_tmep = _T("PIVAS技术员排班表(编号:") + strTitle;
	strTitle_tmep += _T(")");

	pData[0] = strTitle_tmep;

	for (int iRowIndex = 3; iRowIndex < vecTableData.size() + 3; iRowIndex++)
	{
		for (int iColIndex = 0; iColIndex < iColNum; iColIndex++)
		{
			CString strData_temp = vecTableData[iRowIndex - 3][iColIndex];
			pData[iRowIndex + iRowNum * iColIndex] = strData_temp;
		}
	}

	for (int iRowIndex = 1; iRowIndex <  3; iRowIndex++)
	{
		for (int iColIndex = 0; iColIndex < iColNum; iColIndex++)
		{
			if ((iRowIndex == 2) && (iColIndex == 0 ))//|| iColIndex == 1
			{
				continue;
			}
			CString strData_temp = vecTableHeadData[iRowIndex - 1][iColIndex];
			pData[iRowIndex + iRowNum * iColIndex] = strData_temp;
		}
	}

	//pData[iRow_temp+iRowNum*iStartCol] = _T("abc1");

	//iRow_temp = 0;
	//iStartCol = 1;
	//pData[iRow_temp+iRowNum*iStartCol] = _T("abc2");

	//数组写到文件中
	SafeArrayUnaccessData(pPropVals);

	sprintf(chDebug, "gao1119 ExportDataToExcel 222222");
	OutputDebugStringA(chDebug);


	// TODO: Add extra validation here	
	Workbooks wbsMyBooks;
	_Workbook wbMyBook;
	Worksheets wssMysheets;
	_Worksheet wsMysheet;
	Range rgMyRge;

	_variant_t var;
	LPDISPATCH pbook;


	wbsMyBooks.AttachDispatch(g_ExcelApp.GetWorkbooks(), true);
	//pbook = g_ExcelApp.GetActiveWorkbook();//如果当前有打开的Excel文档，返回非空。

	pbook = wbsMyBooks.New();//new工作薄

	//得到Worksheets 
	wbMyBook.AttachDispatch(pbook, true);
	wssMysheets.AttachDispatch(wbMyBook.GetWorksheets(), true);

	//得到sheet1 
	CString str;
	CString str1;
	//增加大于三的表
	int iTemp1 = 3;
	int iCount = wssMysheets.GetCount();


	sprintf(chDebug, "gao1119 ExportDataToExcel 33333");
	OutputDebugStringA(chDebug);

	float fTmp1, fTmp2;

	int iSheetIndex = 1;


	CString myname;
	str = _T("sheet1");
	myname = _T("sheet1");
	wsMysheet.AttachDispatch(wssMysheets.GetItem(_variant_t(str)), true);

	CComVariant varBegin;
	CComVariant varEnd;



	CalcExcelRange(iRowNum, iColNum, varBegin, varEnd);
	rgMyRge.AttachDispatch(wsMysheet.GetRange(varBegin, varEnd), true);


	rgMyRge.SetNumberFormat(COleVariant(_T("@")));

	CComVariant vparam;
	vparam.vt = VT_ARRAY | VT_VARIANT;
	vparam.parray = pPropVals;
	rgMyRge.SetValue(vparam);

	//调整单元格格式
	//AdjustFormat(&wsMysheet, 1, 1, 1, iColNum, true, 16);

	//第一列加黑设置字体，颜色
	//AdjustFormat(&wsMysheet, 4, 1, iRowNum, 1, true, 11);

	//第2列加黑设置字体，颜色
	//AdjustFormat(&wsMysheet, 4, 2, iRowNum, 2, true, 11);






	//Body区域的起始row与起始col

	int iLevel1Num = g_Level1_PersonList.size();
	int iLevel2Num = g_Level2_PersonList.size();
	int iLevel3Num = g_Level3_PersonList.size();

	// ====== 新增：设置第20行第1列，到第25行第1列的range颜色为红色 ======
	// 为了安全起见，可以先判断一下总行数是否到达了20行
	//if (iRowNum >= 20)
	{
		excel9::Range fontRange;
		excel9::Font  cellFont; // 注意这里换成了 Font 对象

		// 利用你代码中已有的 GetPosition 函数将行列转换为 "A20", "A25" 等格式
		CString strFromCell = GetPosition(3+iLevel1Num+1, 1);
		CString strToCell = GetPosition(3 + iLevel1Num+ iLevel2Num, 1);

		// 获取第20行到第25行第一列的 Range
		fontRange.AttachDispatch(wsMysheet.GetRange(COleVariant(strFromCell), COleVariant(strToCell)));

		// 获取该 Range 的 Font (字体) 对象
		cellFont.AttachDispatch(fontRange.GetFont());

		// 设置字体颜色为红色 RGB(255, 0, 0)
		cellFont.SetColor(_variant_t((long)RGB(255, 0, 0)));

		// 释放COM对象，防止内存泄漏
		cellFont.ReleaseDispatch();
		fontRange.ReleaseDispatch();
	}

	{
		excel9::Range fontRange;
		excel9::Font  cellFont; // 注意这里换成了 Font 对象

		// 利用你代码中已有的 GetPosition 函数将行列转换为 "A20", "A25" 等格式
		CString strFromCell = GetPosition(3 + iLevel1Num +1+ iLevel2Num, 1);
		CString strToCell = GetPosition(3 + iLevel1Num  + iLevel2Num + iLevel3Num, 1);

		// 获取第20行到第25行第一列的 Range
		fontRange.AttachDispatch(wsMysheet.GetRange(COleVariant(strFromCell), COleVariant(strToCell)));

		// 获取该 Range 的 Font (字体) 对象
		cellFont.AttachDispatch(fontRange.GetFont());

		// 设置字体颜色为紫色 RGB(255, 0, 0)
		cellFont.SetColor(_variant_t((long)RGB(160, 50, 122)));

		// 释放COM对象，防止内存泄漏
		cellFont.ReleaseDispatch();
		fontRange.ReleaseDispatch();
	}







	//Body区域的起始row与起始col

	int iStartBodyRow = 1;
	int iEndBodyRow = iRowNum;

	int iStartBodyCol = 1;
	int iEndBodyCol = iColNum;
	//设置边框线
	SetBorderAround(iStartBodyRow, iStartBodyCol, iEndBodyRow, iEndBodyCol, &wsMysheet);

	//设置固定字段边框线


	SetBorderAround(iStartBodyRow, iStartBodyCol, iEndBodyRow, iEndBodyCol, &wsMysheet);


	SetBorderAround(iStartBodyRow, iStartBodyCol, iEndBodyRow, iEndBodyCol, &wsMysheet);


	sprintf(chDebug, "gao1119 ExportDataToExcel 44444");
	OutputDebugStringA(chDebug);



	//设置内容为居中对齐
	int iStartBodyRow_temp = 1;
	int iEndBodyRow_temp = 1;

	SetBodyAlignment(iStartBodyRow_temp, iEndBodyRow_temp, iStartBodyCol, iEndBodyCol, &wsMysheet);



	//合并第1行
	MergeRangs(&wsMysheet, 1, 1, 1, iColNum);

	//合并第2行第3行第1格单元格
	MergeRangs(&wsMysheet, 2, 1, 3, 1);

	//合并第2行第3行第2格单元格
	//MergeRangs(&wsMysheet, 2, 2, 3, 2);



	sprintf(chDebug, "gao1119 ExportDataToExcel 55555");
	OutputDebugStringA(chDebug);

	//MergeRangs(&wsMysheet);

	rgMyRge.ReleaseDispatch();
	wsMysheet.ReleaseDispatch();


	//str = _T("d:\\aaa.xlsx");

	CString strMaterialRequisitionFileName = filepath;
	::DeleteFile((LPCTSTR)strMaterialRequisitionFileName);
	var = (_bstr_t)strMaterialRequisitionFileName;

	wbMyBook.mysaveAs(var);


	sprintf(chDebug, "gao1119 ExportDataToExcel 66666");
	OutputDebugStringA(chDebug);

	wbMyBook.Close(true);

	//导出时如果有打开的EXCEL，会报错，所以将以下两句注释去掉  2012.2.15
	wbsMyBooks.Close();//lj.note 不知道为什么要关闭所有文档。暂时先注释，不然导出数据库后会关掉先前打开的Excel文档	


	sprintf(chDebug, "gao1119 ExportDataToExcel 77777");
	OutputDebugStringA(chDebug);


	//释放对象 
	rgMyRge.ReleaseDispatch();
	wsMysheet.ReleaseDispatch();
	wssMysheets.ReleaseDispatch();

	wbMyBook.ReleaseDispatch();
	wbsMyBooks.ReleaseDispatch();


	sprintf(chDebug, "gao1119 ExportDataToExcel 88888");
	OutputDebugStringA(chDebug);

}



void CPIVASScheduleDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 1)
	{
		if (g_bHasAuth)
		{
			CString strTitle;
			strTitle.Format(_T("智能排班软件"));
			SetWindowText(strTitle);
		}
		else
		{
			if (m_RemainingTime > 0)
			{
				m_RemainingTime = m_RemainingTime - 1;
				//设置标题栏显示剩余时间
				CString strTitle;
				int iDay = m_RemainingTime / 86400;
				int iHour = (m_RemainingTime % 86400) / 3600;
				int iMin = (m_RemainingTime % 3600) / 60;
				int iSec = m_RemainingTime % 60;

				strTitle.Format(_T("智能排班软件 试用版  剩余时间 %d天 %02d:%02d:%02d"), iDay, iHour, iMin, iSec);
				//设置对话框标题栏
				SetWindowText(strTitle);
			}
			else
			{
				if (m_RemainingTime == 60)
				{
					MessageBox(_T("试用时间还剩最后1分钟，请尽快保存数据！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
				}
				if (m_RemainingTime <= 0)
				{
					m_RemainingTime = 0;
					KillTimer(1);
					//时间到，自动保存
					writeAccessRegEdit();
					//弹窗提示，试用时间已到，将要退出程序
					MessageBox(_T("试用时间已到，程序将自动保存数据并退出！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
					PostQuitMessage(0);
				}




			}
		}

	}
	else if (nIDEvent == 2)
	{
		writeAccessRegEdit();
	}

	if (nIDEvent == TIMER_AUTO_SAVE)
	{
		// 1. 停止定时器，防止重复触发
		KillTimer(TIMER_AUTO_SAVE);

		// 2. 执行真正的保存
		OnBnClickedButton_Save();

		// 打印调试信息（可选）
		OutputDebugStringA("gao0425 Auto save executed after idle.");
	}

	CDialogEx::OnTimer(nIDEvent);
}

#define HKEY_MAIN					HKEY_LOCAL_MACHINE															//注册表根主键
#define HKEY_CHILD					_T("SOFTWARE\\SONOFOSH\\IPAS\\PIVAS_Schedule")							//配置文件路径注册表子键
#define KEY_NAME 					_T("Remaining Time1")													//配置文件路径项名

BOOL CPIVASScheduleDlg::ReadAccessRegEdit()
{
	HKEY hKEY;
	HKEY hKeyRoot = HKEY_MAIN;

	//以写模式打开HKEY_LOCAL_MACHINE\\SOFTWARE\\Batch\\BatchSetupWizard注册表子键
	long lRet = ::RegOpenKeyEx(hKeyRoot, HKEY_CHILD, 0, KEY_READ, &hKEY);
	if (lRet != ERROR_SUCCESS)
	{
		return FALSE;//创建注册表子键失败
	}

	//设置BatchConfigFilePath项值为传入的配置文件存储路径
	TCHAR chRemainTime[MAX_PATH];
	wsprintf(chRemainTime, _T("%d"), m_RemainingTime);
	DWORD keyType = REG_SZ;//注册表项类型
	DWORD dwLen = MAX_PATH * sizeof(TCHAR);
	lRet = ::RegQueryValueEx(hKEY, KEY_NAME, NULL, &keyType, (LPBYTE)chRemainTime, &dwLen);
	//lRet = ::RegSetValueEx(hKEY, KEY_NAME, 0, REG_SZ, (LPBYTE)chRemainTime, MAX_PATH);
	if (lRet != ERROR_SUCCESS)
	{
		return FALSE;//设置注册表项值失败
	}
	//chRemainTime转整形
	m_RemainingTime = _ttoi(chRemainTime);
	return TRUE;


}
	



BOOL CPIVASScheduleDlg::writeAccessRegEdit()
{

	HKEY hKEY;
	HKEY hKeyRoot = HKEY_MAIN;

	//以写模式打开HKEY_LOCAL_MACHINE\\SOFTWARE\\Batch\\BatchSetupWizard注册表子键
	long lRet = ::RegOpenKeyEx(hKeyRoot, HKEY_CHILD, 0, KEY_WRITE, &hKEY);
	if (lRet != ERROR_SUCCESS)
	{
		//打开失败则创建HKEY_LOCAL_MACHINE\\SOFTWARE\\Batch\\BatchSetupWizard注册表子键
		lRet = ::RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			HKEY_CHILD,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_WRITE,
			NULL,
			&hKEY,
			NULL);
		if (lRet != ERROR_SUCCESS)
		{
			return FALSE;//创建注册表子键失败
		}
	}

	//设置BatchConfigFilePath项值为传入的配置文件存储路径
	TCHAR chRemainTime[MAX_PATH];
	wsprintf(chRemainTime, _T("%d"), m_RemainingTime);
	lRet = ::RegSetValueEx(hKEY, KEY_NAME, 0, REG_SZ, (LPBYTE)chRemainTime, MAX_PATH);
	if (lRet != ERROR_SUCCESS)
	{
		return FALSE;//设置注册表项值失败
	}



	//QString sRegPath = WIN32_REG_ACCESS_PATH;
	//if (Is64BitSystem())
	//{
	//	sRegPath = WIN64_REG_ACCESS_PATH;
	//}
	//QSettings regAccess(sRegPath, QSettings::NativeFormat);
	//regAccess.setValue("Remaining Time", QVariant(m_RemainingTime));

	//char info[200] = { 0 };
	//char pCurrTime[50] = { 0 };
	//getCurrTime(pCurrTime, 50);
	//sprintf(info, "Write Remaining Time:%d", m_RemainingTime);
	//m_RunLog.writeRunLog(3, pCurrTime, "MACFORE_CentralCtrlServer::writeAccessRegEdit", info);

	//WriteRemainingTime();
}
;
void CPIVASScheduleDlg::AddOneModifyToVect(int iPersonIndex, int iDayIndex)
{

	ReturnClassPersonAndDay theReturnClassPersonAndDay1;
	theReturnClassPersonAndDay1.iPersonIndex = iPersonIndex;
	theReturnClassPersonAndDay1.iDayIndex = iDayIndex;
	g_vectorReturnClassPersonAndDay.push_back(theReturnClassPersonAndDay1);

	//顺便记一笔"这个格子本轮被动过"，界面上留一层浅灰底，方便回溯改过哪些地方。
	//右键菜单里可以清除，或者"以当前排班为新基准"一起清掉。
	if (iPersonIndex >= 0 && iDayIndex >= 0)
	{
		CString strKey_Operated;
		strKey_Operated.Format(_T("%d-%d"), iPersonIndex, iDayIndex);
		g_mapOperatedCells[strKey_Operated] = 1;
	}
}
void CPIVASScheduleDlg::AddOnePersonCanRestToVect(int iPersonIndex, int iDayIndex)
{
	ReturnClassPersonAndDay thePersonCanRest;
	thePersonCanRest.iPersonIndex = iPersonIndex;
	thePersonCanRest.iDayIndex = iDayIndex;
	g_vectorPersonCanRest.push_back(thePersonCanRest);
}


void CPIVASScheduleDlg::AddPlanToCurrentRedoArray(bool bHasModifyPersonRest)
{
	
	// 【Bug 1 修复】标准撤销树逻辑：如果在执行了多次“撤销”后，用户突然手动改了格子（触发新历史），
    // 必须斩断（清空）当前索引之后的所有 Redo 历史，否则时空线会错乱。
    if (g_undo_index >= 0 && g_undo_index < g_undo_AllNum - 1)
    {
        g_undo_AllNum = g_undo_index + 1;
    }
	
	if(g_undo_AllNum < MaxUndoNum)
	{ 
		for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
		{
			for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
			{
				main_table_undo[g_undo_AllNum][iPerson][iClassIndex] = main_table[0][iPerson][iClassIndex];
			}
		}
		g_PersonRest_undo[g_undo_AllNum] = bHasModifyPersonRest;
		g_UserDefineClass_undo[g_undo_AllNum] = g_mapAllUserDefineClass;
		g_RestClass_undo[g_undo_AllNum] = g_mapAllRestClass;

		g_undo_AllNum = g_undo_AllNum + 1;
		g_undo_index = g_undo_AllNum - 1;


		if (g_undo_AllNum >= MaxUndoNum)
		{
			//删掉最前面的一个，把后面的所有的往前挪
			for (int Index = 0; Index < MaxUndoNum - 1; Index++)
			{
				for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
				{
					for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
					{
						main_table_undo[Index][iPerson][iClassIndex] = main_table_undo[Index+1][iPerson][iClassIndex];
					}
				}
				// 【Bug 3 遗漏】同步平移人员假期的 undo 状态数组，否则对应的假期历史会发生错位
                g_PersonRest_undo[Index] = g_PersonRest_undo[Index + 1];
				g_UserDefineClass_undo[Index] = g_UserDefineClass_undo[Index + 1];
				g_RestClass_undo[Index] = g_RestClass_undo[Index + 1];
			}
			g_undo_AllNum = MaxUndoNum - 1;
			g_undo_index = g_undo_AllNum - 1;
		}
	}


}



void CPIVASScheduleDlg::OnBnClickedButton_Undo()
{
	if (g_undo_index > 0 && g_undo_AllNum > 1)
	{
		g_undo_index = g_undo_index - 1;

		if (g_PersonRest_undo[g_undo_index] == 1)
		{
			map<CString, PersonRestDay >::iterator iter1;
			for (iter1 = g_mapPersonRestDayNum.begin(); iter1 != g_mapPersonRestDayNum.end(); iter1++)
			{
				iter1->second.iHasUsedDay = 0;
			}
		}

		// ==================== 新增：比对差异并记录高亮 ====================
		g_vectorReturnClassPersonAndDay.clear();
		int iAllPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();

		for (int iPerson = 0; iPerson < iAllPersonNum; iPerson++)
		{
			for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
			{
				bool bChanged = false;
				// 遍历该天对应的所有班次位（包括 Class_ChangeRelax 状态）
				for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum; iClassIndex++)
				{
					if (main_table[0][iPerson][iClassIndex] != main_table_undo[g_undo_index][iPerson][iClassIndex])
					{
						bChanged = true;
						break;
					}
				}
				// 如果该天的数据有任何改变，标记为修改以便高亮
				if (bChanged)
				{
					AddOneModifyToVect(iPerson, iDayIndex);
				}
			}
		}
		// ===================================================================

		// 将撤销的内容放到当前表中
		for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
		{
			for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
			{
				main_table[0][iPerson][iClassIndex] = main_table_undo[g_undo_index][iPerson][iClassIndex];
			}
		}
		//自定义班次的文字不在 main_table 里，要单独还原
		g_mapAllUserDefineClass = g_UserDefineClass_undo[g_undo_index];
		//模板里写死的"休"也不在 main_table 里，要一起还原
		g_mapAllRestClass = g_RestClass_undo[g_undo_index];
		// g_vectorReturnClassPersonAndDay.clear();  // 删除原有的这行 clear，保留我们上面收集的差异

		ShowAllPersonToList(false, false, true);
		// 3. 强制重绘控件
		m_pGrid.Invalidate();
		m_pGrid.RedrawWindow();
	}
	else
	{
		//弹出提示框前面无撤销内容
		MessageBox(_T("前面无撤销内容！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
	}
}
void CPIVASScheduleDlg::OnBnClickedButton_Redo()
{
	if (g_undo_index < g_undo_AllNum - 1 && g_undo_AllNum > 0)
	{
		int target_index = g_undo_index + 1; // 预先计算目标恢复步数

		// ==================== 新增：比对差异并记录高亮 ====================
		g_vectorReturnClassPersonAndDay.clear();
		int iAllPersonNum = g_Level1_PersonList.size() + g_Level2_PersonList.size() + g_Level3_PersonList.size() + g_Level4_PersonList.size();

		for (int iPerson = 0; iPerson < iAllPersonNum; iPerson++)
		{
			for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
			{
				bool bChanged = false;
				for (int iClassIndex = iDayIndex * OneDayClasssNum; iClassIndex < (iDayIndex + 1) * OneDayClasssNum; iClassIndex++)
				{
					if (main_table[0][iPerson][iClassIndex] != main_table_undo[target_index][iPerson][iClassIndex])
					{
						bChanged = true;
						break;
					}
				}
				if (bChanged)
				{
					AddOneModifyToVect(iPerson, iDayIndex);
				}
			}
		}
		// ===================================================================

		// 将恢复的内容放到当前表中
		g_undo_index = target_index;

		for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
		{
			for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
			{
				main_table[0][iPerson][iClassIndex] = main_table_undo[g_undo_index][iPerson][iClassIndex];
			}
		}
		//自定义班次的文字不在 main_table 里，要单独还原
		g_mapAllUserDefineClass = g_UserDefineClass_undo[g_undo_index];
		//模板里写死的"休"也不在 main_table 里，要一起还原
		g_mapAllRestClass = g_RestClass_undo[g_undo_index];
		// g_vectorReturnClassPersonAndDay.clear(); // 删除原有的这行 clear

		ShowAllPersonToList(false, false, true);
		// 3. 强制重绘控件
		m_pGrid.Invalidate();
		m_pGrid.RedrawWindow();
	}
	else
	{
		//弹出提示框前面无撤销内容
		MessageBox(_T("后面无恢复内容！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
	}
}
void CPIVASScheduleDlg::AddTray()
{
	static int siCount = 0;
	m_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	m_notifyIconData.hWnd = m_hWnd;
	m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_notifyIconData.uCallbackMessage = IDM_MYICONNOTIFY;			//绑定消息
	m_notifyIconData.hIcon = m_hIcon;								//设置托盘图标
	CString strMouseOverIcon(MAKEINTRESOURCE(IDS_MOUSEOVERICON));
	lstrcpy(m_notifyIconData.szTip, strMouseOverIcon);				//鼠标停放时的提示信息
	BOOL bRet = Shell_NotifyIcon(NIM_ADD, &m_notifyIconData);					//显示托盘图标

	if (FALSE == bRet) //如果添加系统托盘图标失败，启动一个定时器再添加
	{
		DWORD dwError = GetLastError();

		CStringA strError("");
		strError.Format("Add tray failed, error id = %d", dwError);
		//WriteLog("AddTray", strError.GetBuffer()); strError.ReleaseBuffer();
		//SetTimer(ADDTRAYTIMER, 1000, NULL);
	}
	else
	{
		//if (siCount > 0)
		//	KillTimer(ADDTRAYTIMER);
	}
	siCount++;
}

LRESULT CPIVASScheduleDlg::OnMyIconNotify(WPARAM wParam, LPARAM lParam)
{
	//if (!g_bEnable) //不重复进入，否则可能导致崩溃
	//{
	//	return afx_msg LRESULT();
	//}

	if (WM_LBUTTONDBLCLK == lParam) //双击左键处理
	{
		//m_DebugDialog.ShowModal();
	}

	if (WM_RBUTTONDOWN == lParam) //按下右键，弹出菜单
	{
		CMenu menu;
		tagPOINT point;
		GetCursorPos(&point);		//获取鼠标当前坐标
		menu.CreatePopupMenu();		//创建右键弹出菜单，无菜单项


		//add by 高祖汉 135765 版本号:V2.2.0 需求号:HBATCH-SR-SYS-008-A 增加客户端启动快捷按钮 2015-3-26 below
		CString strQuit(MAKEINTRESOURCE(IDM_QUITENGINE));
		CString strActiveSoftware(MAKEINTRESOURCE(IDM_ACTIVESOFTWARE));

		//CString strAbout(MAKEINTRESOURCE(IDM_ABOUTDIALOG));
		//CString strHelp(MAKEINTRESOURCE(IDM_HELPMENU));

		//CString strStartScheduler(MAKEINTRESOURCE(IDM_STARTSCHEDULER));
		//CString strStartExplorer(MAKEINTRESOURCE(IDM_STARTEXPLORER));
		//CString strOnlineBackUp(MAKEINTRESOURCE(IDM_ONLINEBACKUP));

		//menu.AppendMenu(MF_STRING, IDM_QUITENGINE, strQuit);		//添加‘关于’菜单项
		menu.AppendMenu(MF_STRING, IDM_ACTIVESOFTWARE, strActiveSoftware);		//添加‘关于’菜单项

		//menu.AppendMenu(MF_SEPARATOR);								//分隔符
		//menu.AppendMenu(MF_STRING, IDM_STARTSCHEDULER, strStartScheduler);		//添加‘启动批量调度器’菜单项，绑定消息

		//menu.AppendMenu(MF_STRING, IDM_STARTEXPLORER, strStartExplorer);		//添加‘启动批量编辑器’菜单项，绑定消息
		////分隔符
		//menu.AppendMenu(MF_SEPARATOR);

		//if (TRUE == g_bEnableShow)
		//{
		//	//在线备份菜单
		//	menu.AppendMenu(MF_STRING, IDS_STRING_ONLINEBACKY, strOnlineBackUp);		//添加‘在线备份’菜单项，绑定消息

		//	//分隔符
		//	menu.AppendMenu(MF_SEPARATOR);
		//}

		menu.AppendMenu(MF_STRING, IDM_QUITENGINE, strQuit);		//添加‘退出’菜单项，绑定退出消息				
		::SetForegroundWindow(m_hWnd);
		//menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTALIGN, point.x, point.y, this);	//显示托盘图标右键菜单

		//if (m_bIsInstallScheduler == FALSE)
		//{
		//	menu.EnableMenuItem(IDM_STARTSCHEDULER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		//}
		//if (m_bIsInstallExplorer == FALSE)
		//{
		//	menu.EnableMenuItem(IDM_STARTEXPLORER, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		//}

		menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);	//显示托盘图标右键菜单
		//add by 高祖汉 135765 版本号:V2.2.0 需求号:HBATCH-SR-SYS-008-A 增加客户端启动快捷按钮 2015-3-26 above


		::PostMessage(m_hWnd, WM_NULL, 0, 0);
	}
	return afx_msg LRESULT();
}

void  CPIVASScheduleDlg::QuitEngine()
{
	CString strText(MAKEINTRESOURCE(IDS_QUITENGINETIP));	//消息提示内容
	CString strTitle(MAKEINTRESOURCE(IDS_STRING_SOFTWARENAME));	//消息提示框标题
	int iResponse = MessageBox(strText, strTitle, MB_OKCANCEL | MB_ICONINFORMATION);
	if (IDOK == iResponse)
	{
		PostMessage(WM_CLOSE);
	}

}
void  CPIVASScheduleDlg::ActiveSoftware()
{

	TCHAR tchMachine[200] = { 0 };
	STRING2TCHAR(strMachine, tchMachine, 200);

	CActiveSoftwareDlg theActiveSoftwareDlg;

	theActiveSoftwareDlg.strMachineCode = tchMachine;
	theActiveSoftwareDlg.m_pAuthorizeDLL = m_pAuthorizeDLL;
	theActiveSoftwareDlg.DoModal();
}

void CPIVASScheduleDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//把当前的行高/列宽/窗口大小记下来，下次打开还原
	SaveGridLayoutToIni(this);
	Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);	//退出后删除托盘图标	

	// 【关键修复 2】：防崩溃保护
	// 使用 try-catch 包裹 COM 的释放逻辑，安全吞掉“RPC 服务器不可用”之类的通信错误
	try
	{
		if (g_ExcelApp.m_lpDispatch != NULL) {
			g_ExcelApp.Quit();  //退出
		}
	}
	catch (...)
	{
		// 如果 RPC 通信失败，直接忽略，不抛出弹窗
	}

	if (g_ExcelApp.m_lpDispatch != NULL) {
		g_ExcelApp.ReleaseDispatch();
	}
	CDialogEx::OnClose();
}

void CAboutDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
}


void CPIVASScheduleDlg::OnBnClickedButton_Previous()
{

	//起始时间往前推7天
	g_tStartTime = g_tStartTime - 14 * 24 * 60 * 60;


	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson1 = false;
	int iSize_Level1 = g_Level1_PersonList.size();
	vector<PersonBasicInfo> g_Level1_PersonList_temp1;
	vector<int> vectLockPersonIndex1;
	for (int k = 0; k < iSize_Level1; k++)
	{
		if (g_Level1_PersonList[k].bShiftLocked)
		{
			bHasLockPerson1 = true;
			vectLockPersonIndex1.push_back(k);
		}
		else
		{
			g_Level1_PersonList_temp1.push_back(g_Level1_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level1_PersonList_temp2;
	int iSize_Level1_temp1 = g_Level1_PersonList_temp1.size();
	//g_Level1_PersonList_temp2.push_back(g_Level1_PersonList_temp1[iSize_Level1_temp1 - 1]);
	for (int k = 0; k < iSize_Level1_temp1-1; k++)
	{
		g_Level1_PersonList_temp2.push_back(g_Level1_PersonList_temp1[k+1]);
	}

	g_Level1_PersonList_temp2.push_back(g_Level1_PersonList_temp1[0]);

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex1.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex1[zz];
		g_Level1_PersonList_temp2.insert(g_Level1_PersonList_temp2.begin() + iLockIndex, g_Level1_PersonList[iLockIndex]);
	}


	//vector<PersonBasicInfo> g_Level1_PersonList_temp;
	//g_Level1_PersonList_temp.push_back(g_Level1_PersonList[iSize_Level1 - 1]);
	//for (int k = 0;k< iSize_Level1-1;k++)
	//{
	//	g_Level1_PersonList_temp.push_back(g_Level1_PersonList[k]);
	//}

	g_AllLevel_PersonList.clear();
	g_Level1_PersonList.clear();
	iSize_Level1 = g_Level1_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level1; k++)
	{
		g_Level1_PersonList.push_back(g_Level1_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level1_PersonList_temp2[k]);
	}


	//vector<PersonBasicInfo> g_Level2_PersonList_temp;
	//int iSize_Level2 = g_Level2_PersonList.size();
	//g_Level2_PersonList_temp.push_back(g_Level2_PersonList[iSize_Level2 - 1]);
	//for (int k = 0; k < iSize_Level2 - 1; k++)
	//{
	//	g_Level2_PersonList_temp.push_back(g_Level2_PersonList[k]);
	//}

	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson2 = false;
	int iSize_Level2 = g_Level2_PersonList.size();
	vector<PersonBasicInfo> g_Level2_PersonList_temp1;
	vector<int> vectLockPersonIndex2;
	for (int k = 0; k < iSize_Level2; k++)
	{
		if (g_Level2_PersonList[k].bShiftLocked)
		{
			bHasLockPerson2 = true;
			vectLockPersonIndex2.push_back(k);
		}
		else
		{
			g_Level2_PersonList_temp1.push_back(g_Level2_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level2_PersonList_temp2;
	int iSize_Level2_temp1 = g_Level2_PersonList_temp1.size();
	for (int k = 0; k < iSize_Level2_temp1 - 1; k++)
	{
		g_Level2_PersonList_temp2.push_back(g_Level2_PersonList_temp1[k+1]);
	}
	g_Level2_PersonList_temp2.push_back(g_Level2_PersonList_temp1[0]);

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex2.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex2[zz];
		g_Level2_PersonList_temp2.insert(g_Level2_PersonList_temp2.begin() + iLockIndex, g_Level2_PersonList[iLockIndex]);
	}


	g_Level2_PersonList.clear();
	iSize_Level2 = g_Level2_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level2; k++)
	{
		g_Level2_PersonList.push_back(g_Level2_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level2_PersonList_temp2[k]);

	}


	//vector<PersonBasicInfo> g_Level3_PersonList_temp;
	//int iSize_Level3 = g_Level3_PersonList.size();
	//g_Level3_PersonList_temp.push_back(g_Level3_PersonList[iSize_Level3 - 1]);
	//for (int k = 0; k < iSize_Level3 - 1; k++)
	//{
	//	g_Level3_PersonList_temp.push_back(g_Level3_PersonList[k]);
	//}

	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson3 = false;
	int iSize_Level3 = g_Level3_PersonList.size();
	vector<PersonBasicInfo> g_Level3_PersonList_temp1;
	vector<int> vectLockPersonIndex3;
	for (int k = 0; k < iSize_Level3; k++)
	{
		if (g_Level3_PersonList[k].bShiftLocked)
		{
			bHasLockPerson3 = true;
			vectLockPersonIndex3.push_back(k);
		}
		else
		{
			g_Level3_PersonList_temp1.push_back(g_Level3_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level3_PersonList_temp2;
	int iSize_Level3_temp1 = g_Level3_PersonList_temp1.size();
	//g_Level3_PersonList_temp2.push_back(g_Level3_PersonList_temp1[iSize_Level3_temp1 - 1]);
	for (int k = 0; k < iSize_Level3_temp1 - 1; k++)
	{
		g_Level3_PersonList_temp2.push_back(g_Level3_PersonList_temp1[k+1]);
	}
	g_Level3_PersonList_temp2.push_back(g_Level3_PersonList_temp1[0]);

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex3.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex3[zz];
		g_Level3_PersonList_temp2.insert(g_Level3_PersonList_temp2.begin() + iLockIndex, g_Level3_PersonList[iLockIndex]);
	}


	g_Level3_PersonList.clear();
	iSize_Level3 = g_Level3_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level3; k++)
	{
		g_Level3_PersonList.push_back(g_Level3_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level3_PersonList_temp2[k]);
	}




	//vector<PersonBasicInfo> g_Level4_PersonList_temp;
	//int iSize_Level4 = g_Level4_PersonList.size();
	//g_Level4_PersonList_temp.push_back(g_Level4_PersonList[iSize_Level4 - 1]);
	//for (int k = 0; k < iSize_Level4 - 1; k++)
	//{
	//	g_Level4_PersonList_temp.push_back(g_Level4_PersonList[k]);
	//}


	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson4 = false;
	int iSize_Level4 = g_Level4_PersonList.size();
	vector<PersonBasicInfo> g_Level4_PersonList_temp1;
	vector<int> vectLockPersonIndex4;
	for (int k = 0; k < iSize_Level4; k++)
	{
		if (g_Level4_PersonList[k].bShiftLocked)
		{
			bHasLockPerson4 = true;
			vectLockPersonIndex4.push_back(k);
		}
		else
		{
			g_Level4_PersonList_temp1.push_back(g_Level4_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level4_PersonList_temp2;
	int iSize_Level4_temp1 = g_Level4_PersonList_temp1.size();
	for (int k = 0; k < iSize_Level4_temp1 - 1; k++)
	{
		g_Level4_PersonList_temp2.push_back(g_Level4_PersonList_temp1[k+1]);
	}
	g_Level4_PersonList_temp2.push_back(g_Level4_PersonList_temp1[0]);

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex4.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex4[zz];
		g_Level4_PersonList_temp2.insert(g_Level4_PersonList_temp2.begin() + iLockIndex, g_Level4_PersonList[iLockIndex]);
	}

	g_Level4_PersonList.clear();
	iSize_Level4 = g_Level4_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level4; k++)
	{
		g_Level4_PersonList.push_back(g_Level4_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level4_PersonList_temp2[k]);
	}



	//if (g_modifyClassSettingType == NoSaveToNextTemplate)
	//{
	//	g_mapAllUserDefineClass.clear();
	//	//不保存，使用模板中的班次

	//	for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
	//	{
	//		for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
	//		{
	//			main_table[0][iPerson][iClassIndex] = main_table_init[0][iPerson][iClassIndex];
	//		}
	//	}

	//	ShowAllPersonToList(false, false, true);
	//}
	//else
	//{

	//}
	//ShowAllPersonToList(false, true, true);

	//OnBnSavePersonOrder();
	//OnBnClickedButton_Save();




	// 1. 同步全量刷新（包含人员姓名列 bRefreshPerson = true）
	ShowAllPersonToList(false, true, true);

	// 2. 强制重绘UI，让界面立刻看到变化
	m_pGrid.Invalidate();
	m_pGrid.RedrawWindow();

	// 3. 执行保存（此时 m_pGrid 里的文本已经是最新的了）
	OnBnSavePersonOrder();
	OnBnClickedButton_Save();
}

void CPIVASScheduleDlg::OnBnClickedButton_Next()
{
	// TODO: 在此添加控件通知处理程序代码
	//点击下一轮，将容器中最后的元素往前对调


	//起始时间往后推7天
	g_tStartTime = g_tStartTime + 14 * 24 * 60 * 60;


	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson1 = false;
	int iSize_Level1 = g_Level1_PersonList.size();
	vector<PersonBasicInfo> g_Level1_PersonList_temp1;
	vector<int> vectLockPersonIndex1;
	for (int k = 0; k < iSize_Level1; k++)
	{
		if (g_Level1_PersonList[k].bShiftLocked)
		{
			bHasLockPerson1 = true;
			vectLockPersonIndex1.push_back(k);
		}
		else
		{
			g_Level1_PersonList_temp1.push_back(g_Level1_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level1_PersonList_temp2;
	int iSize_Level1_temp1 = g_Level1_PersonList_temp1.size();
	g_Level1_PersonList_temp2.push_back(g_Level1_PersonList_temp1[iSize_Level1_temp1 - 1]);
	for (int k = 0; k < iSize_Level1_temp1 - 1; k++)
	{
		g_Level1_PersonList_temp2.push_back(g_Level1_PersonList_temp1[k]);
	}

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex1.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex1[zz];
		g_Level1_PersonList_temp2.insert(g_Level1_PersonList_temp2.begin() + iLockIndex, g_Level1_PersonList[iLockIndex]);
	}


	//vector<PersonBasicInfo> g_Level1_PersonList_temp;
	//g_Level1_PersonList_temp.push_back(g_Level1_PersonList[iSize_Level1 - 1]);
	//for (int k = 0;k< iSize_Level1-1;k++)
	//{
	//	g_Level1_PersonList_temp.push_back(g_Level1_PersonList[k]);
	//}

	g_AllLevel_PersonList.clear();
	g_Level1_PersonList.clear();
	iSize_Level1 = g_Level1_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level1; k++)
	{
		g_Level1_PersonList.push_back(g_Level1_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level1_PersonList_temp2[k]);
	}


	//vector<PersonBasicInfo> g_Level2_PersonList_temp;
	//int iSize_Level2 = g_Level2_PersonList.size();
	//g_Level2_PersonList_temp.push_back(g_Level2_PersonList[iSize_Level2 - 1]);
	//for (int k = 0; k < iSize_Level2 - 1; k++)
	//{
	//	g_Level2_PersonList_temp.push_back(g_Level2_PersonList[k]);
	//}

	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson2 = false;
	int iSize_Level2 = g_Level2_PersonList.size();
	vector<PersonBasicInfo> g_Level2_PersonList_temp1;
	vector<int> vectLockPersonIndex2;
	for (int k = 0; k < iSize_Level2; k++)
	{
		if (g_Level2_PersonList[k].bShiftLocked)
		{
			bHasLockPerson2 = true;
			vectLockPersonIndex2.push_back(k);
		}
		else
		{
			g_Level2_PersonList_temp1.push_back(g_Level2_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level2_PersonList_temp2;
	int iSize_Level2_temp1 = g_Level2_PersonList_temp1.size();
	g_Level2_PersonList_temp2.push_back(g_Level2_PersonList_temp1[iSize_Level2_temp1 - 1]);
	for (int k = 0; k < iSize_Level2_temp1 - 1; k++)
	{
		g_Level2_PersonList_temp2.push_back(g_Level2_PersonList_temp1[k]);
	}

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex2.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex2[zz];
		g_Level2_PersonList_temp2.insert(g_Level2_PersonList_temp2.begin() + iLockIndex, g_Level2_PersonList[iLockIndex]);
	}


	g_Level2_PersonList.clear();
	iSize_Level2 = g_Level2_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level2; k++)
	{
		g_Level2_PersonList.push_back(g_Level2_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level2_PersonList_temp2[k]);

	}


	//vector<PersonBasicInfo> g_Level3_PersonList_temp;
	//int iSize_Level3 = g_Level3_PersonList.size();
	//g_Level3_PersonList_temp.push_back(g_Level3_PersonList[iSize_Level3 - 1]);
	//for (int k = 0; k < iSize_Level3 - 1; k++)
	//{
	//	g_Level3_PersonList_temp.push_back(g_Level3_PersonList[k]);
	//}

	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson3 = false;
	int iSize_Level3 = g_Level3_PersonList.size();
	vector<PersonBasicInfo> g_Level3_PersonList_temp1;
	vector<int> vectLockPersonIndex3;
	for (int k = 0; k < iSize_Level3; k++)
	{
		if (g_Level3_PersonList[k].bShiftLocked)
		{
			bHasLockPerson3 = true;
			vectLockPersonIndex3.push_back(k);
		}
		else
		{
			g_Level3_PersonList_temp1.push_back(g_Level3_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level3_PersonList_temp2;
	int iSize_Level3_temp1 = g_Level3_PersonList_temp1.size();
	g_Level3_PersonList_temp2.push_back(g_Level3_PersonList_temp1[iSize_Level3_temp1 - 1]);
	for (int k = 0; k < iSize_Level3_temp1 - 1; k++)
	{
		g_Level3_PersonList_temp2.push_back(g_Level3_PersonList_temp1[k]);
	}

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex3.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex3[zz];
		g_Level3_PersonList_temp2.insert(g_Level3_PersonList_temp2.begin() + iLockIndex, g_Level3_PersonList[iLockIndex]);
	}


	g_Level3_PersonList.clear();
	iSize_Level3 = g_Level3_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level3; k++)
	{
		g_Level3_PersonList.push_back(g_Level3_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level3_PersonList_temp2[k]);
	}




	//vector<PersonBasicInfo> g_Level4_PersonList_temp;
	//int iSize_Level4 = g_Level4_PersonList.size();
	//g_Level4_PersonList_temp.push_back(g_Level4_PersonList[iSize_Level4 - 1]);
	//for (int k = 0; k < iSize_Level4 - 1; k++)
	//{
	//	g_Level4_PersonList_temp.push_back(g_Level4_PersonList[k]);
	//}


	//把锁定的人员剔除，其他人员进行滚动，最后把锁定的人员加回来
	bool bHasLockPerson4 = false;
	int iSize_Level4 = g_Level4_PersonList.size();
	vector<PersonBasicInfo> g_Level4_PersonList_temp1;
	vector<int> vectLockPersonIndex4;
	for (int k = 0; k < iSize_Level4; k++)
	{
		if (g_Level4_PersonList[k].bShiftLocked)
		{
			bHasLockPerson4 = true;
			vectLockPersonIndex4.push_back(k);
		}
		else
		{
			g_Level4_PersonList_temp1.push_back(g_Level4_PersonList[k]);
		}
	}
	vector<PersonBasicInfo> g_Level4_PersonList_temp2;
	int iSize_Level4_temp1 = g_Level4_PersonList_temp1.size();
	g_Level4_PersonList_temp2.push_back(g_Level4_PersonList_temp1[iSize_Level4_temp1 - 1]);
	for (int k = 0; k < iSize_Level4_temp1 - 1; k++)
	{
		g_Level4_PersonList_temp2.push_back(g_Level4_PersonList_temp1[k]);
	}

	//再把锁定的元素放回去
	for (int zz = 0; zz < vectLockPersonIndex4.size(); zz++)
	{
		int iLockIndex = vectLockPersonIndex4[zz];
		g_Level4_PersonList_temp2.insert(g_Level4_PersonList_temp2.begin() + iLockIndex, g_Level4_PersonList[iLockIndex]);
	}

	g_Level4_PersonList.clear();
	iSize_Level4 = g_Level4_PersonList_temp2.size();
	for (int k = 0; k < iSize_Level4; k++)
	{
		g_Level4_PersonList.push_back(g_Level4_PersonList_temp2[k]);
		g_AllLevel_PersonList.push_back(g_Level4_PersonList_temp2[k]);
	}



	if (g_modifyClassSettingType == NoSaveToNextTemplate)
	{
		g_mapAllUserDefineClass.clear();
		//不保存，使用模板中的班次

		for (int iPerson = 0; iPerson < MaxPersonNum; iPerson++)
		{
			for (int iClassIndex = 0; iClassIndex < grade; iClassIndex++)
			{
				main_table[0][iPerson][iClassIndex] = main_table_init[0][iPerson][iClassIndex];
			}
		}

		ShowAllPersonToList(false, false, true);
	}
	else
	{

	}
	//ShowAllPersonToList(false, true, true);
	//PostMessage(WM_REFRESH_GRID, 0, 0);

	//OnBnSavePersonOrder();
	//OnBnClickedButton_Save();

	// 1. 同步全量刷新（包含人员姓名列 bRefreshPerson = true）
	ShowAllPersonToList(false, true, true);

	// 2. 强制重绘UI，让界面立刻看到变化
	m_pGrid.Invalidate();
	m_pGrid.RedrawWindow();

	// 3. 执行保存（此时 m_pGrid 里的文本已经是最新的了）
	OnBnSavePersonOrder();
	OnBnClickedButton_Save();

}

void CPIVASScheduleDlg::OnBnClickedButton_History()
{
	//打开选择文件对话框
    // 设置打开文件对话框的初始路径为当前程序目录
    TCHAR szFilter[] = _T("PIVAS排班历史文件 (*.xlsx)|*.xlsx||");
    CString strInitialDir;
    {
        TCHAR strFileName[MAX_PATH] = { 0 };
        GetModuleFileName(NULL, strFileName, MAX_PATH);
        CString strPath = strFileName;
        int pos = strPath.ReverseFind(_T('\\'));
        if (pos > 0) {
            strInitialDir = strPath.Left(pos);
        }
    }
    CFileDialog fileDialog(TRUE, nullptr, 0, OFN_HIDEREADONLY | OFN_EXPLORER, szFilter, this);

	strInitialDir += _T("\\排班历史文件\\");
    fileDialog.m_ofn.lpstrInitialDir = strInitialDir;
    if (fileDialog.DoModal() != IDOK) return;
	CString filepath = fileDialog.GetPathName();//全路径
	//将模板文件的数据加载到当前排班表中
	//先清空当前排班表数据
	m_pGrid.DeleteAllItems();

	//从文件加载

	m_pGrid.DeleteAllItems();
	g_Level1_PersonList.clear();
	g_Level2_PersonList.clear();
	g_Level3_PersonList.clear();
	g_Level4_PersonList.clear();
	g_AllLevel_PersonList.clear();
	string strHistoryFilePath;	 
	CStringtoString(strHistoryFilePath, filepath);
	ReadInitialSchedulFromModuleFile(strHistoryFilePath);
}

void CPIVASScheduleDlg::OnBnClickedButton_ModifyCanWorkClass()
{
	//打开修改可排班次的对话框，用户可以选择每个人可以排哪些班次，保存后在排班时只能选择这些班次
	CModifyPersonCanWorkClass theModifyCanWorkClassDlg;
	// 判断用户是否点击了“确定”按钮
	if (theModifyCanWorkClassDlg.DoModal() == IDOK)
	{
		// 同步刷新主界面网格（第三个参数 bRefreshClassInfo = true 会重新加载班次下拉框）
		ShowAllPersonToList(false, true, true);

		// 强制重绘，让界面立刻显示最新的下拉框选项
		m_pGrid.Invalidate();
		m_pGrid.RedrawWindow();
	}
	
}

void CPIVASScheduleDlg::OnBnClickedButton_SaveTemplate()
{
	 //弹框保存模板，弹出另存为对话框，路径为当前程序路径下
	TCHAR strFileName[100] = { 0 };
	STRING2TCHAR(g_filePath, strFileName, 100);
	CString strFilePath_temp = strFileName;
	strFilePath_temp += _T("\\排班模板\\排班模板.xlsx");

	CString filepath;

	// 注意：CFileDialog 的 filter 字符串必须以 '|' 分隔，且以 "||" 结尾，不能有多余的字符
	CFileDialog fileDlg(FALSE, _T("cxlsxsv"), strFilePath_temp, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY, _T("PIVAS排班模板文件 (*.xlsx)|*.xlsx||"));
	if (IDOK == fileDlg.DoModal())
	{
		filepath = fileDlg.GetPathName();//全路径
		CString fileName = fileDlg.GetFileExt();//.csv
		// 这里可以继续保存文件的逻辑
	}

	//TCHAR szFilter[] = _T("Worksheet Files (*.xlsx)|*.xlsx|Data Files (*.xlc;*.xls)|*.xlc; *.xls|ods Files (*.ods)|*.ods||");
	//CFileDialog fileDialog(TRUE, nullptr, 0, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, this);
	//if (fileDialog.DoModal() != IDOK) return;


	filepath = fileDlg.GetPathName();//全路径
	// 将当前排班表保存为模板文件
	SaveCurrentDataToModuleFile(filepath);
	//SaveScheduleAsTemplate(filepath);
}

void CPIVASScheduleDlg::OnBnClickedButton_PersonRest()
{
	//弹出所有人员的假期剩余天数列表，用户可以修改假期剩余天数
	CPersonRestDlg thePersonRestDlg;
	//thePersonRestDlg.DoModal();
	// 如果用户点击了“确定”按钮关闭了对话框
	if (thePersonRestDlg.DoModal() == IDOK)
	{
		// 1. 同步刷新主界面网格（bRefreshPerson = true 会重新加载姓名和剩余假期列）
		ShowAllPersonToList(false, true, true);

		// 2. 强制重绘，让界面立刻显示最新的假期天数
		m_pGrid.Invalidate();
		m_pGrid.RedrawWindow();
	}
}


void CPIVASScheduleDlg::SavePersonHasUsedRestDayToFile()
{


	//将当前所有数据保存到csv文件中
	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员已经使用的假期信息.csv";

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

	sprintf(chData, "序号, 人员, 已使用天数,\n");
	chData += strlen(chData);

	map<CString, PersonRestDay >::iterator iter1;
	int iIndex = 1;
	for (iter1 = g_mapPersonRestDayNum.begin(); iter1 != g_mapPersonRestDayNum.end(); iter1++)
	{
		CString strPersonName = iter1->first;
		PersonRestDay thePersonRestDay = iter1->second;

		string strPersonName1;
		CStringtoString(strPersonName1, strPersonName);
		strcat(chData, strPersonName1.c_str());
		strcat(chData, ",");


		sprintf(chData, "%d,%s,%d,\n", iIndex,strPersonName1.c_str(),thePersonRestDay.iHasUsedDay );
		chData += strlen(chData);

		iIndex++;
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
void CPIVASScheduleDlg::ReadPersonHasUsedRestDayFromFile()
{
	return;
	//从csv文件中读取人员已经使用假期天数
	FILE* fp;
	string filename = g_filePath;
	filename += "\\人员已经使用的假期信息.csv";

	//CStringtoString(filename, strFullFilePath);
	fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		cout << "无法打开文件" << endl;
		return;
	}
	char chLine[500] = { 0 };
	fgets(chLine, 500, fp); //跳过第一行标题
	while (!feof(fp))
	{
		memset(chLine, 0, 500);
		fgets(chLine, 500, fp);
		if (strlen(chLine) <= 0)
		{
			break;
		}
		//解析每一行数据
		char* pToken = NULL;
		pToken = strtok(chLine, ",");
		int iIndex = 0;
		CString strPersonName;
		int iHasUsedDay = 0;
		while (pToken != NULL)
		{
			if (iIndex == 1)
			{
				//人员名称
				TCHAR tcPersonName[100] = { 0 };
				STRING2TCHAR(pToken, tcPersonName, 100);
				strPersonName = tcPersonName;
			}
			else if (iIndex == 2)
			{
				//已经使用天数
				iHasUsedDay = atoi(pToken);
			}
			pToken = strtok(NULL, ",");
			iIndex++;
		}
		//将数据保存到map中
		map<CString, PersonRestDay >::iterator iter = g_mapPersonRestDayNum.find(strPersonName);
		if (iter != g_mapPersonRestDayNum.end())
		{
			iter->second.iHasUsedDay = iHasUsedDay;
		}
	}
}



void CPIVASScheduleDlg::OnBnClickedButton_AutoAdjust()
{

	//根据每个人的假期情况，对每天是调休的人员进行自动调整，将调休给到最高的假期人员头上，要是给不到就选次高
	g_vectorReturnClassPersonAndDay.clear();

	int iDayIndex = 0;
	for (iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
	{
		//找到当天调休的人员列表
		for (int iPersonIndex = 0; iPersonIndex < g_AllLevel_PersonList.size(); iPersonIndex++)
		{
			
			//判断该人员当天是否是调休
			//CString strClass = m_pGrid.GetItemText(iPersonIndex + 2, iDayIndex + 3);
			if (main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] == Relax_INIT)
			{
				//找到假期剩余天数最高的人员
				int iMaxRestDays = -1;
				int iMaxRestPersonIndex = -1;
				for (int jPersonIndex_Peer = 0; jPersonIndex_Peer < g_AllLevel_PersonList.size(); jPersonIndex_Peer++)
				{
					if (jPersonIndex_Peer == iPersonIndex)
					{
						continue;
					}

					CString strPersonName_Peer = g_AllLevel_PersonList[jPersonIndex_Peer].strPersonName.c_str();
					CString strPersonName_Self = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
					int iClass_OneDay_Peer = GetPersonOneDayWorkClassIndex(jPersonIndex_Peer, iDayIndex);

					if (bJudgeCanRestByClassIndex(iClass_OneDay_Peer) && iClass_OneDay_Peer>=0 && iClass_OneDay_Peer < Class_ChangeRelax)
					{


						int iPersonLevel_Self = FindPesrsonLevelByName(strPersonName_Self);


						map<CString, PersonRestDay >::iterator iter_temp_Peer = g_mapPersonRestDayNum.find(strPersonName_Peer);
						map<CString, PersonRestDay >::iterator iter_temp_Self = g_mapPersonRestDayNum.find(strPersonName_Self);

						int iRestDays_Peer = 0;
						int iRestDays_Self = 0;

						if (iter_temp_Peer != g_mapPersonRestDayNum.end())
						{
							iRestDays_Peer = iter_temp_Peer->second.iAllRestDay - iter_temp_Peer->second.iHasUsedDay;
						}
						if (iter_temp_Self != g_mapPersonRestDayNum.end())
						{
							iRestDays_Self = iter_temp_Self->second.iAllRestDay - iter_temp_Self->second.iHasUsedDay;
						}
						if (iRestDays_Peer > iRestDays_Self)
						{
							//iMaxRestDays = iRestDays;
							//iMaxRestPersonIndex = jPersonIndex;

							//将调休给到假期剩余天数最高的人员头上
							//查看这俩人是否能进行调休，要是能调休就进行调休
							
							if (NurseCanWorkClassesLevel[iPersonLevel_Self-1][iClass_OneDay_Peer] > 0)
							{


								//检查这个人当天上班是否满足连上5天，是否满足上了夜班不能上早班要求
								int iClassIndex_Peer = iClass_OneDay_Peer + iDayIndex * OneDayClasssNum;

								ChangeCurrentSheduleToTempTable();
								main_table_temp[0][iPersonIndex][iClassIndex_Peer] = Relax_TO_WORK;

								//检查是否合格
								ClassCheckErroType iErrorType;
								CString strErroDes;
								bool bIsChekOK = bCheckOnePersonOneDayClassIsOk(iPersonIndex, iDayIndex, iClass_OneDay_Peer, iErrorType, strErroDes);

								if(bIsChekOK)//能给换
								{
									main_table_temp[0][jPersonIndex_Peer][iClassIndex_Peer] = 0;
									int iClassIndex_relax = Class_ChangeRelax + iDayIndex * OneDayClasssNum;
									RelaxEnumType iRelaxType = (RelaxEnumType)(main_table[0][jPersonIndex_Peer][iDayIndex * OneDayClasssNum + Class_ChangeRelax]); //调休类型

									if (iRelaxType == Relax_TO_WORK)
									{
										main_table_temp[0][jPersonIndex_Peer][iClassIndex_relax] = Relax_INIT;
									}
									else
									{
										main_table_temp[0][jPersonIndex_Peer][iClassIndex_relax] = WORK_TO_Relax;
									}
									main_table_temp[0][iPersonIndex][iClassIndex_relax] = Relax_TO_WORK;


									iter_temp_Peer->second.iHasUsedDay += 1;
									if (1)
									{
										ChangeTempSheduleToCurrentTable();

										AddOneModifyToVect(iPersonIndex, iDayIndex);
										AddOneModifyToVect(jPersonIndex_Peer, iDayIndex);

										
										//ShowAllPersonToList(false, false, true);
									}

								}
							}


						}
					}

				}
				
				//if (iMaxRestPersonIndex != -1 && iMaxRestDays > 0)
				//{
				//	//修改排班表
				//	m_pGrid.SetItemText(iPersonIndex + 2, iDayIndex + 3, _T(" "));
				//	m_pGrid.SetItemText(iMaxRestPersonIndex + 2, iDayIndex + 3, _T("调休"));
				//	//更新假期剩余天数
				//	g_AllLevel_PersonList[iMaxRestPersonIndex].iRestDays -= 1;
				//	g_AllLevel_PersonList[iPersonIndex].iRestDays += 1;
				//}
			}
		}
	}


	ShowAllPersonToList(false, false, true);
	AddPlanToCurrentRedoArray(true);




}
void CPIVASScheduleDlg::OnBnClickedButton_RequestRest()
{
	//弹出人员申请休假对话框
	TakeLongHoliday theTakeLongHoliday;
	theTakeLongHoliday.DoModal();
}
void CPIVASScheduleDlg::OnBnClickedButton_ExportExcel()
{
	OnBnClickedButtonExportExcel();
}

void CPIVASScheduleDlg::OnBnClickedButton_Compare()
{
	TCHAR tchXlsxfilename[300] = { 0 };
	STRING2TCHAR(g_filePath, tchXlsxfilename, 300);

	CString strXlsxfilename = tchXlsxfilename;
	strXlsxfilename += _T("\\模板.xlsx");

	g_myClassVector.clear();
	ReadInitialXlsxToCsv(strXlsxfilename, g_myClassVector);


	ShowAllPersonToList(false, false, true);
	PostMessage(WM_REFRESH_GRID, 0, 0);
}



//void CPIVASScheduleDlg::OnBnClickedButton_Import_Temp()
//{
//	// 1. 打开选择文件的界面
//	TCHAR szFilter[] = _T("Worksheet Files (*.xlsx)|*.xlsx|Data Files (*.xlc;*.xls)|*.xlc; *.xls|ods Files (*.ods)|*.ods||");
//	CFileDialog fileDialog(TRUE, nullptr, 0, OFN_HIDEREADONLY | OFN_EXPLORER, szFilter, this);
//
//	if (fileDialog.DoModal() == IDOK)
//	{
//		CString filepath = fileDialog.GetPathName(); // 获取选中的文件全路径
//
//
//		// --- 新增功能：结束 EXCEL 进程、删除旧模板、拷贝新模板 ---
//		// 静默结束所有 "EXCEL.EXE" 进程
//		ShellExecute(NULL, _T("open"), _T("taskkill.exe"), _T("/F /IM EXCEL.EXE /T"), NULL, SW_HIDE);
//		Sleep(500); // 稍微等待一下，确保系统释放被 Excel 占用的文件句柄
//
//		// 【关键修复 1】：复活全局 g_ExcelApp
//		// 因为底层的 EXCEL.EXE 被杀死了，必须切断旧连接并重新初始化，否则后续调用会报 RPC 错误
//		try {
//			if (g_ExcelApp.m_lpDispatch != NULL) {
//				g_ExcelApp.ReleaseDispatch();
//			}
//			g_ExcelApp.CreateDispatch(_T("Excel.Application"), NULL);
//		}
//		catch (...) {
//			// 忽略初始化时的偶发异常
//		}
//
//
//		// 将全局 string 路径转换为 CString
//		TCHAR tchCurrentDir[300] = { 0 };
//		STRING2TCHAR(g_filePath, tchCurrentDir, 300);
//		CString strCurrentDir = tchCurrentDir;
//
//		// 拼接需要操作的文件路径
//		CString strLastTemplate = strCurrentDir + _T("\\模板_Last.xlsx");
//		CString strMainTemplate = strCurrentDir + _T("\\模板.xlsx");
//
//		// 1. 强制去除文件的只读/隐藏属性 (关键步骤)
//		::SetFileAttributes(strLastTemplate, FILE_ATTRIBUTE_NORMAL);
//		::SetFileAttributes(strMainTemplate, FILE_ATTRIBUTE_NORMAL);
//
//		// 删除当前目录下的旧模板文件
//		::DeleteFile(strLastTemplate);
//		::DeleteFile(strMainTemplate);
//
//		// 将用户选择的文件拷贝到当前目录并重命名为 "模板.xlsx"
//		::CopyFile(filepath, strMainTemplate, FALSE);
//
//
//
//		// 2. 清空当前 Grid 控件的所有内容
//		m_pGrid.DeleteAllItems();
//
//		// 3. 清空内存中现有的各级别人员列表数据，防止与新导入的数据叠加
//		g_Level1_PersonList.clear();
//		g_Level2_PersonList.clear();
//		g_Level3_PersonList.clear();
//		g_Level4_PersonList.clear();
//		g_AllLevel_PersonList.clear();
//
//
//		// ========= 【关键修复】：彻底清空全局排班数据矩阵和相关字典 =========
//		// 如果不清空，A 模板为空白的格子无法覆盖 B 模板的班次，导致数据错乱叠加
//		memset(main_table, 0, sizeof(main_table));
//		memset(main_table_temp, 0, sizeof(main_table_temp));
//		memset(main_table_init, 0, sizeof(main_table_init));
//		memset(main_table_undo, 0, sizeof(main_table_undo)); // 顺手把撤销栈清空，防止时空错乱
//
//		g_mapAllRestClass.clear();
//		g_mapAllUserDefineClass.clear();
//		g_vectorReturnClassPersonAndDay.clear();
//		g_vectorPersonCanRest.clear();
//		// 重置撤销/恢复步数
//		g_undo_AllNum = 0;
//		g_undo_index = -1;
//
//
//		// 4. 将 CString 格式的路径转换为 std::string 格式
//		string strImportFilePath;
//		CStringtoString(strImportFilePath, filepath);
//
//		// 5. 调用现有的模板加载函数，传入选定的文件路径
//		// 该函数内部会自动处理 xlsx转csv -> 解析数据 -> 初始化Grid列 -> 调用ShowAllPersonToList显示数据
//		ReadInitialSchedulFromModuleFile(strImportFilePath,true);
//
//
//		AddPlanToCurrentRedoArray();
//		// 6. （可选）如果你还需要同步人员之前的排序设置，可以解开这行注释
//		// ReadPersonOrderFromFile();
//
//		// 7. 发送消息强制重绘和刷新 Grid，确保UI立即呈现最新状态
//		PostMessage(WM_REFRESH_GRID, 0, 0);
//	}
//}


void CPIVASScheduleDlg::OnBnClickedButton_Import_Temp()
{
	// 1. 打开选择文件的界面
	TCHAR szFilter[] = _T("Worksheet Files (*.xlsx)|*.xlsx|Data Files (*.xlc;*.xls)|*.xlc; *.xls|ods Files (*.ods)|*.ods||");
	CFileDialog fileDialog(TRUE, nullptr, 0, OFN_HIDEREADONLY | OFN_EXPLORER, szFilter, this);

	if (fileDialog.DoModal() == IDOK)
	{
		CString filepath = fileDialog.GetPathName(); // 获取选中的文件全路径

		// --- 新增功能：结束 EXCEL 进程、删除旧模板、拷贝新模板 ---
		// 静默结束所有 "EXCEL.EXE" 进程
		ShellExecute(NULL, _T("open"), _T("taskkill.exe"), _T("/F /IM EXCEL.EXE /T"), NULL, SW_HIDE);
		Sleep(500); // 稍微等待一下，确保系统释放被 Excel 占用的文件句柄

		// 【关键修复 1】：复活全局 g_ExcelApp
		// 因为底层的 EXCEL.EXE 被杀死了，必须切断旧连接并重新初始化，否则后续调用会报 RPC 错误
		try {
			if (g_ExcelApp.m_lpDispatch != NULL) {
				g_ExcelApp.ReleaseDispatch();
			}
			g_ExcelApp.CreateDispatch(_T("Excel.Application"), NULL);
		}
		catch (...) {
			// 忽略初始化时的偶发异常
		}

		// 将全局 string 路径转换为 CString
		TCHAR tchCurrentDir[300] = { 0 };
		STRING2TCHAR(g_filePath, tchCurrentDir, 300);
		CString strCurrentDir = tchCurrentDir;

		// 拼接需要操作的文件路径
		CString strLastTemplate = strCurrentDir + _T("\\模板_Last.xlsx");
		CString strMainTemplate = strCurrentDir + _T("\\模板.xlsx");
		CString strPersonOrder = strCurrentDir + _T("\\人员顺序.csv"); // 【新增】: 人员顺序文件路径

		// 1. 强制去除文件的只读/隐藏属性 (关键步骤，防止因只读属性导致删除失败)
		::SetFileAttributes(strLastTemplate, FILE_ATTRIBUTE_NORMAL);
		::SetFileAttributes(strMainTemplate, FILE_ATTRIBUTE_NORMAL);
		::SetFileAttributes(strPersonOrder, FILE_ATTRIBUTE_NORMAL); // 【新增】: 去除人员顺序文件的只读属性

		// ==================== 错误校验与删除逻辑 ====================
		CString strErrorMsg;

		// 删除当前目录下的旧模板_Last文件
		if (!::DeleteFile(strLastTemplate)) {
			DWORD dwError = ::GetLastError();
			// 如果文件不存在是正常的，跳过报错；其他错误（如被占用）则拦截
			if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND) {
				strErrorMsg.Format(_T("删除旧文件失败: 模板_Last.xlsx\n错误码: %d\n请确保该文件未被其他程序占用后重试！"), dwError);
				MessageBox(strErrorMsg, _T("导入模板失败"), MB_OK | MB_ICONERROR);
				return; // 终止导入流程
			}
		}

		// 删除当前目录下的主模板文件
		if (!::DeleteFile(strMainTemplate)) {
			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND) {
				strErrorMsg.Format(_T("删除旧文件失败: 模板.xlsx\n错误码: %d\n请确保该文件未被其他程序占用后重试！"), dwError);
				MessageBox(strErrorMsg, _T("导入模板失败"), MB_OK | MB_ICONERROR);
				return; // 终止导入流程
			}
		}

		// 【新增】: 删除当前目录下的“人员顺序.csv”文件并增加错误校验
		if (!::DeleteFile(strPersonOrder)) {
			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_FILE_NOT_FOUND && dwError != ERROR_PATH_NOT_FOUND) {
				strErrorMsg.Format(_T("删除文件失败: 人员顺序.csv\n错误码: %d\n请确保该文件未被其他程序占用后重试！"), dwError);
				MessageBox(strErrorMsg, _T("导入模板失败"), MB_OK | MB_ICONERROR);
				return; // 终止导入流程
			}
		}

		// 将用户选择的文件拷贝到当前目录并重命名为 "模板.xlsx"
		if (!::CopyFile(filepath, strMainTemplate, FALSE)) {
			DWORD dwError = ::GetLastError();
			strErrorMsg.Format(_T("拷贝新模板文件失败！\n源文件: %s\n错误码: %d\n请检查磁盘空间或权限设置！"), filepath, dwError);
			MessageBox(strErrorMsg, _T("导入模板失败"), MB_OK | MB_ICONERROR);
			return; // 终止导入流程
		}
		// ==========================================================

		// 2. 清空当前 Grid 控件的所有内容
		m_pGrid.DeleteAllItems();

		// 3. 清空内存中现有的各级别人员列表数据，防止与新导入的数据叠加
		g_Level1_PersonList.clear();
		g_Level2_PersonList.clear();
		g_Level3_PersonList.clear();
		g_Level4_PersonList.clear();
		g_AllLevel_PersonList.clear();

		// =========彻底清空全局排班数据矩阵和相关字典 =========
		memset(main_table, 0, sizeof(main_table));
		memset(main_table_temp, 0, sizeof(main_table_temp));
		memset(main_table_init, 0, sizeof(main_table_init));
		memset(main_table_undo, 0, sizeof(main_table_undo));

		g_mapAllRestClass.clear();
		g_mapAllUserDefineClass.clear();
		g_vectorReturnClassPersonAndDay.clear();
		g_vectorPersonCanRest.clear();

		// 重置撤销/恢复步数
		g_undo_AllNum = 0;
		g_undo_index = -1;

		// 4. 将 CString 格式的路径转换为 std::string 格式
		string strImportFilePath;
		CStringtoString(strImportFilePath, filepath);

		// 5. 调用现有的模板加载函数，传入选定的文件路径
		ReadInitialSchedulFromModuleFile(strImportFilePath, true);

		AddPlanToCurrentRedoArray();

		// 7. 发送消息强制重绘和刷新 Grid，确保UI立即呈现最新状态
		PostMessage(WM_REFRESH_GRID, 0, 0);
	}
}

// 在 PIVAS_ScheduleDlg.cpp 中添加：
void CPIVASScheduleDlg::ReadPersonCanWorkClassFromFile()
{
	// 拼接文件全路径
	std::string filename = g_filePath + "\\PersonCanWorkClass.csv";
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp == NULL)
	{
		// 如果文件不存在，说明用户尚未保存过自定义配置，直接沿用代码里默认的硬编码初始化值
		OutputDebugString(_T("[Warning] PersonCanWorkClass.csv 不存在，将使用代码默认的班次权限配置。\n"));
		return;
	}

	char chLine[1024] = { 0 };
	int iLevel = 0;

	// 逐行读取 CSV 文件，最多读取 4 个级别
	while (fgets(chLine, sizeof(chLine), fp) != NULL && iLevel < 4)
	{
		// 去除行尾的换行符
		std::string strLine(chLine);
		if (!strLine.empty() && strLine.back() == '\n') strLine.pop_back();
		if (!strLine.empty() && strLine.back() == '\r') strLine.pop_back();

		if (strLine.empty()) continue;

		// 使用 strtok 拆分逗号隔开的每一个班次状态值
		char* pToken = strtok((char*)strLine.c_str(), ",");
		int iClass = 0;

		//每行的列数必须和 CModifyPersonCanWorkClass 写文件时保持一致。
		//原来写文件写31列、这里只读30列，最后一列(QA)永远读不回来，
		//导致重启前后 QA 的权限不一样，批量删除时有时删得掉、有时删不掉。
		while (pToken != NULL && iClass < CanWorkClassCheckNum)
		{
			int canWork = atoi(pToken);
			if (canWork > 0)
			{
				// 10 代表该级别护士在该班次可以上班（产出标准值）
				NurseCanWorkClassesLevel[iLevel][iClass] = 10;
			}
			else
			{
				// 0 代表该级别护士无法上该班次
				NurseCanWorkClassesLevel[iLevel][iClass] = 0;
			}

			pToken = strtok(NULL, ",");
			iClass++;
		}
		iLevel++;
	}

	fclose(fp);
	OutputDebugString(_T("[Info] 成功从 PersonCanWorkClass.csv 加载并更新了人员可上班次配置。\n"));
}

//调休安排成功之后，顺手让客户给这个格子填一个自定义班次/说明(补端午、年假之类)。
//以前安排完统一写死"调休(调整)"，客户要改还得先选"删除班次"把格子清空再输入，很费事。
//列表里给出常用项，也允许直接手打任意文字；点取消就保持"调休(调整)"不变。
void AskFillRelaxClassAfterArrange(CPIVASScheduleDlg* pMainDlg, int iPersonIndex, int iDayIndex)
{
	if (pMainDlg == nullptr)
	{
		return;
	}
	if (iPersonIndex < 0 || iPersonIndex >= (int)g_AllLevel_PersonList.size())
	{
		return;
	}
	if (iDayIndex < 0 || iDayIndex >= AllDay)
	{
		return;
	}

	//下拉里给出：节假日调休名称(补五一/补端午…) + 调休/补休 + 本级别能上的班次
	vector<CString> vecOptions;
	for (map<CString, int>::iterator iter_Holi = g_ModfifyResttMap.begin(); iter_Holi != g_ModfifyResttMap.end(); iter_Holi++)
	{
		vecOptions.push_back(iter_Holi->first);
	}
	vecOptions.push_back(RelaxClassName_MODIFY);
	vecOptions.push_back(RESTClassName);

	CString strPersonName = g_AllLevel_PersonList[iPersonIndex].strPersonName.c_str();
	int iPersonLevel = pMainDlg->FindPesrsonLevelByName(strPersonName);
	if (iPersonLevel >= 1 && iPersonLevel <= nurse_level)
	{
		for (int iClassIndex_OneDay = 0; iClassIndex_OneDay < OneDayClasssNum - 1; iClassIndex_OneDay++)
		{
			if (NurseCanWorkClassesLevel[iPersonLevel - 1][iClassIndex_OneDay] > 0)
			{
				TCHAR strClassName_temp[100] = { 0 };
				STRING2TCHAR(ClassName[iClassIndex_OneDay], strClassName_temp, 100);
				vecOptions.push_back(strClassName_temp);
			}
		}
	}

	CString strTip;
	strTip.Format(_T("%s 第%d天的调休已安排成功，请选择或直接输入要填入该格子的班次/说明："), strPersonName, iDayIndex + 1);

	CHoliDayRestDlg dlgFill(pMainDlg);
	dlgFill.SetFillMode(strTip, vecOptions);
	if (dlgFill.DoModal() != IDOK)
	{
		return;		//取消：保持"调休(调整)"
	}

	CString strFill = dlgFill.m_strHoliDayName;
	strFill.Trim();
	if (strFill.IsEmpty())
	{
		return;
	}

	CString strKey;
	strKey.Format(_T("%d-%d"), iPersonIndex, iDayIndex);

	map<CString, int>::iterator iterHoli = g_ModfifyResttMap.find(strFill);
	if (iterHoli != g_ModfifyResttMap.end())
	{
		//选的是"补五一""补端午"这类节假日调休：按节假日调休的值存，格子文字自然就是它
		int iValue = iterHoli->second + BU_HOLIDAY_1_WORK_TO_Relax - BU_HOLIDAY_1;
		main_table[0][iPersonIndex][iDayIndex * OneDayClasssNum + Class_ChangeRelax] = iValue;
		g_mapAllUserDefineClass.erase(strKey);
	}
	else if (strFill == RelaxClassName_MODIFY || strFill == RelaxClassName)
	{
		//还是用回"调休(调整)"，把自定义说明清掉即可
		g_mapAllUserDefineClass.erase(strKey);
	}
	else
	{
		//其它一律当成这个格子的自定义说明，调休标记原样保留，调休统计不受影响
		g_mapAllUserDefineClass[strKey] = strFill;
	}

	pMainDlg->AddOneModifyToVect(iPersonIndex, iDayIndex);
	pMainDlg->AddPlanToCurrentRedoArray();
	if (g_bAutoSave)
	{
		pMainDlg->SetTimer(TIMER_AUTO_SAVE, 2000, NULL);
	}
	pMainDlg->PostMessage(WM_REFRESH_GRID, 0, 0);
}

void CPIVASScheduleDlg::UpdateGridColorsOnly()
{
	int iPersonNum = g_AllLevel_PersonList.size();

	// 遍历所有人员和天数，利用 O(1) 坐标映射直接修改单元格背景色
	for (int iPersonIndex = 0; iPersonIndex < iPersonNum; iPersonIndex++)
	{
		int iRow = iPersonIndex + 2; // 精准映射到 Grid 的行

		for (int iDayIndex = 0; iDayIndex < AllDay; iDayIndex++)
		{
			int iCol = iDayIndex + 3; // 精准映射到 Grid 的列

			CGridCellBase* pCell = m_pGrid.GetCell(iRow, iCol);
			if (!pCell) continue;

			// --- 1. 还原该单元格原本的底色（擦除上一个方案留下的高亮） ---
			COLORREF clr_BK = G_Bk_Defalut_COLORREF; // 默认白色底

			int iClassIndex_OneDay_temp = -1;
			int iClass_ChangeRelaxValue = 0;
			GetClassNameOneDayOnePerson(iDayIndex, iPersonIndex, iClassIndex_OneDay_temp, iClass_ChangeRelaxValue);

			// 还原还班/调休等特殊班次的背景色（保持与 ShowOneLevelPersonToList 逻辑同步）
			//和 ShowOneLevelPersonToList 保持同一套底色规则：
			//  蓝底=本轮成功安排的调休   浅黄底=本轮被调出来顶班   橙底=当前正在预览的方案
			if (iClass_ChangeRelaxValue == WORK_TO_Relax ||
				iClass_ChangeRelaxValue == WORK_TO_REST_INIT ||
				iClass_ChangeRelaxValue >= BU_HOLIDAY_1_WORK_TO_Relax)
			{
				clr_BK = G_Bk_RelaxArranged_COLORREF;
			}
			else if (iClass_ChangeRelaxValue == Relax_TO_WORK)
			{
				clr_BK = G_Bk_TakeOverWork_COLORREF;
			}

			//本轮操作过的格子铺一层浅灰底，和 ShowOneLevelPersonToList 保持一致
			if (clr_BK == G_Bk_Defalut_COLORREF && bJudgeCellHasOperated(iPersonIndex, iDayIndex))
			{
				clr_BK = G_Bk_Operated_COLORREF;
			}

			//和原始排班表的差异改用"红色字体"标注(在 ShowOneLevelPersonToList 里统一处理)，
			//这里不再拿它去改底色 —— 原来这段用的 key 公式(行*14+列)和填表、显示两处都不一样，
			//算出来的是上一行的格子，底色自然乱跳。

			// --- 2. 叠加上当前新方案的高亮颜色 ---
			for (size_t mm = 0; mm < g_vectorReturnClassPersonAndDay.size(); mm++)
			{
				if (g_vectorReturnClassPersonAndDay[mm].iPersonIndex == iPersonIndex &&
					g_vectorReturnClassPersonAndDay[mm].iDayIndex == iDayIndex)
				{
					clr_BK = G_Bk_ReturnClass_COLORREF_MODIFY; // 赋予新预览方案的高亮色
					break;
				}
			}

			// 【核心优化点】：直接调用底层指针修改背景色。
			// 绝不调用 SetCellType，绝不重新配置下拉菜单，性能提升上百倍，完全不会发生阻塞
			pCell->SetBackClr(clr_BK);
		}
	}

	// 仅使当前网格视图失效并强制重绘，开销极小
	m_pGrid.Invalidate();
	m_pGrid.RedrawWindow();
}

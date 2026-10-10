
// PIVAS_ScheduleDlg.h: 头文件
//
#include "CDlgRestDayManagement.h"
#include "SchedulCommonStruct.h"
#include "GridCtrl_src/GridCtrl.h"

#include "ISAuthorizeDLL_DrugTrace.h"
#define WM_REFRESH_GRID (WM_USER + 100)
#pragma once
#define TIMER_AUTO_SAVE 300

#include <map>



// CPIVASScheduleDlg 对话框
class CPIVASScheduleDlg : public CDialogEx
{
// 构造
public:
	CPIVASScheduleDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PIVAS_SCHEDULE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	NOTIFYICONDATA m_notifyIconData;	//托盘图标对象
	void AddTray();
	afx_msg LRESULT OnMyIconNotify(WPARAM wParam, LPARAM lParam);		//托盘鼠标事件响应
	afx_msg void  QuitEngine();	//退出程序
	afx_msg void  ActiveSoftware();	//退出程序
	afx_msg LRESULT OnRefreshGrid(WPARAM wParam, LPARAM lParam);
	bool m_bIsUpdatingGrid;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//CListCtrl m_ListCtrl;
	CDlgRestDayManagement *m_pDlgRestDayManagement;
	//CGridCtrl m_pGrid;

public:
	int FindeClassIndex(CString stData);
	int FindPesrsonLevelByName(CString strPersonName);
	int FindPesrsonIndexByName(CString strPersonName);
	int GetPersonOneDayWorkClassIndex(int iPersonIndex, int iDayIndex);

	//查找当天调休的人员
	int GetThisDayRestPersons(int iDayIndex, vector<int>&NoWorkPersonList,bool bCanFindHasScheduled = false);
	//从指定人员中查找能上指定班的人员
	int GetThisDayCanWorPersons(int iDayIndex, int iClass_OneDay, vector<int>& NoWorkPersonList, vector<int>& NoWorkPersonCanWorkInThisClass);
	
	//查找当天原先是调休改成工作的人员
	int GetThisDayRelaxToWorkPersons(int iDayIndex, vector<int>& RelaxToWorkPersonList);

	
	//获取某人某天的班次，及调休班次的值
	CString GetClassNameOneDayOnePerson(int iDayIndex, int iPersonIndex,int &iClassIndexOneDay,int &iClass_ChangeRelaxValue);

	bool JudgeCanSwitchClassByTwoPerson(int iPersonIndex_A, int iDayIndex_A, int iClassIndex_OneDay_A, int iPersonLevel_A,
		int iPersonIndex_B, int iDayIndex_B, int iClassIndex_OneDay_B, int iPersonLevel_B,ClassCheckErroType& iErrorType);

	//获取某天上指定班次的人
	void GetPersonOneDayWorkInOneClase(int iDayIndex, int iClass_OneDay, int& iPersonIndex);


	bool bJudgeCanRestByClassIndex(int iClassIndex);


	void ReadRuleConfigFromIni();


	void ParseOneRangeData(int iRowIndex, int iCowIndex, CString stData, int& iPersonLevel);
	void ReadInitialSchedulFromModuleFile(string strAllPatch, bool bIsImport = false);

	void ReadInitialXlsxToCsv(CString strXlsxPatch,map<UINT,CString> &myClassVector,bool bIsImport = false);



	void ReadStartTimeFromFile();

	//从最初始的文件中解析休息的班次
	void ReadInitialSchedulFromModuleFile_FromInitFile();
	void ParseOneRangeData_FromInitFile(int iRowIndex, int iCowIndex, CString stData, int& iPersonLevel);


	void ReadLastRestDayDataFromFile();
	void ReadPersonOrderFromFile();

	void ShowAllPersonToList(bool bRefreshIndex_Level,bool bRefreshPerson,bool bRefreshClassInfo);

	void ShowOneLevelPersonToList(vector<PersonBasicInfo>& theLevel_PersonList, bool bRefreshIndex_Level, bool bRefreshPerson, bool bRefreshClassInfo, int &iLineIndex, int &iColumnIndex, CString strLevelName, int iCurrentPersonLevel);


	void PutPersonLastRestInfoToVect(CString strPersonName, CString strRestType, int iDayNumber, RestEnumType iSureRestDay = NO_REST,int iDayIndex = -1);


	void ChangeCurrentSheduleToTempTable();
	void ChangeTempSheduleToCurrentTable();
	bool bCheckOnePersonOneDayClassIsOk(int iPersonIndex,int iDayIndex,int iClassIndexOneDay, ClassCheckErroType &iErrorType,CString &strErroDes);


	bool CheckPersonRestCanModifyOk(CString strPersonName, CTime cStartTime, CTime cEndTime);

	//afx_msg void OnBnClickedButton_Next();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnOpenRestManageDlg();
	CGridCtrl m_pGrid;

	void GridCtrlInit();


	afx_msg void OnGridClick(NMHDR* pNotifyStruct, LRESULT* pResult);

	afx_msg void OnGridDblClick(NMHDR* pNotifyStruct, LRESULT* pResult);
	afx_msg void OnGridEndEdit(NMHDR* pNotifyStruct, LRESULT* pResult);
	bool OnAdjustRestAndNeeWorkPerson_EX(int iPersonIndex, int iDayIndex, int iClass_OneDay,int &iPesonIndex_Rest,bool bChange = true,bool bIsRestClass = false);


	// 调整休息与需要上班的人员 (穷举返回所有可能方案)
	// @param bEnableThreePersonSwap: 是否允许进行三人轮换的深度查找
	bool OnAdjustRestAndNeeWorkPerson_EX_new(int iPersonIndex, int iDayIndex, int iClass_OneDay, vector<RestAdjustPlan>& outPlans, vector<int> &vecLevelOrder,
		bool bIsRestClass = false, bool bEnableThreePersonSwap = true, int iMaxChainPersons = ChainMaxPersonNum_Default);

	// N人链式联动换班查找：A腾出班次 -> B接A的班 -> C接B的班 -> ... -> 当天调休的人接最后一个人的班
	// 适用于 A、T、B、C、D、Z 等级别受限、两三人换不动的班次
	// @param iMaxChainPersons: 一条链最多几个人(含申请人A)，2表示不做链式查找
	bool FindChainSwapPlans(int iPersonIndex, int iDayIndex, int iClass_OneDay, vector<int>& vecLevelOrder,
		int iMaxChainPersons, vector<RestAdjustPlan>& outPlans);

	// 链式查找的递归体：iClassNeedCover 为当前空出来、需要有人顶上的班次
	void DoChainSwapDfs(ChainSearchCtx& ctx, int iClassNeedCover);

	// 判断把某人挪到新班次是不是"制造了新的违规"。
	// 调用前 main_table 必须already处于换班后的状态；iClassIndex_Old 为该人原班次，-1表示原本休息。
	// 现有排班里本来就存在的违规(例如这个人本轮已经连上5天)不算在这次换班头上，
	// 否则表里只要有一处历史违规，这个人就再也参与不了任何换班。
	bool bChainMoveMakeNewErro(int iPersonIndex, int iDayIndex, int iClassIndex_New, int iClassIndex_Old, CString& strErroDes_Out);

	// 把一条完整的链记录成一个可选方案
	void MakeOneChainPlan(ChainSearchCtx& ctx, int iEndPersonIndex, bool bIsUniversalCell);

	// 判断某人某天是不是"万能格"：用了"删除班次"之后留下的空白格。
	// 万能格没有班次、没有调休标记，也不是模板里写死的"休"，可以接任何本级别能上的班。
	bool bJudgeIsUniversalCell(int iPersonIndex, int iDayIndex);

	// 找出当天所有万能格人员（换班时最优先把班给他们）
	int GetThisDayUniversalCellPersons(int iDayIndex, vector<int>& UniversalPersonList);

	// 批量删除班次，删完的格子变成空白的万能格。
	// iDeleteType: 1=调休类班次(调休/补休/补五一…)  2=自定义班次(补端毛/Z破…)  3=范围内的全部班次
	// bUseSelection: true=只处理表格里选中的区域，false=整张表
	// bOnlyCount: true=只统计有多少格会被删，不真的改数据
	int BatchDeleteClass(int iDeleteType, bool bUseSelection, bool bOnlyCount);

	// 右键菜单里"批量删除"各项的入口
	void OnBatchDeleteClass(int iDeleteType, bool bUseSelection);

	bool OnAdjustRestAndNeeWorkPerson_NeedReturnClass(int iPersonIndex, int iDayIndex, int iClass_OneDay);
	bool OnAdjustRestAndNeeWorkPerson_NeedReturnClass_EX(int iPersonIndex, int iDayIndex, int iClass_OneDay);




	void GetOnePersonRelaxDays(int iPersonIndex, vector<int> &vecRelaxDays);


	bool ChangeTwoDayClass(int iPersonIndex, CString strPersonName_request, int iDayIndex, int iClass_OneDay,CString strClassName, int iPersonIndex_temp, vector<int> &vecRelaxDays, CString& strOutPut, RelaxEnumType iRelaxType);


	afx_msg void OnBnSavePersonOrder();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CSize m_OldSize;

	CEdit m_TraceWnd;
	afx_msg void OnBnClickedButtonRule();
	void OnGridRClick(NMHDR *pNotifyStruct, LRESULT* /*pResult*/);
	afx_msg void OnDBEditorInsert();


	void AddOnePersonByLevel(wstring strPesonName,int iPersonIndex, int iSelectLevel, bool bIsEnd, bool bIsFreeze);

	bool SwitchOnePersonOneDayToCurrentClass(int iPersonIndex_A, int iDayIndex, CString itemText,int iClassIndexOneDay_A);
	void MergeCells(int nStartRow, int nStartCol, int nEndRow, int nEndCol);

	//afx_msg void OnBnClickedButton_Save();
	afx_msg void OnBnClickedButtonExportExcel();


	void ExportDataToExcel(vector<vector<CString>>& vecTableData, CString filepath,CString strTitle, vector<vector<CString>>& vecTableHeadData);

	void TakeRetunrClassToCurentTable(ReturnClassStruct &theReturnClassInfo);
	afx_msg void OnTimer(UINT_PTR nIDEvent);



	BOOL ReadAccessRegEdit();

	BOOL writeAccessRegEdit();

	int m_RemainingTime;

	void AddOneModifyToVect(int iPersonIndex, int iDayIndex);

	void AddOnePersonCanRestToVect(int iPersonIndex, int iDayIndex);

	//afx_msg void OnBnClickedButtonOperCancle();
	//void OnBnClickedButtonOperResume();


	void AddPlanToCurrentRedoArray(bool bHasModifyPersonRest = false);


	CISAuthorizeDLL_DRUGTRACE *m_pAuthorizeDLL;
	afx_msg void OnClose();

	CToolBar m_Toolbar;
	CImageList m_Imagelist;

	void ParseChineseTxtToMap(CString strTxtPath, std::map<CString, int>& resultMap, std::map<int, CString >& BuHoliDayMap, bool bCountFrequency);


	void SaveCurrentDataToModuleFile(CString strFullFilePath);

	void SavePersonHasUsedRestDayToFile();
	void ReadPersonHasUsedRestDayFromFile();

	// 在 PIVAS_ScheduleDlg.h 中添加：
	void ReadPersonCanWorkClassFromFile();
	bool JudgeIsSpecialClass(int iClassIndexOneDay);

	afx_msg void OnBnClickedButton_Previous();
	afx_msg void OnBnClickedButton_Next();
	afx_msg void OnBnClickedButton_Save();
	afx_msg void OnBnClickedButton_Undo();
	afx_msg void OnBnClickedButton_Redo();
	afx_msg void OnBnClickedButton_History();
	afx_msg void OnBnClickedButton_ModifyCanWorkClass();
	afx_msg void OnBnClickedButton_SaveTemplate();
	afx_msg void OnBnClickedButton_PersonRest();

	afx_msg void OnBnClickedButton_AutoAdjust();
	afx_msg void OnBnClickedButton_RequestRest();

	afx_msg void OnBnClickedButton_ExportExcel();
	afx_msg void OnBnClickedButton_Compare();

	afx_msg void OnBnClickedButton_Import_Temp();


	void UpdateGridColorsOnly();

};

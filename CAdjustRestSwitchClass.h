#pragma once
#include "afxdialogex.h"
#include "SchedulCommonStruct.h"

// CAdjustRestSwitchClass 对话框

class CAdjustRestSwitchClass : public CDialogEx
{
	DECLARE_DYNAMIC(CAdjustRestSwitchClass)

public:
	CAdjustRestSwitchClass(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CAdjustRestSwitchClass();

	void InitData(vector<RestAdjustPlan>& outPlans, int iPersonIndex, int iDayIndex, int iClassIndexOneDay, CString itemText, bool bIsHoliDayRst, int HoliDayRlexIndex, int iRow, int iCol);
	vector<RestAdjustPlan> m_theAllPlanClassInfo;
	int m_iCurrentSelectIndex;
	void ShowReturnClassInfo();

	// 资源里方案说明只有一行高，4人联动的文字根本放不下。
	// 这里在运行时把窗口和几个文字控件放大，让文字自动换行显示完整（不用改资源文件）。
	void ResizeForLongText();

	// 运行时补出来的两个控件：方案下拉框（方案多的时候直接跳，不用一个个点"下一个"）
	// 和"展开详情"按钮（默认只看候选人名单，要看链路再展开）。
	// 资源文件里没有这两个控件，全部在代码里建，省得动 .rc。
	void CreateRuntimeControls();
	void FillPlanListCombo();

	// 方案的一句话摘要："嚣乐乐 接班（两人换班）"
	CString MakePlanBriefText(RestAdjustPlan& thePlan);

	CComboBox m_comboPlanList;		//方案下拉框
	CButton m_btnToggleDetail;		//展开/收起详情
	bool m_bShowDetail;				//当前是不是展开着详情



	// 新增：保存换班所需的上下文状态
	int m_iPersonIndex;
	int m_iDayIndex;
	int m_iClassIndexOneDay;
	CString m_itemText;
	bool m_bIsHoliDayRst;
	int m_HoliDayRlexIndex;
	int m_iRow;
	int m_iCol;

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAdjustRestSwitchClass };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void PostNcDestroy(); // 新增：用于非模态对话框的销毁
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonLastPlan();
	afx_msg void OnBnClickedButtonNextPlan();
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCancel();

	afx_msg void OnBnClickedToggleDetail();		//展开/收起方案详情
	afx_msg void OnSelchangePlanList();			//从下拉框里直接跳到某个方案

};

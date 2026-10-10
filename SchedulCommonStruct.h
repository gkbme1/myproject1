#pragma once
#if !defined(AFX_SENDVALUETOHDBTHREAD_H__0E47D2C2_feee_4e83_BBAE_0DF618BED242__INCLUDED_)
#define AFX_SENDVALUETOHDBTHREAD_H__0E47D2C2_feee_4e83_BBAE_0DF618BED242__INCLUDED_
////////////////////////////////////////////////////////////如下是班次的信息定义///////////////////////////////////////////////////////

#include <string>
#include <vector>
#include <iostream>
using namespace std;


//一天的所有班次数目
#define OneDayClasssNum  32									//定义一天的班次数量(早、中、晚)
#define OneDayMaxClasssNum  100	

#define MaxPersonNum 100										//定义最大人数
#define AllDay 14
#define quantity 5										//定义总群数量
#define grade AllDay*OneDayClasssNum						//定义班次数量

#define MaxUndoNum	50										//定义总群数量


#define Class_2_3   2
#define Class_2_4   3
#define Class_2_5   4
#define Class_2_6   5

//定义3班次的索引号
#define Class_3  6		
	
#define Class_3__  8	
#define Class_3___1 9	

//定义3-1班次的索引号
#define Class_3_1  10									

#define Class_3_2  11

//定义3-3班次的索引号
#define Class_3_3  12	

#define Class_3_4  13									//定义3 - 4班次的索引号
#define Class_3_5  14									//定义3 - 5班次的索引号
#define Class_3_6  15									//定义3 - 6班次的索引号
#define Class_3_7  16									//定义3 - 7班次的索引号

//定义3(10)班次的索引号
#define Class_3_10  7									

//定义夜1班次的索引号
#define Class_ye_1  28	

//定义夜2班次的索引号
#define Class_ye_2  29	

//定义帮夜的索引号
#define Class_bang_ye  27


#define Class_A  21										//定义A班次的索引号
#define Class_B  22										//定义A班次的索引号
#define Class_C  23										//定义A班次的索引号
#define Class_D  24										//定义A班次的索引号
#define Class_T  25										//定义T班次的索引号

#define Class_4_4  20									//定义4 - 4班次的索引号
#define Class_Z  26										//定义Z班次的索引号

//定义调休的班次号
#define Class_ChangeRelax  31	


//二、	3 - 4，3 - 5，3 - 6，3 - 7 四个班次每个人一轮不能上超过两个，其中3 - 6不能进行调休
//三、	3 - 1，3(10)， 3 - 3一天两人，3 一天1 - 3人
//四、	3 - 6，夜1，夜2，3(10) 不能进行调休
//五、	3 - 4，3 - 5， 3 - 7 调休之后需要还班
//六、	3(10) 上一休一，一轮14天，4个人，一天两人
//七、	夜1，夜2 周一到周日连上七天，连上之后周一必须休息，
//八、	不能连续上五天班
//九、	帮夜周一至周六，帮夜期间中间必须休息一天，不进行调休，一轮休5天，其他人一轮休4天
//十、	排班规格为班不动，人员进行滚动
//十一、	调休需要提前申请
//十二、	双休日原则上不调休

#define nurse_level 5									//定义护士等级1(1-4级)	



#define RelaxClassName _T("调休")		//调休班次名称
#define ReleaseClassName _T("休")		//休息班次名称
#define RepairRlexClassName _T("补休")		//休息班次名称
#define HoliDayRlexClassName _T("节假日调休")		//休息班次名称

#define DeleteClassName _T("删除班次")		//删除班次名称

#define RelaxClassName_MODIFY _T("调休(调整)")		//调休班次名称
#define RESTClassName _T("补休")		//调休班次名称
#define RESTClassName_MODIFY _T("补休(调整)")		//调休班次名称

#define MODIFYName  _T("(调整)")


//普通1班个数
#define NormalClasss1Num  8
////普通2班个数
//#define NormalClasss2Num  2
////普通3班个数
//#define NormalClasss3Num  1








////////////////////////////////////////////////////////////如上是班次的信息定义///////////////////////////////////////////////////////

//调休类型
enum RelaxEnumType
{
	NO_Relax = 0,//无调休
	Relax_INIT = 1,//初始的调休
	Relax_TO_WORK = 2,//初始的调休变成工作
	WORK_TO_Relax = 3,//初始工作变成调休
	REST_INIT = 4,//初始的补休
	WORK_TO_REST_INIT = 5,//工作变成补休

	BU_HOLIDAY_1 = 6,//补休1
	BU_HOLIDAY_2 = 7,//补休2
	BU_HOLIDAY_3 = 8,//补休3
	BU_HOLIDAY_4 = 9,//补休4
	BU_HOLIDAY_5 = 10,//补休5
	BU_HOLIDAY_6 = 11,//补休6
	BU_HOLIDAY_7 = 12,//补休7
	BU_HOLIDAY_8 = 13,//补休8
	BU_HOLIDAY_9 = 14,//补休9
	BU_HOLIDAY_10 = 15,//补休10


	BU_HOLIDAY_1_WORK_TO_Relax = 16,//补休1
	BU_HOLIDAY_2_WORK_TO_Relax,//补休2
	BU_HOLIDAY_3_WORK_TO_Relax,//补休3
	BU_HOLIDAY_4_WORK_TO_Relax,//补休4
	BU_HOLIDAY_5_WORK_TO_Relax,//补休5
	BU_HOLIDAY_6_WORK_TO_Relax,//补休6
	BU_HOLIDAY_7_WORK_TO_Relax,//补休7
	BU_HOLIDAY_8_WORK_TO_Relax,//补休8
	BU_HOLIDAY_9_WORK_TO_Relax,//补休9
	BU_HOLIDAY_10_WORK_TO_Relax,//补休10
};


//当天休息类型
enum RestEnumType
{
	NO_REST,//不休息
	Request_REST,//申请休息不补班
	Request_REST_AND_WORK,//申请休息且补班
	HasRest_NoAdjust,//已经休息了，不想被调
};

//班次类型
enum ClassType
{
	//早班
	Moning = 0,	//早班
	//中班
	Noon,		//中班
	//晚班
	Night,		//晚班
};

enum ClassCheckErroType
{
	HasNO_ERROR = 0,	//没有错误
	ConitWork_5_Day = 1,	//连续工作5天
	Moning_And_Night,	//早班和晚班不能连续
	SameClass_3_4,	//3 - 4，3 - 5，3 - 6，3 - 7 四个班次每个人一轮不能上超过两个3_4,	//3 - 4，3 - 5，3 - 6，3 - 7 四个班次每个人一轮不能上超过两个
	SameClass_3_5,
	SameClass_3_6,
	SameClass_3_7,
	SameClass_3_4567,
	Class_NotMatch,	//班次不匹配
	SameClass_AT,
	BangYe_Role,
};

struct RequestRestStruct
{
	RestEnumType restType;	//休息类型
	bool bHasProcessed;	//是否已经处理过
	RequestRestStruct()
	{
		restType = NO_REST;
		bHasProcessed = false;
	};
};


struct PersonRestInfo
{
	wstring strPersonName;		//姓名
	int level;			//级别

	//上轮调休这轮补休
	int LastCale_ThisRoundNeedWorkDay; //上轮结余本轮需要补班天数
	int LastCale_ThisRoundNeedRestDay; //上轮结余本轮需要调休天数

	int ThisRequest_ThisRoundNeedRestDay; //本轮申请需要休息的天数
	RequestRestStruct NeedRestDay[AllDay]; //本轮每天是否指定必须休息，0表示不需要休息，1表示申请休息不补班，2：表示申请休息且补班，3：已经休息了，不想被调
	int ThisRequest_ThisRoundNeedWordDay; //本轮申请需要加班天数
	int ThisRequest_NextRoundNeedWordDay; //下轮调休天数
	PersonRestInfo()
	{
		strPersonName = L"";
		level = 0;
		LastCale_ThisRoundNeedWorkDay = 0;
		LastCale_ThisRoundNeedRestDay = 0;
		ThisRequest_ThisRoundNeedRestDay = 0;
		ThisRequest_ThisRoundNeedRestDay = 0;
		ThisRequest_NextRoundNeedWordDay = 0;
		for (int k = 0; k < AllDay; k++)
		{
			NeedRestDay[k].restType = NO_REST;
			NeedRestDay[k].bHasProcessed = false;
		}
	};
	
};

#define RestOrWorkTye1 _T("上轮调休这轮补班")
#define RestOrWorkTye2 _T("上轮加班这轮补休")
#define RestOrWorkTye3 _T("这轮调休这轮补班")
#define RestOrWorkTye4 _T("这轮调休下轮补班")
#define RestOrWorkTye5 _T("这轮补班下轮调休")



struct AdjustRestAndWorkPerson
{
	int iPersonIndex; //人员索引
	int iDayIndex; //调休日期索引
	int iClass_OneDay;//
	RestEnumType iRestType;
	AdjustRestAndWorkPerson()
	{
		iPersonIndex = -1;
		iDayIndex = -1;
		iClass_OneDay = 0;
		iRestType = Request_REST_AND_WORK;
	}
};

struct PersonBasicInfo
{
	wstring strPersonName;	//姓名
	bool bIsFreeze;	//是否冻结
	bool bShiftLocked;//是否锁定
	PersonBasicInfo()
	{
		bIsFreeze = false;
		bShiftLocked = false;
	}
};



struct PersonRestDay
{
	int iAllRestDay;
	int iHasUsedDay;
	PersonRestDay()
	{
		iAllRestDay = 0;
		iHasUsedDay = 0;
	}
};


enum MyCellType
{
	Text_CanNotEdit,//文本不允许修改
	Text_CanEdit,//文本允许修改
	Combox,//下拉框
};
struct GridOneCellData
{
	//单元格类型
	MyCellType theCellType;
	//单元格颜色
	COLORREF clr_Cell;
	//单元格内文本颜色
	COLORREF clr_text;
	CString strData;
	//下拉选项
	vector<CString> ComboxDataArry;
	GridOneCellData()
	{
		clr_Cell = RGB(255, 255, 0);
		clr_text = RGB(255, 255, 255);
	}
};

struct ReturnClassStruct
{
	int iDayIndex_Source;
	int iDayIndex_Dst;

	int iPerson_Source;//被调休的人员
	int iPerson_Dst;//换班，还班人员
	int iPerson_Rest;//调休人员

	int iClass_OneDay_SrcPerson_SrcClass;
	int iClass_OneDay_DstPerson_SrcClass;


	int iClass_OneDay_SrcPerson_DstClass;
	int iClass_OneDay_DstPerson_DstClass;

	int iLevel_Plan;
	ReturnClassStruct()
	{
		iDayIndex_Source = -1;
		iDayIndex_Dst = -1;

		iPerson_Source = -1;//被调休的人员
		iPerson_Dst = -1;//换班，还班人员
		iPerson_Rest = -1;//调休人员

		iClass_OneDay_SrcPerson_SrcClass = -1;
		iClass_OneDay_DstPerson_SrcClass = -1;


		iClass_OneDay_SrcPerson_DstClass = -1;
		iClass_OneDay_DstPerson_DstClass = -1;

		iLevel_Plan = 0;
	}
};


struct ReturnClassPersonAndDay
{
	int iPersonIndex;
	int iDayIndex;
	ReturnClassPersonAndDay()
	{
		iPersonIndex = -1;
		iDayIndex = -1;
	}
};

enum ModifyClassSettingType
{
	NoSaveToNextTemplate = 0, //不保存到下次模板
	SaveToNextTemplate = 1, //保存到下次模板
	UserDefineTemplate = 2 //用户自定义模板
};



//链式联动换班的方案类型（4人及以上，也复用于新查找出的3人链）
#define ChainPlanType 4

//链式联动查找的默认上限
#define ChainMaxPersonNum_Default	4		//一条链最多几个人(含申请调休的人)，想放长改这里即可
#define ChainMaxPlanNum_Default		60		//最多产出多少条链式方案
#define ChainMaxCheckNum_Default	20000	//最多做多少次规则校验(防组合爆炸)

struct RestAdjustPlan {
	int iPlanType;         // 换班类型：2代表两人直换，3代表三人换班，4代表N人链式联动
	int iPersonIndex_B;    // 参与换班的调休人员B（最终接手工作的人）
	int iPersonIndex_C;    // 参与换班的人员C（仅三人轮换时有效，接手A的班，自己的班给B）
	CString strPlanDesc;   // 方案的文字描述（方便直接显示在UI列表里）
	int iPerson_Source;
	int iDayIndex_Source;

	//int iClass_OneDay_SrcPerson_SrcClass;
	CString strClassNameSrc;
	CString strClassName_B;
	CString strClassName_C;

	// === 新增字段：用于标注该方案是否符合规则及具体违规描述 ===
	bool bIsRuleValid;
	CString strRuleErrorDesc;

	// 链条末端接班的人是不是"万能格"(用了删除班次留下的空白格)。
	// 万能格本来就没有班、也没有调休名额，接班后正常上班即可，不能标成"调休转上班"。
	bool bEndIsUniversalCell;

	// ===== N人链式联动换班（iPlanType == ChainPlanType 时有效）=====
	// vecChainPersons[0]        ：申请调休的人 A（换班后当天变为调休）
	// vecChainPersons[i]        ：接手 vecChainPersons[i-1] 原班次的人
	// vecChainPersons[size - 1] ：当天原本调休/补休、最终顶班上岗的人
	// vecChainClassIdx[i]       ：vecChainPersons[i] 换班前的原班次索引，最后一人为 -1（休息）
	vector<int> vecChainPersons;
	vector<int> vecChainClassIdx;
	vector<CString> vecChainClassName;

	RestAdjustPlan()
	{
		iPlanType = 0;
		iPersonIndex_B = -1;
		iPersonIndex_C = -1;
		iDayIndex_Source = -1;
		iPerson_Source = -1;
		//iClass_OneDay_SrcPerson_SrcClass = -1;
		bIsRuleValid = false;
		bEndIsUniversalCell = false;
	}

};

//链式联动换班查找过程中的上下文
struct ChainSearchCtx
{
	int iDayIndex;					//在哪一天换班
	int iMaxChainPersons;			//一条链最多几个人(含申请人)
	int iMaxPlanNum;				//最多产出多少条方案
	int iCheckedNum;				//已经做过的规则校验次数
	int iMaxCheckNum;				//规则校验次数上限

	vector<int> vecPath;			//链上的人员索引，[0]为申请调休的人
	vector<int> vecPathClass;		//vecPath[i] 换班前的原班次索引
	vector<CString> vecPathErroDes;	//vecPath[i] 这一步违反了什么规则，空字符串表示这一步合规

	bool bAllowRuleErro;			//true=违规也照样产出方案，只把原因记下来交给用户判断

	vector<int> vecUniversalPersons;	//当天是万能格(删除班次留下的空白格)的人，接班的最优先人选
	vector<int> vecRestPersons;		//当天处于调休/补休状态、可以被拉出来顶班的人
	vector<int> vecWorkPersons;		//当天正常上班、且班次允许被换走的人
	vector<int> vecPersonLevel;		//每个人的级别(下标为人员索引，避免内层循环反复查名字)

	vector<RestAdjustPlan>* pOutPlans;	//查找结果

	ChainSearchCtx()
	{
		iDayIndex = -1;
		iMaxChainPersons = ChainMaxPersonNum_Default;
		iMaxPlanNum = ChainMaxPlanNum_Default;
		iCheckedNum = 0;
		iMaxCheckNum = ChainMaxCheckNum_Default;
		pOutPlans = nullptr;
		bAllowRuleErro = false;
	}
};





#define ConitWork_5_Day_ErroDes _T("连续工作超过或等于5天")
#define Moning_And_Night_ErroDes _T("早班和晚班不能连续")
#define SameClass_3_4_ErroDes _T("3 - 4 班次每个人一轮不能上超过两个")
#define SameClass_3_5_ErroDes _T("3 - 5 班次每个人一轮不能上超过两个")
#define SameClass_3_6_ErroDes _T("3 - 6 班次每个人一轮不能上超过两个")
#define SameClass_3_7_ErroDes _T("3 - 7 班次每个人一轮不能上超过两个")
#define SameClass_3_4567_ErroDes _T("3 - 4 3 - 5 3 - 6 3 - 7班次每个人一轮不能上超过两个")
#define Class_NotMatch_ErroDes _T("上不了该班次")
#define SameClass_AT_ErroDes _T("A T 班次每个人一轮不能上超过3个")
#define BangYe_Role_ErroDes _T("周日不能上帮夜")

//颜色定义


//背景色默认颜色-淡黄色
#define G_Bk_Defalut_COLORREF		    RGB(0xFF, 0xFF, 0xFF) //RGB(0xFF, 0xFF, 0xE0)

//背景色可调整颜色-白色
//#define G_Bk_CanModify_COLORREF		   RGB(255, 255, 255)       //‌RGB(255, 255, 255)‌ //RGB(0x90, 0xd7, 0xec)//RGB(177, 227, 228)	//90d7ec


//文字默认颜色—黑色
#define G_Text_Defalut_COLORREF		    RGB(0,0,0)	

//原始调休文字颜色-红色
#define G_Text_Relax_INIT_COLORREF		        RGB(255,0,0)	

//文字颜色-紫色
#define 	G_Text_Purple_COLORREF		        RGB(128,0,128)


//从工作变为调休文字颜色-绿色
#define G_Text_WorkToRelax_COLORREF		RGB(0, 200, 0)//RGB(0x00, 0xFF, 0x00) //f47920

//背景色冻结颜色-灰色
#define G_Bk_Freeze_COLORREF		    RGB(128, 128, 128)  //(128, 128, 128)


//背景色调休颜色-淡紫色
//#define G_Bk_Release_COLORREF           ‌RGB(255, 255, 255) 

//RGB(0xaf, 0xb4, 0xdb) //
#define G_Bk_Release_COLORREF  RGB(255,255,255)

//万能格背景色-淡绿色(删除班次后留下的空白格，可以接任何本级别能上的班)
#define G_Bk_UniversalCell_COLORREF		    RGB(198, 239, 206)

//还班颜色-橙色
#define G_Bk_ReturnClass_COLORREF		    RGB(250,132,43) //

#define G_Bk_ReturnClass_COLORREF_MODIFY		    RGB(176, 224, 230)//RGB(250,132,43) //



//文字颜色-紫色
#define G_Text_Purple_COLORREF		        RGB(255,0,255)	

//文字颜色-深蓝色
#define G_Text_Blue_COLORREF		        RGB(0, 0, 128)	


#endif // !defined(AFX_SENDVALUETOHDBTHREAD_H__F4930EB4_9976_4A71_BF02_E5F377D40744__INCLUDED_)

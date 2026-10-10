// TakeLongHoliday.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "TakeLongHoliday.h"
#include "SchedulCommonStruct.h"
#include "PIVAS_ScheduleDlg.h"
extern CPIVASScheduleDlg* g_pMainDlg;

//人员列表
extern vector<PersonBasicInfo> g_Level1_PersonList;
extern vector<PersonBasicInfo> g_Level2_PersonList;
extern vector<PersonBasicInfo> g_Level3_PersonList;
extern vector<PersonBasicInfo> g_Level4_PersonList;

// TakeLongHoliday 对话框

IMPLEMENT_DYNAMIC(TakeLongHoliday, CDialogEx)

TakeLongHoliday::TakeLongHoliday(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TakeLongHoliday, pParent)
{

}

TakeLongHoliday::~TakeLongHoliday()
{
}

void TakeLongHoliday::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_ComboBox_AllPerson);
}


BEGIN_MESSAGE_MAP(TakeLongHoliday, CDialogEx)
	ON_BN_CLICKED(IDOK, &TakeLongHoliday::OnBnClickedOk)
END_MESSAGE_MAP()


// TakeLongHoliday 消息处理程序

void TakeLongHoliday::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData(TRUE);
	CTime cdate_start;

	((CDateTimeCtrl*)GetDlgItem(IDC_DATETIMEPICKER1))->GetTime(cdate_start);    //得到控件
	m_starttime = cdate_start.GetTime();

	CTime cdate_end;
	((CDateTimeCtrl*)GetDlgItem(IDC_DATETIMEPICKER2))->GetTime(cdate_end);    //得到控件
	m_endtime = cdate_end.GetTime();

	if (m_starttime >= m_endtime)
	{
		MessageBox(_T("起始时间不能大于或等于结束时间"), _T("提示"), MB_OK | MB_ICONINFORMATION);
		return;
	}

	int index = m_ComboBox_AllPerson.GetCurSel(); // 获取当前选中项的索引
	CString strPersonName;
	if (index != CB_ERR) { // 确保有选中项
		m_ComboBox_AllPerson.GetLBText(index, strPersonName); // 获取当前选中项的文本
	}


	//开始校验请假的班次是否能有人能上
	g_pMainDlg->CheckPersonRestCanModifyOk(strPersonName, m_starttime, m_endtime);

	CDialogEx::OnOK();
}

BOOL TakeLongHoliday::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化


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
	m_ComboBox_AllPerson.SetCurSel(0);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

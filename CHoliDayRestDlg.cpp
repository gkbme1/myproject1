// CHoliDayRestDlg.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CHoliDayRestDlg.h"
#include <map>
using namespace std;

extern std::map<CString, int> g_ModfifyResttMap;


// CHoliDayRestDlg 对话框

IMPLEMENT_DYNAMIC(CHoliDayRestDlg, CDialogEx)

CHoliDayRestDlg::CHoliDayRestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHoliDayRestDlg, pParent)
{
	m_bAllowInput = false;
}

CHoliDayRestDlg::~CHoliDayRestDlg()
{
}

void CHoliDayRestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_HoliDayCombox);
}


BEGIN_MESSAGE_MAP(CHoliDayRestDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CHoliDayRestDlg::OnBnClickedOk)
END_MESSAGE_MAP()


//切换到"填写"用法：下拉里换成调用方给的选项，并且允许手工输入
void CHoliDayRestDlg::SetFillMode(CString strTip, vector<CString>& vecOptions)
{
	m_strFillTip = strTip;
	m_vecFillOptions = vecOptions;
	m_bAllowInput = true;
}

//资源里的下拉框如果是"只能选不能打字"的 CBS_DROPDOWNLIST，就地换成可输入的 CBS_DROPDOWN。
//下拉框的样式建好之后改不了，只能原地重建一个；万一重建失败就保持原样，
//客户仍然可以从列表里选，只是打不了字，不影响其它功能。
void CHoliDayRestDlg::TryMakeComboCanInput()
{
	if (m_HoliDayCombox.GetSafeHwnd() == NULL)
	{
		return;
	}

	DWORD dwStyle = m_HoliDayCombox.GetStyle();
	if ((dwStyle & 0x0003) == CBS_DROPDOWN)
	{
		return;		//本来就能输入
	}

	//记下原来的位置和字体
	CRect rcCombo;
	m_HoliDayCombox.GetWindowRect(&rcCombo);
	ScreenToClient(&rcCombo);
	CFont* pFont = m_HoliDayCombox.GetFont();

	//下拉列表展开的高度也由这个矩形决定，给足 200 像素
	CRect rcNew = rcCombo;
	rcNew.bottom = rcNew.top + 200;

	m_HoliDayCombox.DestroyWindow();

	BOOL bOk = m_HoliDayCombox.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL
		| CBS_DROPDOWN | CBS_AUTOHSCROLL, rcNew, this, IDC_COMBO1);
	if (!bOk)
	{
		return;
	}

	if (pFont != NULL)
	{
		m_HoliDayCombox.SetFont(pFont);
	}
	m_HoliDayCombox.SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}


// CHoliDayRestDlg 消息处理程序

BOOL CHoliDayRestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	if (m_bAllowInput)
	{
		//【填写用法】下拉里放调用方给的选项，并且允许手工输入
		TryMakeComboCanInput();

		for (int k = 0; k < (int)m_vecFillOptions.size(); k++)
		{
			if (m_HoliDayCombox.FindStringExact(-1, m_vecFillOptions[k]) == CB_ERR)
			{
				m_HoliDayCombox.AddString(m_vecFillOptions[k]);
			}
		}

		if (!m_strFillTip.IsEmpty())
		{
			SetWindowText(m_strFillTip);
		}

		if (m_HoliDayCombox.GetCount() > 0)
		{
			m_HoliDayCombox.SetCurSel(0);
		}
	}
	else
	{
		//【节假日调休用法】保持原样：把 g_ModfifyResttMap 里的内容放到 ComboBox 中
		for (map<CString, int>::iterator it = g_ModfifyResttMap.begin(); it != g_ModfifyResttMap.end(); it++)
		{
			m_HoliDayCombox.AddString(it->first);
		}
	}



	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CHoliDayRestDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	m_strHoliDayName = _T("");

	//手工输入的内容只能用 GetWindowText 取；只能选的下拉框用它取到的也是选中项的文字，
	//所以两种用法都先走它，取不到再退回按选中项取。
	m_HoliDayCombox.GetWindowText(m_strHoliDayName);
	m_strHoliDayName.Trim();

	if (m_strHoliDayName.IsEmpty())
	{
		int index = m_HoliDayCombox.GetCurSel(); // 获取当前选中项的索引
		if (index != CB_ERR) { // 确保有选中项
			m_HoliDayCombox.GetLBText(index, m_strHoliDayName); // 获取当前选中项的文本
		}
	}

	CDialogEx::OnOK();
}

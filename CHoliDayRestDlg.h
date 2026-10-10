#pragma once
#include "afxdialogex.h"
#include <vector>
using namespace std;


// CHoliDayRestDlg 对话框
// 两种用法：
//   1) 默认用法(节假日调休)：直接 DoModal，下拉里是"补元旦补清明.txt"里的节假日调休名称，只能从列表里选；
//   2) 填写用法：先调 SetFillMode(提示文字, 选项列表)，下拉里换成调用方给的选项，
//      并且允许客户直接手工输入列表里没有的文字（调休安排成功之后填自定义班次就走这条路）。

class CHoliDayRestDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHoliDayRestDlg)

public:
	CHoliDayRestDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CHoliDayRestDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHoliDayRestDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_HoliDayCombox;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	CString m_strHoliDayName;

	// 切换到"填写"用法：strTip 是窗口标题上的提示，vecOptions 是下拉里的选项，允许手工输入
	void SetFillMode(CString strTip, vector<CString>& vecOptions);

	// 下面两个成员为空/false 时就是原来的节假日调休用法，老代码不受影响
	vector<CString> m_vecFillOptions;
	CString m_strFillTip;
	bool m_bAllowInput;

protected:
	// 资源里的下拉框可能是"只能选、不能打字"的 CBS_DROPDOWNLIST，
	// 填写用法下就地把它换成可输入的 CBS_DROPDOWN；换不成也不影响使用(退化成只能选)
	void TryMakeComboCanInput();
};

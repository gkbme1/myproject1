// CActiveSoftwareDlg.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CActiveSoftwareDlg.h"

extern bool g_bHasAuth;

// CActiveSoftwareDlg 对话框

IMPLEMENT_DYNAMIC(CActiveSoftwareDlg, CDialogEx)

CActiveSoftwareDlg::CActiveSoftwareDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CActiveSoftwareDlg, pParent)
{
	m_pAuthorizeDLL = nullptr;
}

CActiveSoftwareDlg::~CActiveSoftwareDlg()
{
}

void CActiveSoftwareDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CActiveSoftwareDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CActiveSoftwareDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CActiveSoftwareDlg 消息处理程序

BOOL CActiveSoftwareDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	//将机器码显示到界面上
	SetDlgItemTextW(IDC_EDIT_MACHINECODE, strMachineCode);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CActiveSoftwareDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//CDialogEx::OnOK();
	if (m_pAuthorizeDLL)
	{
		CString strKey;
		GetDlgItemTextW(IDC_EDIT_AUTHCODE, strKey);
		std::string strKeyStd = CT2A(strKey);
		int iRet = m_pAuthorizeDLL->ValidateAuth((char*)strKeyStd.c_str());
		if (iRet == 0)
		{
			g_bHasAuth = true;
			AfxMessageBox(_T("激活成功！"));
			CDialogEx::OnOK();
		}
		else if (iRet == 1)
		{
			AfxMessageBox(_T("激活码无效，请重新输入！"));
		}
		else if (iRet == 2)
		{
			AfxMessageBox(_T("激活失败，注册表读写权限受限，请联系管理员！"));
		}
		else
		{
			AfxMessageBox(_T("激活失败，未知错误！"));
		}
	}
	else
	{
		AfxMessageBox(_T("激活失败，授权对象未创建！"));
	}
}

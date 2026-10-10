// CModifyPersonCanWorkClass.cpp: 实现文件
//

#include "pch.h"
#include "PIVAS_Schedule.h"
#include "afxdialogex.h"
#include "CModifyPersonCanWorkClass.h"

#include "SchedulCommonStruct.h"
extern int NurseCanWorkClassesLevel[nurse_level][OneDayClasssNum];
extern string g_filePath;
// CModifyPersonCanWorkClass 对话框

IMPLEMENT_DYNAMIC(CModifyPersonCanWorkClass, CDialogEx)

CModifyPersonCanWorkClass::CModifyPersonCanWorkClass(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CModifyPersonCanWorkClass, pParent)
{

}

CModifyPersonCanWorkClass::~CModifyPersonCanWorkClass()
{
}

void CModifyPersonCanWorkClass::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CModifyPersonCanWorkClass, CDialogEx)
	ON_BN_CLICKED(IDOK, &CModifyPersonCanWorkClass::OnBnClickedOk)
END_MESSAGE_MAP()


// CModifyPersonCanWorkClass 消息处理程序

BOOL CModifyPersonCanWorkClass::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	for (int iLevel = 0; iLevel < 4; iLevel++)
	{

		//int iClass = 19;
		//循环上限必须是 CanWorkClassCheckNum(30)，不能用 OneDayClasssNum-1(31)：
		//多出来的那一次会取到下一个级别的第一个复选框，详见 CModifyPersonCanWorkClass.h 的说明。
		for (int iClass = 0; iClass < CanWorkClassCheckNum; iClass++)
		{
			int iCheckButtonID = 0;
			switch (iLevel)
			{
				case 0:
					iCheckButtonID = IDC_CHECK_CLASS1 + iClass;
					break;
				case 1:
					iCheckButtonID = IDC_CHECK_CLASS31 + iClass;
					break;
				case 2:
					iCheckButtonID = IDC_CHECK_CLASS61 + iClass;
					break;
				case 3:
					iCheckButtonID = IDC_CHECK_CLASS91 + iClass;
					break;
				default:
					break;
			}

			
			CButton* pCheckButton = (CButton*)GetDlgItem(iCheckButtonID);
			if (NurseCanWorkClassesLevel[iLevel][iClass] > 0)
			{
				//将IDC_CHECK_CLASS1设置为选中

				if (pCheckButton != nullptr)
				{
					pCheckButton->SetCheck(BST_CHECKED);
				}
			}
			else
			{
				//将IDC_CHECK_CLASS1设置为未选中
				if (pCheckButton != nullptr)
				{
					pCheckButton->SetCheck(BST_UNCHECKED);
				}
			}

		}
	}




	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CModifyPersonCanWorkClass::OnBnClickedOk()
{
	// 1. 构建保存的 CSV 文件路径
	std::string filename = g_filePath + "\\PersonCanWorkClass.csv"; // 你可以自定义文件名
	FILE* fp = fopen(filename.c_str(), "w");
	if (fp == NULL)
	{
		AfxMessageBox(_T("无法保存班次配置文件，请检查目录权限！"));
		CDialogEx::OnOK();
		return;
	}

	// 用于拼接 CSV 内容的缓冲区
	char chData[4096] = { 0 };

	// 2. 遍历 4 个级别，每个级别 CanWorkClassCheckNum(30) 个班次。
	//    这里的列数必须和 CPIVASScheduleDlg::ReadPersonCanWorkClassFromFile 读文件时一致，
	//    原来写31列、读30列，最后一列永远读不回来。
	for (int iLevel = 0; iLevel < 4; iLevel++)
	{
		for (int iClass = 0; iClass < CanWorkClassCheckNum; iClass++)
		{
			// 计算当前控件的 ID
			int iCheckButtonID = 0;
			switch (iLevel)
			{
			case 0: iCheckButtonID = IDC_CHECK_CLASS1 + iClass; break;
			case 1: iCheckButtonID = IDC_CHECK_CLASS31 + iClass; break;
			case 2: iCheckButtonID = IDC_CHECK_CLASS61 + iClass; break;
			case 3: iCheckButtonID = IDC_CHECK_CLASS91 + iClass; break;
			}

			// 获取 CheckBox 状态
			CButton* pCheckButton = (CButton*)GetDlgItem(iCheckButtonID);
			int canWork = 0;

			if (pCheckButton != nullptr && pCheckButton->GetCheck() == BST_CHECKED)
			{
				canWork = 1;
				// 同步更新内存中的全局数组 (10代表该班次可上/产出标准值)
				NurseCanWorkClassesLevel[iLevel][iClass] = 10;
			}
			else
			{
				canWork = 0;
				// 同步更新内存中的全局数组 (0代表该班次不可上)
				NurseCanWorkClassesLevel[iLevel][iClass] = 0;
			}

			// 3. 格式化拼接数据
			char temp[16] = { 0 };
			if (iClass == CanWorkClassCheckNum - 1)
			{
				// 最后一个班次不加逗号，直接换行
				sprintf(temp, "%d\n", canWork);
			}
			else
			{
				// 否则以逗号分隔
				sprintf(temp, "%d,", canWork);
			}
			strcat(chData, temp);
		}
	}

	// 4. 将拼接好的字符串写入 CSV 文件
	fseek(fp, 0, SEEK_SET);
	fwrite(chData, strlen(chData), 1, fp);
	fclose(fp);

	CDialogEx::OnOK();
}

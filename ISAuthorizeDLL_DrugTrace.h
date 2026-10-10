// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 REGISTERAUTH_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// ISAUTHORIZEH_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。


#if !defined(AFX_REGISTERAUTH_H__D1F7D2B5_2AA7_4D01_9664_A556D1604030__INCLUDED_)
#define AFX_REGISTERAUTH_H__D1F7D2B5_2AA7_4D01_9664_A556D1604030__INCLUDED_


#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#ifdef ISAUTHORIZEH_DRUGTRACE_API
#define ISAUTHORIZEH_DRUGTRACE_API __declspec(dllexport)
#else
#define ISAUTHORIZEH_DRUGTRACE_API __declspec(dllimport)
#endif

typedef struct  _AuthorityState_DRUGTRACE
{
public:
	_AuthorityState_DRUGTRACE()
	{
		tmProduceTime = (struct tm*)malloc(sizeof(struct tm));
		tmUserRegDate = (struct tm*)malloc(sizeof(struct tm));
	}
	~_AuthorityState_DRUGTRACE()
	{
		if(tmProduceTime)
		{
			free(tmProduceTime);
			tmProduceTime = NULL;
		}
		if(tmUserRegDate)
		{
			free(tmUserRegDate);
			tmUserRegDate = NULL;
		}
	}

	struct tm*		tmProduceTime;					//授权码产生时间
	struct tm*		tmUserRegDate;					//授权码激活时间
	WORD			wDayLen;						//授权期限
	BOOL			bElectronicsState;				//电子授权状态
	DWORD			dwAccumulatedUseTime;			//累计时间
}AuthorityState_DRUGTRACE;

// 此类是从 ISAuthorize.dll 导出的
class ISAUTHORIZEH_DRUGTRACE_API CISAuthorizeDLL_DRUGTRACE {
public:
	//版本信息9位V00.00.00
	CISAuthorizeDLL_DRUGTRACE(char *chProIdentity, char* chVersion, BOOL bFlag = TRUE, BOOL bTimer = FALSE);
	~CISAuthorizeDLL_DRUGTRACE();
	// TODO: 在此添加您的方法。

	//获取机器码
	const char* GetMachineCodeSZ();
	std::string GetMachineCode();
	//验证授权码是否正确
	//返回值：-1 初始化，0 成功，1 过期，2 失效，3 写注册表失败，4 机器码不合法， 5 未到激活时间, 6权限不足
	int ValidateAuth( char* chKey  );

	//查询授权状态，返回值,-1标示初始化，0标示成功，1标示使用到期，2累计时间到期,3去读注册表失败, 6权限不足
	int ReadAuthorityState(AuthorityState_DRUGTRACE** pAuthorityState);

	std::string	GetManufacturer();	//获取厂商ID
	const char* GetVendorId();      //获取厂商ID
	
	//获取当前激活授权的授权码、授权码产生时间、授权码激活时间、授权期限
	bool GetCurrentAuthorityCodeAndTime(char *AuthorityCode, int AuthorityCodeBufferLength, tm *tmProduceTime, tm*	tmUserRegDate, WORD	&wDayLen);
	//校验传入机器码和授权码是否合法
	bool CheckMachineCodeIsMatchAuthorityCode(char* chMachineCode, char* chAuthorityCode);

private:

};

#endif //AFX_REGISTERAUTH_H__D1F7D2B5_2AA7_4D01_9664_A556D1604030__INCLUDED_

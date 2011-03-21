// QuerierDlg.h : 头文件
//

#pragma once
#include "deelx.h"

// CQuerierDlg 对话框
class CQuerierDlg : public CDialog
{
// 构造
public:
	CQuerierDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_QUERIER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	static std::string UTF8ToAnsi( const std::string& strIn, std::string& strOut );
	static std::string AnsiToUTF8( const std::string& strIn, std::string& strOut );
	static int Test_Identity(const char* str);

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSelect();	
	
	afx_msg void OnBnClickedButtonExit();
};

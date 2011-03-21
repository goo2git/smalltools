// QuerierDlg.h : ͷ�ļ�
//

#pragma once
#include "deelx.h"

// CQuerierDlg �Ի���
class CQuerierDlg : public CDialog
{
// ����
public:
	CQuerierDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_QUERIER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	static std::string UTF8ToAnsi( const std::string& strIn, std::string& strOut );
	static std::string AnsiToUTF8( const std::string& strIn, std::string& strOut );
	static int Test_Identity(const char* str);

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSelect();	
	
	afx_msg void OnBnClickedButtonExit();
};

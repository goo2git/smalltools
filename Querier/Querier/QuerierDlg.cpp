// QuerierDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "Querier.h"
#include "QuerierDlg.h"

extern "C"
{
#include "SQLite/include/sqlite3.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CQuerierDlg �Ի���




CQuerierDlg::CQuerierDlg(CWnd* pParent /*=NULL*/)
: CDialog(CQuerierDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CQuerierDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CQuerierDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_SELECT, &CQuerierDlg::OnBnClickedButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_EXIT, &CQuerierDlg::OnBnClickedButtonExit)
END_MESSAGE_MAP()


// CQuerierDlg ��Ϣ�������

BOOL CQuerierDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CQuerierDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CQuerierDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CQuerierDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

std::string CQuerierDlg::UTF8ToAnsi( const std::string& strIn, std::string& strOut )
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;

	int i = MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), -1, NULL, 0);

	strSrc = new WCHAR[i+1];
	MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), -1, strSrc, i);

	i = WideCharToMultiByte(CP_ACP, 0, strSrc, -1, NULL, 0, NULL, NULL);

	szRes = new TCHAR[i+1];
	WideCharToMultiByte(CP_ACP, 0, strSrc, -1, szRes, i, NULL, NULL);

	strOut = szRes;

	delete[] strSrc;
	delete[] szRes;

	return strOut;
}

std::string CQuerierDlg::AnsiToUTF8( const std::string& strIn, std::string& strOut )
{
	WCHAR* strSrc    = NULL;
	TCHAR* szRes    = NULL;

	int len = MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strIn.c_str(), -1, NULL,0);

	unsigned short* wszUtf8 = new unsigned short[len+1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)strIn.c_str(), -1, (LPWSTR)wszUtf8, len);

	len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL);

	char* szUtf8 = new char[len + 1];
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte (CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, szUtf8, len, NULL,NULL);

	strOut = szUtf8;

	delete[] szUtf8;
	delete[] wszUtf8;

	return strOut;
}

int CQuerierDlg::Test_Identity(const char* str)
{
	static CRegexpT<char> regexp("^(\\d{15}$|^\\d{18}$|^\\d{17}(\\d|x|X))$");
	MatchResult result = regexp.MatchExact(str);
	if (result.IsMatched() == 0)
	{
		return 0;
	}

	CString strID(str);
	CString strYear("");
	CString strMonth("");
	CString strDay("");
	int iYear;
	int iMonth;
	int iDay;
	if (strID.GetLength() == 15)
	{
		strYear = "19" + strID.Mid(6, 2);
		iYear = atoi(strYear.GetBuffer());		

		strMonth = strID.Mid(8, 2);
		iMonth = atoi(strMonth.GetBuffer());
		if (iMonth < 1 || iMonth > 12)
		{
			return 0;
		}

		CString strDay = strID.Mid(10, 2);
		iDay = atoi(strDay.GetBuffer());		
		switch(iMonth)
		{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			{
				if (iDay < 1 || iDay > 31)
				{
					return 0;
				}
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			{
				if (iDay < 1 || iDay > 30)
				{
					return 0;
				}
			}
			break;
		case 2:
			{
				if (iYear % 4 == 0 && iYear % 100 != 0 || iYear % 400 == 0)
				{
					if (iDay < 1 || iDay > 29)
					{
						return 0;
					}
				}
				else
				{
					if (iDay < 1 || iDay > 28)
					{
						return 0;
					}
				}

			}
		}		
	}
	else if (strID.GetLength() == 18)
	{
		strYear = strID.Mid(6, 4);
		iYear = atoi(strYear.GetBuffer());	

		strMonth = strID.Mid(10, 2);
		iMonth = atoi(strMonth.GetBuffer());
		if (iMonth < 1 || iMonth > 12)
		{
			return 0;
		}

		CString strDay = strID.Mid(12, 2);
		iDay = atoi(strDay.GetBuffer());
		switch(iMonth)
		{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			{
				if (iDay < 1 || iDay > 31)
				{
					return 0;
				}
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			{
				if (iDay < 1 || iDay > 30)
				{
					return 0;
				}
			}
			break;
		case 2:
			{
				if (iYear % 4 == 0 && iYear % 100 != 0 || iYear % 400 == 0)
				{
					if (iDay < 1 || iDay > 29)
					{
						return 0;
					}
				}
				else
				{
					if (iDay < 1 || iDay > 28)
					{
						return 0;
					}
				}
			}
		}		
	}

	return 1;
}

//���Ұ�ť
void CQuerierDlg::OnBnClickedButtonSelect()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	sqlite3* db = NULL;
	int result;
	char* errmsg = NULL;
	char** dbResult;
	int nRow,nColumn;

	CString strID;
	GetDlgItem(IDC_EDIT_ID)->GetWindowText(strID);
	int bMatching = Test_Identity(strID.GetBuffer());
	if (bMatching != 0)//������֤��ʽ��ȷ
	{
		result = sqlite3_open("data.db", &db);
		if (result != SQLITE_OK)
		{
			GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("�Բ������ݶ�ȡʧ�ܣ�");
			GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
			return;
		}

		CString strArea = strID.Left(6);//����		
		CString strBirth("");           //���������������� 
		CString strNumber("");          //����
		CString strCheckCode("");       //У����
		if (strID.GetLength() == 15)
		{
			strBirth = "19" + strID.Mid(6, 2) + "��" + strID.Mid(8, 2) + "��" + strID.Mid(10, 2) + "��";

			strNumber = strID.Right(3);
		}
		else if (strID.GetLength() == 18)
		{
			strBirth = strID.Mid(6, 4) + "��" + strID.Mid(10, 2) + "��" + strID.Mid(12, 2) + "��";

			strNumber = strID.Mid(14, 3);
			strCheckCode = strID.Right(1);
		}

		CString strSQL;
		strSQL.Format("SELECT * FROM Card WHERE Data='%s'", strArea.GetBuffer());
		result = sqlite3_get_table(db, strSQL.GetBuffer(), &dbResult, &nRow, &nColumn, &errmsg);
		if (SQLITE_OK == result)
		{
			if (nRow > 0)
			{			
				int index = nColumn;
				for (int i = 0; i < nRow; ++i)
				{
					string strSheng;
					string strShi;
					string strXian;
					string strPath;

					UTF8ToAnsi(dbResult[index + 1], strSheng);
					UTF8ToAnsi(dbResult[index + 2], strShi);
					UTF8ToAnsi(dbResult[index + 3], strXian);
					strPath = strSheng + strShi + strXian;

					GetDlgItem(IDC_STATIC_SHOW)->SetWindowText(strPath.c_str());//���õ���
					GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText(strBirth);//��������

					break;
				}
			}
			else
			{
				GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("û���ҵ������֤���룡");
				GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
			}
		}
		else
		{
			GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("�Բ������ݶ�ȡʧ�ܣ�");
			GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
		}

		sqlite3_free_table(dbResult);
		sqlite3_close(db);
	}
	else
	{
		GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("�Բ������ǹ��һ�û��������һ�����֤��");
		GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
	}
}

void CQuerierDlg::OnBnClickedButtonExit()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	this->DestroyWindow();
}

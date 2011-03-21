// QuerierDlg.cpp : 实现文件
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


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
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


// CQuerierDlg 对话框




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


// CQuerierDlg 消息处理程序

BOOL CQuerierDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CQuerierDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
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

//查找按钮
void CQuerierDlg::OnBnClickedButtonSelect()
{
	// TODO: 在此添加控件通知处理程序代码
	sqlite3* db = NULL;
	int result;
	char* errmsg = NULL;
	char** dbResult;
	int nRow,nColumn;

	CString strID;
	GetDlgItem(IDC_EDIT_ID)->GetWindowText(strID);
	int bMatching = Test_Identity(strID.GetBuffer());
	if (bMatching != 0)//如果身份证格式正确
	{
		result = sqlite3_open("data.db", &db);
		if (result != SQLITE_OK)
		{
			GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("对不起，数据读取失败！");
			GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
			return;
		}

		CString strArea = strID.Left(6);//区号		
		CString strBirth("");           //用来保存完整生日 
		CString strNumber("");          //号码
		CString strCheckCode("");       //校验码
		if (strID.GetLength() == 15)
		{
			strBirth = "19" + strID.Mid(6, 2) + "年" + strID.Mid(8, 2) + "月" + strID.Mid(10, 2) + "日";

			strNumber = strID.Right(3);
		}
		else if (strID.GetLength() == 18)
		{
			strBirth = strID.Mid(6, 4) + "年" + strID.Mid(10, 2) + "月" + strID.Mid(12, 2) + "日";

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

					GetDlgItem(IDC_STATIC_SHOW)->SetWindowText(strPath.c_str());//设置地区
					GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText(strBirth);//设置生日

					break;
				}
			}
			else
			{
				GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("没有找到该身份证号码！");
				GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
			}
		}
		else
		{
			GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("对不起，数据读取失败！");
			GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
		}

		sqlite3_free_table(dbResult);
		sqlite3_close(db);
	}
	else
	{
		GetDlgItem(IDC_STATIC_SHOW)->SetWindowText("对不起，咱们国家还没有人用这一号身份证！");
		GetDlgItem(IDC_STATIC_BIRTHDAY)->SetWindowText("");
	}
}

void CQuerierDlg::OnBnClickedButtonExit()
{
	// TODO: 在此添加控件通知处理程序代码

	this->DestroyWindow();
}

// ConvertResDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "ConvertResDlg.h"
#include ".\convertresdlg.h"

// CConvertResDlg dialog

CConvertResDlg::CConvertResDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CConvertResDlg::IDD, pParent)
	, m_name(_T(""))
	, m_desc(_T(""))
{
}

CConvertResDlg::~CConvertResDlg()
{
}

void CConvertResDlg::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT3, m_name);
	DDX_Text(pDX, IDC_COMBO1, m_mime);
	DDX_Control(pDX, IDC_COMBO1, m_mimectrl);
	DDX_Text(pDX, IDC_EDIT2, m_desc);
}

BOOL CConvertResDlg::OnInitDialog()
{
	__super::OnInitDialog();

	AddAnchor(IDC_EDIT3, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBO1, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDIT2, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_CENTER);
	AddAnchor(IDCANCEL, BOTTOM_CENTER);

	CRegKey key;
	CString str(_T("MIME\\Database\\Content Type"));
	if(ERROR_SUCCESS == key.Open(HKEY_CLASSES_ROOT, str, KEY_READ))
	{
		CAtlMap<CString, bool, CStringElementTraits<CString> > mimes;

		TCHAR buff[256];
		DWORD len = countof(buff);
		for(int i = 0; ERROR_SUCCESS == key.EnumKey(i, buff, &len); i++, len = countof(buff))
		{
			CRegKey mime;
			TCHAR ext[64];
			ULONG len = countof(ext);
			if(ERROR_SUCCESS == mime.Open(HKEY_CLASSES_ROOT, str + _T("\\") + buff, KEY_READ)
			&& ERROR_SUCCESS == mime.QueryStringValue(_T("Extension"), ext, &len))
			{
				CString mime = CString(buff).MakeLower();
				mimes[mime] = true;
				m_mimectrl.AddString(mime);
			}
		}

		if(!mimes.Lookup(_T("application/x-truetype-font")))
			m_mimectrl.AddString(_T("application/x-truetype-font"));
	}

	m_desc.Replace(_T("\n"), _T("\r\n"));

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConvertResDlg::OnOK()
{
	UpdateData();

	m_name.Trim();
	m_mime.Trim();
	m_desc.Replace(_T("\r\n"), _T("\r"));
	m_desc.Trim();

	__super::OnOK();
}

BEGIN_MESSAGE_MAP(CConvertResDlg, CResizableDialog)
END_MESSAGE_MAP()

// CConvertResDlg message handlers
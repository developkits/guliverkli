
// PPagePlayer.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include "PPagePlayer.h"
#include "MainFrm.h"

// CPPagePlayer dialog

IMPLEMENT_DYNAMIC(CPPagePlayer, CPPageBase)
CPPagePlayer::CPPagePlayer()
	: CPPageBase(CPPagePlayer::IDD, CPPagePlayer::IDD)
	, m_iAllowMultipleInst(0)
	, m_iAlwaysOnTop(FALSE)
	, m_fTrayIcon(FALSE)
	, m_iShowBarsWhenFullScreen(FALSE)
	, m_nShowBarsWhenFullScreenTimeOut(0)
	, m_iTitleBarTextStyle(0)
	, m_fExitFullScreenAtTheEnd(FALSE)
	, m_fRememberWindowPos(FALSE)
	, m_fRememberWindowSize(FALSE)
	, m_fUseIni(FALSE)
	, m_fSetFullscreenRes(FALSE)
{
}

CPPagePlayer::~CPPagePlayer()
{
}

void CPPagePlayer::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO1, m_iAllowMultipleInst);
	DDX_Radio(pDX, IDC_RADIO3, m_iTitleBarTextStyle);
	DDX_Check(pDX, IDC_CHECK2, m_iAlwaysOnTop);
	DDX_Check(pDX, IDC_CHECK3, m_fTrayIcon);
	DDX_Check(pDX, IDC_CHECK4, m_iShowBarsWhenFullScreen);
	DDX_Text(pDX, IDC_EDIT1, m_nShowBarsWhenFullScreenTimeOut);
	DDX_Check(pDX, IDC_CHECK5, m_fExitFullScreenAtTheEnd);
	DDX_Check(pDX, IDC_CHECK6, m_fRememberWindowPos);
	DDX_Check(pDX, IDC_CHECK7, m_fRememberWindowSize);
	DDX_Check(pDX, IDC_CHECK8, m_fUseIni);
	DDX_Control(pDX, IDC_SPIN1, m_nTimeOutCtrl);
	DDX_Check(pDX, IDC_CHECK9, m_fSetFullscreenRes);
	DDX_Control(pDX, IDC_COMBO1, m_dispmodecombo);
}


BEGIN_MESSAGE_MAP(CPPagePlayer, CPPageBase)
	ON_BN_CLICKED(IDC_CHECK8, OnBnClickedCheck8)
	ON_UPDATE_COMMAND_UI(IDC_SPIN1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_EDIT1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_STATIC1, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_STATIC2, OnUpdateTimeout)
	ON_UPDATE_COMMAND_UI(IDC_COMBO1, OnUpdateDispModeCombo)
END_MESSAGE_MAP()


// CPPagePlayer message handlers

BOOL CPPagePlayer::OnInitDialog()
{
	__super::OnInitDialog();

	AppSettings& s = AfxGetAppSettings();

	m_iAllowMultipleInst = s.fAllowMultipleInst;
	m_iTitleBarTextStyle = s.iTitleBarTextStyle;
	m_iAlwaysOnTop = s.fAlwaysOnTop;
	m_fTrayIcon = s.fTrayIcon;
	m_iShowBarsWhenFullScreen = s.fShowBarsWhenFullScreen;
	m_nShowBarsWhenFullScreenTimeOut = s.nShowBarsWhenFullScreenTimeOut;
	m_nTimeOutCtrl.SetRange(-1, 10);
	m_fSetFullscreenRes = s.dmFullscreenRes.fValid;
	int iSel = -1;
	dispmode dm, dmtoset = s.dmFullscreenRes;
	if(!dmtoset.fValid) GetCurDispMode(dmtoset);
	for(int i = 0, j = 0; GetDispMode(i, dm); i++)
	{
		if(dm.bpp <= 8) continue;

		m_dms.Add(dm);

		CString str;
		str.Format(_T("%dx%d %dbpp %dHz"), dm.size.cx, dm.size.cy, dm.bpp, dm.freq);
		m_dispmodecombo.AddString(str);

		if(iSel < 0 && dmtoset.fValid && dm.size == dmtoset.size
		&& dm.bpp == dmtoset.bpp && dm.freq == dmtoset.freq)
			iSel = j;

		j++;
	}
	m_dispmodecombo.SetCurSel(iSel);

	m_fExitFullScreenAtTheEnd = s.fExitFullScreenAtTheEnd;
	m_fRememberWindowPos = s.fRememberWindowPos;
	m_fRememberWindowSize = s.fRememberWindowSize;
	m_fUseIni = ((CMPlayerCApp*)AfxGetApp())->IsIniValid();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPagePlayer::OnApply()
{
	UpdateData();

	AppSettings& s = AfxGetAppSettings();

	s.fAllowMultipleInst = !!m_iAllowMultipleInst;
	s.iTitleBarTextStyle = m_iTitleBarTextStyle;
	s.fAlwaysOnTop = !!m_iAlwaysOnTop;
	s.fTrayIcon = !!m_fTrayIcon;
	s.fShowBarsWhenFullScreen = !!m_iShowBarsWhenFullScreen;
	s.nShowBarsWhenFullScreenTimeOut = m_nShowBarsWhenFullScreenTimeOut;
	int iSel = m_dispmodecombo.GetCurSel();
	if((s.dmFullscreenRes.fValid = !!m_fSetFullscreenRes) && iSel >= 0 && iSel < m_dms.GetCount())
		s.dmFullscreenRes = m_dms[m_dispmodecombo.GetCurSel()];
	s.fExitFullScreenAtTheEnd = !!m_fExitFullScreenAtTheEnd;
	s.fRememberWindowPos = !!m_fRememberWindowPos;
	s.fRememberWindowSize = !!m_fRememberWindowSize;

	((CMainFrame*)AfxGetMainWnd())->ShowTrayIcon(s.fTrayIcon);

	return __super::OnApply();
}

void CPPagePlayer::OnBnClickedCheck8()
{
	UpdateData();

	if(m_fUseIni) ((CMPlayerCApp*)AfxGetApp())->StoreSettingsToIni();
	else ((CMPlayerCApp*)AfxGetApp())->StoreSettingsToRegistry();
}

void CPPagePlayer::OnUpdateTimeout(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_iShowBarsWhenFullScreen);
}

void CPPagePlayer::OnUpdateDispModeCombo(CCmdUI* pCmdUI)
{
	UpdateData();
	pCmdUI->Enable(m_fSetFullscreenRes);
}
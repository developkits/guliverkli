// Media Player Classic.  Copyright 2003 Gabest.
// http://www.gabest.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html

#pragma once
#include "PPageBase.h"
#include "FloatEdit.h"

// CPPageRealMediaQuickTime dialog

class CPPageRealMediaQuickTime : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageRealMediaQuickTime)

public:
	CPPageRealMediaQuickTime();
	virtual ~CPPageRealMediaQuickTime();

// Dialog Data
	enum { IDD = IDD_PPAGERMQT };
	BOOL m_fIntRealMedia;
	BOOL m_fRealMediaRenderless;
	int m_iQuickTimeRenderer;
	CFloatEdit m_RealMediaQuickTimeFPS;
	int m_iRtspHandler;
	BOOL m_fRtspFileExtFirst;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
};

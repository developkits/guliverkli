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

#include <afxwin.h>
#include <afxcmn.h>
#include "resource.h"
#include "..\..\subtitles\STS.h"
#include "afxwin.h"

// CPPageSubtitles dialog

class CPPageSubtitles : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageSubtitles)

public:
	CPPageSubtitles();
	virtual ~CPPageSubtitles();

	BOOL m_fOverridePlacement;
	int m_nHorPos;
	CEdit m_nHorPosEdit;
	CSpinButtonCtrl m_nHorPosCtrl;
	int m_nVerPos;
	CEdit m_nVerPosEdit;
	CSpinButtonCtrl m_nVerPosCtrl;
	int m_nSPCSize;
	CSpinButtonCtrl m_nSPCSizeCtrl;
	CComboBox m_spmaxres;

// Dialog Data
	enum { IDD = IDD_PPAGESUBTITLES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnUpdatePosOverride(CCmdUI* pCmdUI);
};

/* 
 *	Media Player Classic.  Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "afxcmn.h"
#include "PPageBase.h"
#include "FloatEdit.h"

// CPPageAudioSwitcher dialog

class CPPageAudioSwitcher : public CPPageBase
{
	DECLARE_DYNAMIC(CPPageAudioSwitcher)

private:
	CComPtr<IUnknown> m_pAudioSwitcher;
	DWORD m_pSpeakerToChannelMap[18][18];
	DWORD m_dwChannelMask;

public:
	CPPageAudioSwitcher(IUnknown* pAudioSwitcher);
	virtual ~CPPageAudioSwitcher();

// Dialog Data
	enum { IDD = IDD_PPAGEAUDIOSWITCHER };

	BOOL m_fEnableAudioSwitcher;
	BOOL m_fDownSampleTo441;
	CButton m_fDownSampleTo441Ctrl;
	BOOL m_fCustomChannelMapping;
	CButton m_fCustomChannelMappingCtrl;
	CEdit m_nChannelsCtrl;
	int m_nChannels;
	CSpinButtonCtrl m_nChannelsSpinCtrl;
	CListCtrl m_list;
	int m_tAudioTimeShift;
	CButton m_fAudioTimeShiftCtrl;
	CIntEdit m_tAudioTimeShiftCtrl;
	CSpinButtonCtrl m_tAudioTimeShiftSpin;
	BOOL m_fAudioTimeShift;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnNMClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnUpdateAudioSwitcher(CCmdUI* pCmdUI);
	afx_msg void OnUpdateChannelMapping(CCmdUI* pCmdUI);
};

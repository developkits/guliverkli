/* 
 *	Copyright (C) 2003-2005 Gabest
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
#include "afxwin.h"

// CPPageFileInfoClip dialog

class CPPageFileInfoClip : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPageFileInfoClip)

private:
	CComPtr<IFilterGraph> m_pFG;

	HICON m_hIcon;

public:
	CPPageFileInfoClip(CString fn, IFilterGraph* pFG);
	virtual ~CPPageFileInfoClip();

// Dialog Data
	enum { IDD = IDD_FILEPROPCLIP };

	CStatic m_icon;
	CString m_fn;
	CString m_clip;
	CString m_author;
	CString m_copyright;
	CString m_rating;
	CString m_location;
	CEdit m_desc;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
};

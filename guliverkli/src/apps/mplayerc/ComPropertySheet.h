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

#include "ComPropertyPage.h"

interface IComPropertyPageDirty
{
	virtual void OnSetDirty(bool fDirty) = 0;
};

// CComPropertySheet

class CComPropertySheet : public CPropertySheet, public IComPropertyPageDirty
{
	DECLARE_DYNAMIC(CComPropertySheet)

	CComPtr<IPropertyPageSite> m_pSite;
	CInterfaceList<ISpecifyPropertyPages> m_spp;
	CAutoPtrList<CComPropertyPage> m_pages;
	CSize m_size;

public:
	CComPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CComPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CComPropertySheet();

	int AddPages(CComPtr<ISpecifyPropertyPages> pSPP);

	// CPropertySheet
	void OnSetDirty(bool fDirty) {if(CPropertyPage* p = GetActivePage()) p->SetModified(fDirty);}

	// IComPropertyPageDirty
	void OnActivated(CPropertyPage* pPage);

	virtual BOOL OnInitDialog();

protected:
	DECLARE_MESSAGE_MAP()
};



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

#include <afxole.h>

class CDropTarget
{
public:
	CDropTarget() {}

	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) {return FALSE;}
	virtual DROPEFFECT OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point) {return (DROPEFFECT)-1;}
	virtual void OnDragLeave() {}
	virtual DROPEFFECT OnDragScroll(DWORD dwKeyState, CPoint point) {return DROPEFFECT_NONE;}
};

// CFileDropTarget command target

class CFileDropTarget : public COleDropTarget
{
//	DECLARE_DYNAMIC(CFileDropTarget)

private:
	CDropTarget* m_pDropTarget;

public:
	CFileDropTarget(CDropTarget* pDropTarget);
	virtual ~CFileDropTarget();

protected:
	DECLARE_MESSAGE_MAP()

	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	DROPEFFECT OnDropEx(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
	void OnDragLeave(CWnd* pWnd);
	DROPEFFECT OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point);
};


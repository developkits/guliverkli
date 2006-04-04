/* 
 *	Copyright (C) 2003-2006 Gabest
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
 *  TODO: 
 *  - fill effect
 *  - collision detection
 *  - outline bkg still very slow
 *
 */

#include "stdafx.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include "SSF.h"
#include "..\subpic\MemSubPic.h"

namespace ssf
{
	CRenderer::CRenderer(CCritSec* pLock)
		: ISubPicProviderImpl(pLock)
	{
	}

	CRenderer::~CRenderer()
	{
	}

	bool CRenderer::Open(CString fn, CString name)
	{
		m_fn.Empty();
		m_name.Empty();
		m_file.Free();
		m_renderer.Free();

		if(name.IsEmpty())
		{
			CString str = fn;
			str.Replace('\\', '/');
			name = str.Left(str.ReverseFind('.'));
			name = name.Mid(name.ReverseFind('/')+1);
			name = name.Mid(name.ReverseFind('.')+1);
		}

		try
		{
			if(Open(FileInputStream(fn), name)) 
			{
				m_fn = fn;
				return true;
			}
		}
		catch(Exception& e)
		{
			TRACE(_T("%s\n"), e.ToString());
		}

		return false;	
	}

	bool CRenderer::Open(InputStream& s, CString name)
	{
		m_fn.Empty();
		m_name.Empty();
		m_file.Free();
		m_renderer.Free();

		try
		{
			m_file.Attach(new SubtitleFile());
			m_file->Parse(s);
			m_renderer.Attach(new Renderer());
			m_name = name;
			return true;
		}
		catch(Exception& e)
		{
			TRACE(_T("%s\n"), e.ToString());
		}

		return false;
	}

	STDMETHODIMP CRenderer::NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		CheckPointer(ppv, E_POINTER);
		*ppv = NULL;

		return 
			QI(IPersist)
			QI(ISubStream)
			QI(ISubPicProvider)
			__super::NonDelegatingQueryInterface(riid, ppv);
	}

	// ISubPicProvider

	STDMETHODIMP_(POSITION) CRenderer::GetStartPosition(REFERENCE_TIME rt, double fps)
	{
		size_t k;
		return m_file && m_file->m_segments.Lookup((float)rt/10000000, k) ? (POSITION)(++k) : NULL;
	}

	STDMETHODIMP_(POSITION) CRenderer::GetNext(POSITION pos)
	{
		size_t k = (size_t)pos;
		return m_file && m_file->m_segments.GetSegment(k) ? (POSITION)(++k) : NULL;
	}

	STDMETHODIMP_(REFERENCE_TIME) CRenderer::GetStart(POSITION pos, double fps)
	{
		size_t k = (size_t)pos-1;
		const SubtitleFile::Segment* s = m_file ? m_file->m_segments.GetSegment(k) : NULL;
		return s ? (REFERENCE_TIME)(s->m_start*10000000) : 0;
	}

	STDMETHODIMP_(REFERENCE_TIME) CRenderer::GetStop(POSITION pos, double fps)
	{
		CheckPointer(m_file, 0);

		size_t k = (size_t)pos-1;
		const SubtitleFile::Segment* s = m_file ? m_file->m_segments.GetSegment(k) : NULL;
		return s ? (REFERENCE_TIME)(s->m_stop*10000000) : 0;
	}

	STDMETHODIMP_(bool) CRenderer::IsAnimated(POSITION pos)
	{
		return true;
	}

	STDMETHODIMP CRenderer::Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox)
	{
		CheckPointer(m_file, E_UNEXPECTED);
		CheckPointer(m_renderer, E_UNEXPECTED);	

		if(spd.type != MSP_RGB32) return E_INVALIDARG;

		CAutoLock csAutoLock(m_pLock);

		CRect bbox2(0, 0, 0, 0);

		CAutoPtrList<Subtitle> subs;
		m_file->Lookup((float)rt/10000000, subs);

		POSITION pos = subs.GetHeadPosition();
		while(pos)
		{
			const Subtitle* s = subs.GetNext(pos);

			const RenderedSubtitle* rs = m_renderer->Lookup(s, CSize(spd.w, spd.h), spd.vidrect);

			if(rs)
			{
				// shadow

				POSITION pos = rs->m_glyphs.GetHeadPosition();
				while(pos)
				{
					Glyph* g = rs->m_glyphs.GetNext(pos);

					if(g->style.shadow.depth <= 0) continue;

					DWORD c = 
						(min(max((DWORD)g->style.shadow.color.b, 0), 255) <<  0) |
						(min(max((DWORD)g->style.shadow.color.g, 0), 255) <<  8) |
						(min(max((DWORD)g->style.shadow.color.r, 0), 255) << 16) |
						(min(max((DWORD)g->style.shadow.color.a, 0), 255) << 24);

					DWORD sw[6] = {c, -1};

					bool outline = g->style.background.type == L"outline" && g->style.background.size > 0;

					bbox2 |= g->ras_shadow.Draw(spd, rs->m_clip, g->tls.x >> 3, g->tls.y >> 3, sw, true, outline);
				}

				// background

				pos = rs->m_glyphs.GetHeadPosition();
				while(pos)
				{
					Glyph* g = rs->m_glyphs.GetNext(pos);

					DWORD c = 
						(min(max((DWORD)g->style.background.color.b, 0), 255) <<  0) |
						(min(max((DWORD)g->style.background.color.g, 0), 255) <<  8) |
						(min(max((DWORD)g->style.background.color.r, 0), 255) << 16) |
						(min(max((DWORD)g->style.background.color.a, 0), 255) << 24);

					DWORD sw[6] = {c, -1};

					if(g->style.background.type == L"outline" && g->style.background.size > 0)
					{
						bool body = !g->style.font.color.a && !g->style.background.color.a;

						bbox2 |= g->ras.Draw(spd, rs->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, body, true);
					}
					else if(g->style.background.type == L"enlarge" && g->style.background.size > 0
					|| g->style.background.type == L"box" && g->style.background.size >= 0)
					{
						bbox2 |= g->ras_bkg.Draw(spd, rs->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, true, false);
					}
				}

				// body

				pos = rs->m_glyphs.GetHeadPosition();
				while(pos)
				{
					Glyph* g = rs->m_glyphs.GetNext(pos);

					DWORD c = 
						(min(max((DWORD)g->style.font.color.b, 0), 255) <<  0) |
						(min(max((DWORD)g->style.font.color.g, 0), 255) <<  8) |
						(min(max((DWORD)g->style.font.color.r, 0), 255) << 16) |
						(min(max((DWORD)g->style.font.color.a, 0), 255) << 24);

					DWORD sw[6] = {c, -1}; // TODO: fill

					bbox2 |= g->ras.Draw(spd, rs->m_clip, g->tl.x >> 3, g->tl.y >> 3, sw, true, false);
				}
			}
		}

		bbox = bbox2 & CRect(0, 0, spd.w, spd.h);

		return S_OK;
	}

	// IPersist

	STDMETHODIMP CRenderer::GetClassID(CLSID* pClassID)
	{
		return pClassID ? *pClassID = __uuidof(this), S_OK : E_POINTER;
	}

	// ISubStream

	STDMETHODIMP_(int) CRenderer::GetStreamCount()
	{
		return 1;
	}

	STDMETHODIMP CRenderer::GetStreamInfo(int iStream, WCHAR** ppName, LCID* pLCID)
	{
		if(iStream != 0) return E_INVALIDARG;

		if(ppName)
		{
			if(!(*ppName = (WCHAR*)CoTaskMemAlloc((m_name.GetLength()+1)*sizeof(WCHAR))))
				return E_OUTOFMEMORY;

			wcscpy(*ppName, CStringW(m_name));
		}

		if(pLCID)
		{
			*pLCID = 0; // TODO
		}

		return S_OK;
	}

	STDMETHODIMP_(int) CRenderer::GetStream()
	{
		return 0;
	}

	STDMETHODIMP CRenderer::SetStream(int iStream)
	{
		return iStream == 0 ? S_OK : E_FAIL;
	}

	STDMETHODIMP CRenderer::Reload()
	{
		CAutoLock csAutoLock(m_pLock);

		return !m_fn.IsEmpty() && Open(m_fn, m_name) ? S_OK : E_FAIL;
	}
}
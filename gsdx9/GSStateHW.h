/* 
 *	Copyright (C) 2003-2004 Gabest
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

#include "GSState.h"

class GSStateHW : public GSState
{
protected:
	CSurfMap<IDirect3DTexture9> m_pRenderTargets;
	CSurfMap<IDirect3DSurface9> m_pDepthStencils;
	CMap<DWORD, DWORD, CGSWnd*, CGSWnd*> m_pRenderWnds;

	GSTextureCache m_tc;
	bool CreateTexture(GSTexture& t);

	#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)

	struct CUSTOMVERTEX
	{
		float x, y, z, rhw;
		D3DCOLOR color;
		float tu, tv;
//		float tu2, tv2;
	};

	GSVertexList<CUSTOMVERTEX> m_vl;

	void Reset();
	void VertexKick(bool fSkip);
	void DrawingKick(bool fSkip);
	void NewPrim();
	void FlushPrim();
	void Flip();
	void EndFrame();
	void InvalidateTexture(DWORD TBP0);

	D3DPRIMITIVETYPE m_primtype;
	CUSTOMVERTEX* m_pVertices;
	int m_nMaxVertices, m_nVertices, m_nPrims;

public:
	GSStateHW(HWND hWnd, HRESULT& hr);
	~GSStateHW();

	void LOGVERTEX(CUSTOMVERTEX& v, LPCTSTR type)
	{
		GSDrawingContext* ctxt = &m_de.CTXT[m_de.PRIM.CTXT];
		int tw = 1, th = 1;
		if(m_de.PRIM.TME) {tw = 1<<ctxt->TEX0.TW; th = 1<<ctxt->TEX0.TH;}
		LOG2((_T("\t %s (%.2f, %.2f, %.2f, %.2f) (%08x) (%f, %f) (%f, %f)\n"), 
			type,
			v.x, v.y, v.z, v.rhw, 
			v.color, v.tu, v.tv,
			v.tu*tw, v.tv*th));
	}
};
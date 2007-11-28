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

extern bool IsDepthFormatOk(IDirect3D9* pD3D, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat);
extern bool IsRenderTarget(IDirect3DTexture9* pTexture);
extern HRESULT CompileShaderFromResource(IDirect3DDevice9* dev, UINT id, CString entry, CString target, UINT flags, IDirect3DVertexShader9** ppVertexShader, ID3DXConstantTable** ppConstantTable = NULL);
extern HRESULT CompileShaderFromResource(IDirect3DDevice9* dev, UINT id, CString entry, CString target, UINT flags, IDirect3DPixelShader9** ppPixelShader, ID3DXConstantTable** ppConstantTable = NULL);
extern HRESULT AssembleShaderFromResource(IDirect3DDevice9* dev, UINT id, UINT flags, IDirect3DPixelShader9** ppPixelShader);
extern bool HasSharedBits(DWORD spsm, DWORD dpsm);
extern bool HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm);
extern bool IsRectInRect(const CRect& inner, const CRect& outer);
extern bool IsRectInRectH(const CRect& inner, const CRect& outer);
extern bool IsRectInRectV(const CRect& inner, const CRect& outer);
extern BYTE* LoadResource(UINT id, DWORD& size);
extern bool CompileTFX(IDirect3DDevice9* dev, IDirect3DPixelShader9** ps, CString target, DWORD flags, int tfx, int bpp, int tcc, int aem, int fog, int rt, int fst, int clamp);
extern bool CompileTFX(CString fn, CString target, DWORD flags);


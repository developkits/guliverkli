/* 
 *	Copyright (C) 2007 Gabest
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

#include "stdafx.h"
#include "GSState.h"

GSState::GSState(BYTE* base, bool mt, void (*irq)(), int nloophack)
	: m_mt(mt)
	, m_irq(irq)
	, m_nloophack_org(nloophack)
	, m_nloophack(nloophack == 1)
	, m_crc(0)
	, m_options(0)
	, m_path3hack(0)
	, m_q(1.0f)
	, m_version(4)
	, m_vmsize(4 * 1024 * 1024)
{
	m_sssize = sizeof(m_version) + sizeof(m_env) + sizeof(m_v) + sizeof(m_x) + sizeof(m_y) + m_vmsize + sizeof(m_path) + sizeof(m_q);

	ASSERT(base);

	PMODE = (GSRegPMODE*)(base + GS_PMODE);
	SMODE1 = (GSRegSMODE1*)(base + GS_SMODE1);
	SMODE2 = (GSRegSMODE2*)(base + GS_SMODE2);
	// SRFSH = (GSRegPMODE*)(base + GS_SRFSH);
	// SYNCH1 = (GSRegPMODE*)(base + GS_SYNCH1);
	// SYNCH2 = (GSRegPMODE*)(base + GS_SYNCH2);
	// SYNCV = (GSRegPMODE*)(base + GS_SYNCV);
	DISPFB[0] = (GSRegDISPFB*)(base + GS_DISPFB1);
	DISPFB[1] = (GSRegDISPFB*)(base + GS_DISPFB2);
	DISPLAY[0] = (GSRegDISPLAY*)(base + GS_DISPLAY1);
	DISPLAY[1] = (GSRegDISPLAY*)(base + GS_DISPLAY2);
	EXTBUF = (GSRegEXTBUF*)(base + GS_EXTBUF);
	EXTDATA = (GSRegEXTDATA*)(base + GS_EXTDATA);
	EXTWRITE = (GSRegEXTWRITE*)(base + GS_EXTWRITE);
	BGCOLOR = (GSRegBGCOLOR*)(base + GS_BGCOLOR);
	CSR = (GSRegCSR*)(base + GS_CSR);
	IMR = (GSRegIMR*)(base + GS_IMR);
	BUSDIR = (GSRegBUSDIR*)(base + GS_BUSDIR);
	SIGLBLID = (GSRegSIGLBLID*)(base + GS_SIGLBLID);

	PRIM = &m_env.PRIM;
//	CSR->rREV = 0x20;
	m_env.PRMODECONT.AC = 1;

	m_x = m_y = 0;
	m_bytes = 0;
	m_maxbytes = 1024 * 1024 * 4;
	m_buff = (BYTE*)_aligned_malloc(m_maxbytes, 16);

	Reset();

	ResetHandlers();
}

GSState::~GSState()
{
	_aligned_free(m_buff);
}

void GSState::Reset()
{
	memset(&m_env, 0, sizeof(m_env));
	memset(m_path, 0, sizeof(m_path));
	memset(&m_v, 0, sizeof(m_v));

//	PRIM = &m_env.PRIM;
//	m_env.PRMODECONT.AC = 1;

	m_context = &m_env.CTXT[0];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psm[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psm[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psm[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psm[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psm[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psm[m_env.CTXT[1].TEX0.PSM];
}

void GSState::ResetHandlers()
{
	for(int i = 0; i < countof(m_fpGIFPackedRegHandlers); i++)
	{
		m_fpGIFPackedRegHandlers[i] = &GSState::GIFPackedRegHandlerNull;
	}

	m_fpGIFPackedRegHandlers[GIF_REG_PRIM] = &GSState::GIFPackedRegHandlerPRIM;
	m_fpGIFPackedRegHandlers[GIF_REG_RGBA] = &GSState::GIFPackedRegHandlerRGBA;
	m_fpGIFPackedRegHandlers[GIF_REG_STQ] = &GSState::GIFPackedRegHandlerSTQ;
	m_fpGIFPackedRegHandlers[GIF_REG_UV] = &GSState::GIFPackedRegHandlerUV;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF2] = &GSState::GIFPackedRegHandlerXYZF2;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ2] = &GSState::GIFPackedRegHandlerXYZ2;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_1] = &GSState::GIFPackedRegHandlerTEX0<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_TEX0_2] = &GSState::GIFPackedRegHandlerTEX0<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_1] = &GSState::GIFPackedRegHandlerCLAMP<0>;
	m_fpGIFPackedRegHandlers[GIF_REG_CLAMP_2] = &GSState::GIFPackedRegHandlerCLAMP<1>;
	m_fpGIFPackedRegHandlers[GIF_REG_FOG] = &GSState::GIFPackedRegHandlerFOG;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZF3] = &GSState::GIFPackedRegHandlerXYZF3;
	m_fpGIFPackedRegHandlers[GIF_REG_XYZ3] = &GSState::GIFPackedRegHandlerXYZ3;
	m_fpGIFPackedRegHandlers[GIF_REG_A_D] = &GSState::GIFPackedRegHandlerA_D;
	m_fpGIFPackedRegHandlers[GIF_REG_NOP] = &GSState::GIFPackedRegHandlerNOP;

	for(int i = 0; i < countof(m_fpGIFRegHandlers); i++)
	{
		m_fpGIFRegHandlers[i] = &GSState::GIFRegHandlerNull;
	}

	m_fpGIFRegHandlers[GIF_A_D_REG_PRIM] = &GSState::GIFRegHandlerPRIM;
	m_fpGIFRegHandlers[GIF_A_D_REG_RGBAQ] = &GSState::GIFRegHandlerRGBAQ;
	m_fpGIFRegHandlers[GIF_A_D_REG_ST] = &GSState::GIFRegHandlerST;
	m_fpGIFRegHandlers[GIF_A_D_REG_UV] = &GSState::GIFRegHandlerUV;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF2] = &GSState::GIFRegHandlerXYZF2;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ2] = &GSState::GIFRegHandlerXYZ2;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_1] = &GSState::GIFRegHandlerTEX0<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX0_2] = &GSState::GIFRegHandlerTEX0<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_1] = &GSState::GIFRegHandlerCLAMP<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_CLAMP_2] = &GSState::GIFRegHandlerCLAMP<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOG] = &GSState::GIFRegHandlerFOG;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZF3] = &GSState::GIFRegHandlerXYZF3;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYZ3] = &GSState::GIFRegHandlerXYZ3;
	m_fpGIFRegHandlers[GIF_A_D_REG_NOP] = &GSState::GIFRegHandlerNOP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_1] = &GSState::GIFRegHandlerTEX1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX1_2] = &GSState::GIFRegHandlerTEX1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_1] = &GSState::GIFRegHandlerTEX2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEX2_2] = &GSState::GIFRegHandlerTEX2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_1] = &GSState::GIFRegHandlerXYOFFSET<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_XYOFFSET_2] = &GSState::GIFRegHandlerXYOFFSET<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODECONT] = &GSState::GIFRegHandlerPRMODECONT;
	m_fpGIFRegHandlers[GIF_A_D_REG_PRMODE] = &GSState::GIFRegHandlerPRMODE;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXCLUT] = &GSState::GIFRegHandlerTEXCLUT;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCANMSK] = &GSState::GIFRegHandlerSCANMSK;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_1] = &GSState::GIFRegHandlerMIPTBP1<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP1_2] = &GSState::GIFRegHandlerMIPTBP1<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_1] = &GSState::GIFRegHandlerMIPTBP2<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_MIPTBP2_2] = &GSState::GIFRegHandlerMIPTBP2<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXA] = &GSState::GIFRegHandlerTEXA;
	m_fpGIFRegHandlers[GIF_A_D_REG_FOGCOL] = &GSState::GIFRegHandlerFOGCOL;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEXFLUSH] = &GSState::GIFRegHandlerTEXFLUSH;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_1] = &GSState::GIFRegHandlerSCISSOR<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_SCISSOR_2] = &GSState::GIFRegHandlerSCISSOR<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_1] = &GSState::GIFRegHandlerALPHA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ALPHA_2] = &GSState::GIFRegHandlerALPHA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_DIMX] = &GSState::GIFRegHandlerDIMX;
	m_fpGIFRegHandlers[GIF_A_D_REG_DTHE] = &GSState::GIFRegHandlerDTHE;
	m_fpGIFRegHandlers[GIF_A_D_REG_COLCLAMP] = &GSState::GIFRegHandlerCOLCLAMP;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_1] = &GSState::GIFRegHandlerTEST<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_TEST_2] = &GSState::GIFRegHandlerTEST<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_PABE] = &GSState::GIFRegHandlerPABE;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_1] = &GSState::GIFRegHandlerFBA<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FBA_2] = &GSState::GIFRegHandlerFBA<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_1] = &GSState::GIFRegHandlerFRAME<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_FRAME_2] = &GSState::GIFRegHandlerFRAME<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_1] = &GSState::GIFRegHandlerZBUF<0>;
	m_fpGIFRegHandlers[GIF_A_D_REG_ZBUF_2] = &GSState::GIFRegHandlerZBUF<1>;
	m_fpGIFRegHandlers[GIF_A_D_REG_BITBLTBUF] = &GSState::GIFRegHandlerBITBLTBUF;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXPOS] = &GSState::GIFRegHandlerTRXPOS;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXREG] = &GSState::GIFRegHandlerTRXREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_TRXDIR] = &GSState::GIFRegHandlerTRXDIR;
	m_fpGIFRegHandlers[GIF_A_D_REG_HWREG] = &GSState::GIFRegHandlerHWREG;
	m_fpGIFRegHandlers[GIF_A_D_REG_SIGNAL] = &GSState::GIFRegHandlerSIGNAL;
	m_fpGIFRegHandlers[GIF_A_D_REG_FINISH] = &GSState::GIFRegHandlerFINISH;
	m_fpGIFRegHandlers[GIF_A_D_REG_LABEL] = &GSState::GIFRegHandlerLABEL;
}

CPoint GSState::GetDisplayPos(int i)
{
	ASSERT(i >= 0 && i < 2);

	CPoint p;

	p.x = DISPLAY[i]->DX / (DISPLAY[i]->MAGH + 1);
	p.y = DISPLAY[i]->DY / (DISPLAY[i]->MAGV + 1);

	return p;
}

CSize GSState::GetDisplaySize(int i)
{
	ASSERT(i >= 0 && i < 2);

	CSize s;

	s.cx = (DISPLAY[i]->DW + 1) / (DISPLAY[i]->MAGH + 1);
	s.cy = (DISPLAY[i]->DH + 1) / (DISPLAY[i]->MAGV + 1);

	if(s.cy & 1) s.cy++;

	return s;
}

CRect GSState::GetDisplayRect(int i)
{
	return CRect(GetDisplayPos(i), GetDisplaySize(i));
}

CSize GSState::GetDisplayPos()
{
	return GetDisplayPos(IsEnabled(1) ? 1 : 0);
}	

CSize GSState::GetDisplaySize()
{
	return GetDisplaySize(IsEnabled(1) ? 1 : 0);
}	

CRect GSState::GetDisplayRect()
{
	return GetDisplayRect(IsEnabled(1) ? 1 : 0);
}

CPoint GSState::GetFramePos(int i)
{
	ASSERT(i >= 0 && i < 2);

	return CPoint(DISPFB[i]->DBX, DISPFB[i]->DBY);
}

CSize GSState::GetFrameSize(int i)
{
	CSize s = GetDisplaySize(i);

	if(SMODE2->INT && SMODE2->FFMD && s.cy > 1) s.cy >>= 1;

	return s;
}

CRect GSState::GetFrameRect(int i)
{
	return CRect(GetFramePos(i), GetFrameSize(i));
}

CSize GSState::GetFramePos()
{
	return GetFramePos(IsEnabled(1) ? 1 : 0);
}	

CSize GSState::GetFrameSize()
{
	return GetFrameSize(IsEnabled(1) ? 1 : 0);
}	

CRect GSState::GetFrameRect()
{
	return GetFrameRect(IsEnabled(1) ? 1 : 0);
}

bool GSState::IsEnabled(int i)
{
	ASSERT(i >= 0 && i < 2);

	if(i == 0 && PMODE->EN1) 
	{
		return DISPLAY[0]->DW || DISPLAY[0]->DH;
	}
	else if(i == 1 && PMODE->EN2) 
	{
		return DISPLAY[1]->DW || DISPLAY[1]->DH;
	}

	return false;
}

int GSState::GetFPS()
{
	return ((SMODE1->CMOD & 1) ? 50 : 60) / (SMODE2->INT ? 1 : 2);
}

//

static __m128i _000000ff = _mm_set1_epi32(0x000000ff);
static __m128i _00003fff = _mm_set1_epi32(0x00003fff);

// GIFPackedRegHandler*

void GSState::GIFPackedRegHandlerNull(GIFPackedReg* r)
{
	// ASSERT(0);
}

void GSState::GIFPackedRegHandlerPRIM(GIFPackedReg* r)
{
	GIFReg r2;
	r2.PRIM.i64 = r->PRIM.PRIM;
	ASSERT(r2.PRIM.PRIM < 7);
	GIFRegHandlerPRIM(&r2);
}

void GSState::GIFPackedRegHandlerRGBA(GIFPackedReg* r)
{
#if defined(_M_AMD64) || _M_IX86_FP >= 2

	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_and_si128(r0, _000000ff);
	r0 = _mm_packs_epi32(r0, r0);
	r0 = _mm_packus_epi16(r0, r0);
	m_v.RGBAQ.ai32[0] = _mm_cvtsi128_si32(r0);

#else

	m_v.RGBAQ.R = r->RGBA.R;
	m_v.RGBAQ.G = r->RGBA.G;
	m_v.RGBAQ.B = r->RGBA.B;
	m_v.RGBAQ.A = r->RGBA.A;

#endif

	m_v.RGBAQ.Q = m_q;
}

void GSState::GIFPackedRegHandlerSTQ(GIFPackedReg* r)
{
#if defined(_M_AMD64)

	m_v.ST.i64 = r->ai64[0];

#elif _M_IX86_FP >= 2

	_mm_storel_epi64((__m128i*)&m_v.ST.i64, _mm_loadl_epi64((__m128i*)r));

#else

	m_v.ST.S = r->STQ.S;
	m_v.ST.T = r->STQ.T;

#endif

	m_q = r->STQ.Q;
}

void GSState::GIFPackedRegHandlerUV(GIFPackedReg* r)
{
#if defined(_M_AMD64) || _M_IX86_FP >= 2

	__m128i r0 = _mm_loadu_si128((__m128i*)r);
	r0 = _mm_and_si128(r0, _00003fff);
	r0 = _mm_packs_epi32(r0, r0);
	m_v.UV.ai32[0] = _mm_cvtsi128_si32(r0);

#else

	m_v.UV.U = r->UV.U;
	m_v.UV.V = r->UV.V;

#endif
}

void GSState::GIFPackedRegHandlerXYZF2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZF2.X;
	m_v.XYZ.Y = r->XYZF2.Y;
	m_v.XYZ.Z = r->XYZF2.Z;
	m_v.FOG.F = r->XYZF2.F;

	VertexKick(r->XYZF2.ADC);
}

void GSState::GIFPackedRegHandlerXYZ2(GIFPackedReg* r)
{
	m_v.XYZ.X = r->XYZ2.X;
	m_v.XYZ.Y = r->XYZ2.Y;
	m_v.XYZ.Z = r->XYZ2.Z;

	VertexKick(r->XYZ2.ADC);
}

template<int i> void GSState::GIFPackedRegHandlerTEX0(GIFPackedReg* r)
{
	GIFRegHandlerTEX0<i>((GIFReg*)&r->ai64[0]);
}

template<int i> void GSState::GIFPackedRegHandlerCLAMP(GIFPackedReg* r)
{
	GIFRegHandlerCLAMP<i>((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerFOG(GIFPackedReg* r)
{
	m_v.FOG.F = r->FOG.F;
}

void GSState::GIFPackedRegHandlerXYZF3(GIFPackedReg* r)
{
	GIFRegHandlerXYZF3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerXYZ3(GIFPackedReg* r)
{
	GIFRegHandlerXYZ3((GIFReg*)&r->ai64[0]);
}

void GSState::GIFPackedRegHandlerA_D(GIFPackedReg* r)
{
	(this->*m_fpGIFRegHandlers[(BYTE)r->A_D.ADDR])(&r->r);
}

void GSState::GIFPackedRegHandlerNOP(GIFPackedReg* r)
{
}

// GIFRegHandler*

void GSState::GIFRegHandlerNull(GIFReg* r)
{
	// ASSERT(0);
}

void GSState::GIFRegHandlerPRIM(GIFReg* r)
{
	// ASSERT(r->PRIM.PRIM < 7);

	if(GetPrimClass(m_env.PRIM.PRIM) == GetPrimClass(r->PRIM.PRIM))
	{
		if(((m_env.PRIM.i64 ^ r->PRIM.i64) & ~7) != 0)
		{
			Flush();
		}
	}
	else
	{
		if(m_env.PRIM.i64 != r->PRIM.i64)
		{
			Flush();
		}
	}

	m_env.PRIM = r->PRIM;
	m_env.PRMODE._PRIM = r->PRIM.PRIM;

	if(m_env.PRMODECONT.AC)
	{
		m_context = &m_env.CTXT[m_env.PRIM.CTXT];
	}

	ResetPrim();
}

void GSState::GIFRegHandlerRGBAQ(GIFReg* r)
{
	m_v.RGBAQ = r->RGBAQ;
}

void GSState::GIFRegHandlerST(GIFReg* r)
{
	m_v.ST = r->ST;
}

void GSState::GIFRegHandlerUV(GIFReg* r)
{
	m_v.UV = r->UV;
}

void GSState::GIFRegHandlerXYZF2(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(false);
}

void GSState::GIFRegHandlerXYZ2(GIFReg* r)
{
	m_v.XYZ = r->XYZ;

	VertexKick(false);
}

template<int i> void GSState::GIFRegHandlerTEX0(GIFReg* r)
{
	// even if TEX0 did not change, a new palette may have been uploaded and will overwrite the currently queued for drawing

	if(PRIM->CTXT == i && m_env.CTXT[i].TEX0.i64 != r->TEX0.i64 || m_mem.IsCLUTUpdating(r->TEX0, m_env.TEXCLUT))
	{
		Flush(); 
	}

	m_env.CTXT[i].TEX0 = r->TEX0;

	// ASSERT(m_env.CTXT[i].TEX0.TW <= 10 && m_env.CTXT[i].TEX0.TH <= 10 && (m_env.CTXT[i].TEX0.CPSM & ~0xa) == 0);

	if(m_env.CTXT[i].TEX0.TW > 10) m_env.CTXT[i].TEX0.TW = 10;
	if(m_env.CTXT[i].TEX0.TH > 10) m_env.CTXT[i].TEX0.TH = 10;

	m_env.CTXT[i].TEX0.CPSM &= 0xa; // 1010b

	m_env.CTXT[i].ttbl = &GSLocalMemory::m_psm[m_env.CTXT[i].TEX0.PSM];

	FlushWrite();

	m_mem.WriteCLUT(r->TEX0, m_env.TEXCLUT);
}

template<int i> void GSState::GIFRegHandlerCLAMP(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].CLAMP.i64 != r->CLAMP.i64)
	{
		Flush();
	}

	m_env.CTXT[i].CLAMP = r->CLAMP;
}

void GSState::GIFRegHandlerFOG(GIFReg* r)
{
	m_v.FOG = r->FOG;
}

void GSState::GIFRegHandlerXYZF3(GIFReg* r)
{
/*
	m_v.XYZ.X = r->XYZF.X;
	m_v.XYZ.Y = r->XYZF.Y;
	m_v.XYZ.Z = r->XYZF.Z;
	m_v.FOG.F = r->XYZF.F;
*/
	m_v.XYZ.ai32[0] = r->XYZF.ai32[0];
	m_v.XYZ.ai32[1] = r->XYZF.ai32[1] & 0x00ffffff;
	m_v.FOG.ai32[1] = r->XYZF.ai32[1] & 0xff000000;

	VertexKick(true);
}

void GSState::GIFRegHandlerXYZ3(GIFReg* r)
{
	m_v.XYZ = r->XYZ;

	VertexKick(true);
}

void GSState::GIFRegHandlerNOP(GIFReg* r)
{
}

template<int i> void GSState::GIFRegHandlerTEX1(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].TEX1.i64 != r->TEX1.i64)
	{
		Flush();
	}

	m_env.CTXT[i].TEX1 = r->TEX1;
}

template<int i> void GSState::GIFRegHandlerTEX2(GIFReg* r)
{
	// m_env.CTXT[i].TEX2 = r->TEX2; // not used

	UINT64 mask = 0xFFFFFFE003F00000ui64; // TEX2 bits

	r->i64 = (r->i64 & mask) | (m_env.CTXT[i].TEX0.i64 & ~mask);

	GIFRegHandlerTEX0<i>(r);
}

template<int i> void GSState::GIFRegHandlerXYOFFSET(GIFReg* r)
{
	if(m_env.CTXT[i].XYOFFSET.i64 != r->XYOFFSET.i64)
	{
		Flush();
	}

	m_env.CTXT[i].XYOFFSET = r->XYOFFSET;

	m_env.CTXT[i].UpdateScissor();
}

void GSState::GIFRegHandlerPRMODECONT(GIFReg* r)
{
	if(m_env.PRMODECONT.i64 != r->PRMODECONT.i64)
	{
		Flush();
	}

	m_env.PRMODECONT = r->PRMODECONT;

	PRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	if(PRIM->PRIM == 7) TRACE(_T("Invalid PRMODECONT/PRIM\n"));

	m_context = &m_env.CTXT[PRIM->CTXT];
}

void GSState::GIFRegHandlerPRMODE(GIFReg* r)
{
	if(!m_env.PRMODECONT.AC)
	{
		Flush();
	}

	UINT32 _PRIM = m_env.PRMODE._PRIM;
	m_env.PRMODE = r->PRMODE;
	m_env.PRMODE._PRIM = _PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];
}

void GSState::GIFRegHandlerTEXCLUT(GIFReg* r)
{
	if(m_env.TEXCLUT.i64 != r->TEXCLUT.i64)
	{
		Flush();
	}

	m_env.TEXCLUT = r->TEXCLUT;
}

void GSState::GIFRegHandlerSCANMSK(GIFReg* r)
{
	if(m_env.SCANMSK.i64 != r->SCANMSK.i64)
	{
		Flush();
	}

	m_env.SCANMSK = r->SCANMSK;
}

template<int i> void GSState::GIFRegHandlerMIPTBP1(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].MIPTBP1.i64 != r->MIPTBP1.i64)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP1 = r->MIPTBP1;
}

template<int i> void GSState::GIFRegHandlerMIPTBP2(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].MIPTBP2.i64 != r->MIPTBP2.i64)
	{
		Flush();
	}

	m_env.CTXT[i].MIPTBP2 = r->MIPTBP2;
}

void GSState::GIFRegHandlerTEXA(GIFReg* r)
{
	if(m_env.TEXA.i64 != r->TEXA.i64)
	{
		Flush();
	}

	m_env.TEXA = r->TEXA;
}

void GSState::GIFRegHandlerFOGCOL(GIFReg* r)
{
	if(m_env.FOGCOL.i64 != r->FOGCOL.i64)
	{
		Flush();
	}

	m_env.FOGCOL = r->FOGCOL;
}

void GSState::GIFRegHandlerTEXFLUSH(GIFReg* r)
{
	// what should we do here?
}

template<int i> void GSState::GIFRegHandlerSCISSOR(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].SCISSOR.i64 != r->SCISSOR.i64)
	{
		Flush();
	}

	m_env.CTXT[i].SCISSOR = r->SCISSOR;

	m_env.CTXT[i].UpdateScissor();
}

template<int i> void GSState::GIFRegHandlerALPHA(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].ALPHA.i64 != r->ALPHA.i64)
	{
		Flush();
	}

	m_env.CTXT[i].ALPHA = r->ALPHA;
}

void GSState::GIFRegHandlerDIMX(GIFReg* r)
{
	if(m_env.DIMX.i64 != r->DIMX.i64)
	{
		Flush();
	}

	m_env.DIMX = r->DIMX;
}

void GSState::GIFRegHandlerDTHE(GIFReg* r)
{
	if(m_env.DTHE.i64 != r->DTHE.i64)
	{
		Flush();
	}

	m_env.DTHE = r->DTHE;
}

void GSState::GIFRegHandlerCOLCLAMP(GIFReg* r)
{
	if(m_env.COLCLAMP.i64 != r->COLCLAMP.i64)
	{
		Flush();
	}

	m_env.COLCLAMP = r->COLCLAMP;
}

template<int i> void GSState::GIFRegHandlerTEST(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].TEST.i64 != r->TEST.i64)
	{
		Flush();
	}

	m_env.CTXT[i].TEST = r->TEST;
}

void GSState::GIFRegHandlerPABE(GIFReg* r)
{
	if(m_env.PABE.i64 != r->PABE.i64)
	{
		Flush();
	}

	m_env.PABE = r->PABE;
}

template<int i> void GSState::GIFRegHandlerFBA(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].FBA.i64 != r->FBA.i64)
	{
		Flush();
	}

	m_env.CTXT[i].FBA = r->FBA;
}

template<int i> void GSState::GIFRegHandlerFRAME(GIFReg* r)
{
	if(PRIM->CTXT == i && m_env.CTXT[i].FRAME.i64 != r->FRAME.i64)
	{
		Flush();
	}

	m_env.CTXT[i].FRAME = r->FRAME;

	m_env.CTXT[i].ftbl = &GSLocalMemory::m_psm[m_env.CTXT[i].FRAME.PSM];
}

template<int i> void GSState::GIFRegHandlerZBUF(GIFReg* r)
{
	r->ZBUF.PSM |= 0x30;

	if(PRIM->CTXT == i && m_env.CTXT[i].ZBUF.i64 != r->ZBUF.i64)
	{
		Flush();
	}

	m_env.CTXT[i].ZBUF = r->ZBUF;

	if(m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ32
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ24
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16
	&& m_env.CTXT[i].ZBUF.PSM != PSM_PSMZ16S)
	{
		m_env.CTXT[i].ZBUF.PSM = PSM_PSMZ32;
	}

	m_env.CTXT[i].ztbl = &GSLocalMemory::m_psm[m_env.CTXT[i].ZBUF.PSM];
}

void GSState::GIFRegHandlerBITBLTBUF(GIFReg* r)
{
	if(m_env.BITBLTBUF.i64 != r->BITBLTBUF.i64)
	{
		FlushWrite();
	}

	m_env.BITBLTBUF = r->BITBLTBUF;
}

void GSState::GIFRegHandlerTRXPOS(GIFReg* r)
{
	if(m_env.TRXPOS.i64 != r->TRXPOS.i64)
	{
		FlushWrite();
	}

	m_env.TRXPOS = r->TRXPOS;
}

void GSState::GIFRegHandlerTRXREG(GIFReg* r)
{
	if(m_env.TRXREG.i64 != r->TRXREG.i64 || m_env.TRXREG2.i64 != r->TRXREG.i64)
	{
		FlushWrite();
	}

	m_env.TRXREG = m_env.TRXREG2 = r->TRXREG;
}

void GSState::GIFRegHandlerTRXDIR(GIFReg* r)
{
	Flush();

	m_env.TRXDIR = r->TRXDIR;

	switch(m_env.TRXDIR.XDIR)
	{
	case 0: // host -> local
		m_x = m_env.TRXPOS.DSAX;
		m_y = m_env.TRXPOS.DSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 1: // local -> host
		m_x = m_env.TRXPOS.SSAX;
		m_y = m_env.TRXPOS.SSAY;
		m_env.TRXREG.RRW = m_x + m_env.TRXREG2.RRW;
		m_env.TRXREG.RRH = m_y + m_env.TRXREG2.RRH;
		break;
	case 2: // local -> local
		Move();
		break;
	case 3: 
		ASSERT(0);
		break;
	}
}

void GSState::GIFRegHandlerHWREG(GIFReg* r)
{
	// TODO

	ASSERT(0);
}

void GSState::GIFRegHandlerSIGNAL(GIFReg* r)
{
	if(m_mt) return;

	SIGLBLID->SIGID = (SIGLBLID->SIGID & ~r->SIGNAL.IDMSK) | (r->SIGNAL.ID & r->SIGNAL.IDMSK);

	if(CSR->wSIGNAL) CSR->rSIGNAL = 1;
	if(!IMR->SIGMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerFINISH(GIFReg* r)
{
	if(m_mt) return;

	if(CSR->wFINISH) CSR->rFINISH = 1;
	if(!IMR->FINISHMSK && m_irq) m_irq();
}

void GSState::GIFRegHandlerLABEL(GIFReg* r)
{
	if(m_mt) return;

	SIGLBLID->LBLID = (SIGLBLID->LBLID & ~r->LABEL.IDMSK) | (r->LABEL.ID & r->LABEL.IDMSK);
}

//

void GSState::Flush()
{
	FlushWrite();

	FlushPrim();
}

void GSState::FlushWrite()
{
	FlushWrite(m_buff, m_bytes);

	m_bytes = 0;
}

void GSState::FlushWrite(BYTE* mem, int len)
{
	if(len > 0)
	{
		int y = m_y;

		GSLocalMemory::SwizzleTexture st = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].st;

		(m_mem.*st)(m_x, m_y, mem, len, m_env.BITBLTBUF, m_env.TRXPOS, m_env.TRXREG);

		m_perfmon.Put(GSPerfMon::Swizzle, len);

		//ASSERT(m_env.TRXREG.RRH >= m_y - y);

		CRect r;
		
		r.left = m_env.TRXPOS.DSAX;
		r.top = y;
		r.right = m_env.TRXREG.RRW;
		r.bottom = min(m_x == m_env.TRXPOS.DSAX ? m_y : m_y + 1, m_env.TRXREG.RRH);

		InvalidateVideoMem(m_env.BITBLTBUF, r);
	}
}

//

void GSState::Write(BYTE* mem, int len)
{
	if(len == 0) return;

	// TODO: hmmmm

	if(PRIM->TME && (m_env.BITBLTBUF.DBP == m_context->TEX0.TBP0 || m_env.BITBLTBUF.DBP == m_context->TEX0.CBP))
	{
		FlushPrim();
	}

	int bpp = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].trbpp;

	int pitch = (m_env.TRXREG.RRW - m_env.TRXPOS.DSAX) * bpp >> 3;

	if(pitch <= 0) {ASSERT(0); return;}

	int height = len / pitch;

	if(height > m_env.TRXREG.RRH - m_env.TRXPOS.DSAY)
	{
		height = m_env.TRXREG.RRH - m_env.TRXPOS.DSAY;

		len = height * pitch;
	}

	if(m_bytes > 0 || height < m_env.TRXREG.RRH - m_env.TRXPOS.DSAY)
	{
		ASSERT(len <= m_maxbytes); // more than 4mb into a 4mb local mem doesn't make sense

		len = min(m_maxbytes, len);

		if(m_bytes + len > m_maxbytes)
		{
			FlushWrite();
		}

		memcpy(&m_buff[m_bytes], mem, len);

		m_bytes += len;
	}
	else
	{
		FlushWrite(mem, len);
	}

	m_mem.InvalidateCLUT();
}

void GSState::Read(BYTE* mem, int len)
{
	BYTE* pb = (BYTE*)mem;
	WORD* pw = (WORD*)mem;
	DWORD* pd = (DWORD*)mem;

	if(m_y >= (int)m_env.TRXREG.RRH) {ASSERT(0); return;}

	if(m_x == m_env.TRXPOS.SSAX && m_y == m_env.TRXPOS.SSAY)
	{
		CRect r(m_env.TRXPOS.SSAX, m_env.TRXPOS.SSAY, m_env.TRXREG.RRW, m_env.TRXREG.RRH);

		InvalidateLocalMem(m_env.BITBLTBUF, r);
	}

	switch(m_env.BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pd++)
			*pd = m_mem.readPixel32(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb+=3)
		{
			DWORD dw = m_mem.readPixel24(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pw++)
			*pw = (WORD)m_mem.readPixel16(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pw++)
			*pw = (WORD)m_mem.readPixel16S(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb++)
			*pb = (BYTE)m_mem.readPixel8(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb++)
			*pb = (BYTE)(m_mem.readPixel4(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb++)
			*pb = (BYTE)m_mem.readPixel8H(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb++)
			*pb = (BYTE)(m_mem.readPixel4HL(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4HL(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb++)
			*pb = (BYTE)(m_mem.readPixel4HH(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_mem.readPixel4HH(m_x+1, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pd++)
			*pd = m_mem.readPixel32Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pb+=3)
		{
			DWORD dw = m_mem.readPixel24Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pw++)
			*pw = (WORD)m_mem.readPixel16Z(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; StepTransfer(m_env.TRXPOS.SSAX, m_env.TRXREG.RRW), pw++)
			*pw = (WORD)m_mem.readPixel16SZ(m_x, m_y, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW);
		break;
	}
}

void GSState::Move()
{
	// ffxii uses this to move the top/bottom of the scrolling menus offscreen and then blends them back over the text to create a shading effect
	// guitar hero copies the far end of the board to do a similar blend too

	GSLocalMemory::readPixel rp = GSLocalMemory::m_psm[m_env.BITBLTBUF.SPSM].rp;
	GSLocalMemory::writePixel wp = GSLocalMemory::m_psm[m_env.BITBLTBUF.DPSM].wp;

	int sx = m_env.TRXPOS.SSAX;
	int dx = m_env.TRXPOS.DSAX;
	int sy = m_env.TRXPOS.SSAY;
	int dy = m_env.TRXPOS.DSAY;
	int w = m_env.TRXREG.RRW;
	int h = m_env.TRXREG.RRH;
	int xinc = 1;
	int yinc = 1;

	if(sx < dx) sx += w-1, dx += w-1, xinc = -1;
	if(sy < dy) sy += h-1, dy += h-1, yinc = -1;

	InvalidateLocalMem(m_env.BITBLTBUF, CRect(CPoint(sx, sy), CSize(w, h)));
	InvalidateVideoMem(m_env.BITBLTBUF, CRect(CPoint(dx, dy), CSize(w, h)));

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_mem.*wp)(dx, dy, (m_mem.*rp)(sx, sy, m_env.BITBLTBUF.SBP, m_env.BITBLTBUF.SBW), m_env.BITBLTBUF.DBP, m_env.BITBLTBUF.DBW);
}

void GSState::ReadFIFO(BYTE* mem, int size)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	Flush();

	Read(mem, size * 16);
}

void GSState::Transfer(BYTE* mem, int size, int index)
{
	GSPerfMonAutoTimer pmat(m_perfmon);

	GIFPath& path = m_path[index];

	while(size > 0)
	{
		bool eop = false;

		if(path.tag.NLOOP == 0)
		{
			path.tag = *(GIFTag*)mem;
			path.nreg = 0;

			mem += sizeof(GIFTag);
			size--;

			m_q = 1.0f;

			if(index == 2 && path.tag.EOP)
			{
				m_path3hack = 1;
			}

			if(path.tag.PRE)
			{
				GIFReg r;
				r.i64 = path.tag.PRIM;
				(this->*m_fpGIFRegHandlers[GIF_A_D_REG_PRIM])(&r);
			}

			if(path.tag.EOP)
			{
				eop = true;
			}
			else if(path.tag.NLOOP == 0)
			{
				if(index == 0 && m_nloophack)
				{
					continue;
				}

				eop = true;
			}
		}

		switch(path.tag.FLG)
		{
		case GIF_FLG_PACKED:

			for(GIFPackedReg* r = (GIFPackedReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFPackedReg))
			{
				(this->*m_fpGIFPackedRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG) 
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}

			break;

		case GIF_FLG_REGLIST:

			size *= 2;

			for(GIFReg* r = (GIFReg*)mem; path.tag.NLOOP > 0 && size > 0; r++, size--, mem += sizeof(GIFReg))
			{
				(this->*m_fpGIFRegHandlers[path.GetGIFReg()])(r);

				if((path.nreg = (path.nreg + 1) & 0xf) == path.tag.NREG)
				{
					path.nreg = 0; 
					path.tag.NLOOP--;
				}
			}
			
			if(size & 1) mem += sizeof(GIFReg);

			size /= 2;
			
			break;

		case GIF_FLG_IMAGE2: // hmmm

			path.tag.NLOOP = 0;

			break;

		case GIF_FLG_IMAGE:
			{
				int len = min(size, path.tag.NLOOP);

				//ASSERT(!(len&3));

				switch(m_env.TRXDIR.XDIR)
				{
				case 0:
					Write(mem, len*16);
					break;
				case 1: 
					Read(mem, len*16); // TODO: writing access violation with aqtime
					break;
				case 2: 
					Move();
					break;
				case 3: 
					ASSERT(0);
					break;
				default: 
					__assume(0);
				}

				mem += len*16;
				path.tag.NLOOP -= len;
				size -= len;
			}

			break;

		default: 
			__assume(0);
		}

		if(eop && ((int)size <= 0 || index == 0))
		{
			break;
		}
	}

	// FIXME: dq8, pcsx2 error probably

	if(index == 0)
	{
		if(!path.tag.EOP && path.tag.NLOOP > 0)
		{
			path.tag.NLOOP = 0;

			TRACE(_T("path1 hack\n"));
		}
	}
}

int GSState::Freeze(freezeData* fd, bool sizeonly)
{
	if(sizeonly)
	{
		fd->size = m_sssize;
		return 0;
	}
	
	if(!fd->data || fd->size < m_sssize)
	{
		return -1;
	}

	Flush();

	BYTE* data = fd->data;

	memcpy(data, &m_version, sizeof(m_version)); data += sizeof(m_version);
	memcpy(data, &m_env, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(data, &m_v, sizeof(m_v)); data += sizeof(m_v);
	memcpy(data, &m_x, sizeof(m_x)); data += sizeof(m_x);
	memcpy(data, &m_y, sizeof(m_y)); data += sizeof(m_y);
	memcpy(data, m_mem.GetVM(), m_vmsize); data += m_vmsize;
	memcpy(data, m_path, sizeof(m_path)); data += sizeof(m_path);
	memcpy(data, &m_q, sizeof(m_q)); data += sizeof(m_q);

	return 0;
}

int GSState::Defrost(const freezeData* fd)
{
	if(!fd || !fd->data || fd->size == 0) 
	{
		return -1;
	}

	if(fd->size != m_sssize) 
	{
		return -1;
	}

	BYTE* data = fd->data;

	if(*(int*)data != m_version)
	{
		return -1;
	}

	data += sizeof(m_version);

	Flush();

	memcpy(&m_env, data, sizeof(m_env)); data += sizeof(m_env); 
	memcpy(&m_v, data, sizeof(m_v)); data += sizeof(m_v);
	memcpy(&m_x, data, sizeof(m_x)); data += sizeof(m_x);
	memcpy(&m_y, data, sizeof(m_y)); data += sizeof(m_y);
	memcpy(m_mem.GetVM(), data, m_vmsize); data += m_vmsize;
	memcpy(&m_path, data, sizeof(m_path)); data += sizeof(m_path);
	memcpy(&m_q, data, sizeof(m_q)); data += sizeof(m_q);

	PRIM = !m_env.PRMODECONT.AC ? (GIFRegPRIM*)&m_env.PRMODE : &m_env.PRIM;

	m_context = &m_env.CTXT[PRIM->CTXT];

	m_env.CTXT[0].ftbl = &GSLocalMemory::m_psm[m_env.CTXT[0].FRAME.PSM];
	m_env.CTXT[0].ztbl = &GSLocalMemory::m_psm[m_env.CTXT[0].ZBUF.PSM];
	m_env.CTXT[0].ttbl = &GSLocalMemory::m_psm[m_env.CTXT[0].TEX0.PSM];

	m_env.CTXT[1].ftbl = &GSLocalMemory::m_psm[m_env.CTXT[1].FRAME.PSM];
	m_env.CTXT[1].ztbl = &GSLocalMemory::m_psm[m_env.CTXT[1].ZBUF.PSM];
	m_env.CTXT[1].ttbl = &GSLocalMemory::m_psm[m_env.CTXT[1].TEX0.PSM];

m_perfmon.SetFrame(5000);

	return 0;
}

void GSState::SetGameCRC(int crc, int options)
{
	m_crc = crc;
	m_options = options;

	if(m_nloophack_org == 2)
	{
		switch(crc)
		{
		case 0xa39517ab: // ffx pal/eu
		case 0xa39517ae: // ffx pal/fr
		case 0x941bb7d9: // ffx pal/de
		case 0xa39517a9: // ffx pal/it
		case 0x941bb7de: // ffx pal/es
		case 0xb4414ea1: // ffx pal/ru
		case 0xbb3d833a: // ffx ntsc/us
		case 0x6a4efe60: // ffx ntsc/j
		case 0x3866ca7e: // ffx int. ntsc/asia (SLPM-67513, some kind of a asia version) 
		case 0x658597e2: // ffx int. ntsc/j
		case 0x9aac5309: // ffx-2 pal/e
		case 0x9aac530c: // ffx-2 pal/fr
		case 0x9aac530a: // ffx-2 pal/fr? (maybe belgium or luxembourg version)
		case 0x9aac530d: // ffx-2 pal/de
		case 0x9aac530b: // ffx-2 pal/it
		case 0x48fe0c71: // ffx-2 ntsc/us
		case 0xe1fd9a2d: // ffx-2 int+lm ntsc/j
		case 0xf0a6d880: // harvest moon ntsc/us
			m_nloophack = true;
			break;
		}
	}
}

void GSState::SetFrameSkip(int frameskip)
{
	if(m_frameskip != frameskip)
	{
		m_frameskip = frameskip;

		if(frameskip)
		{
		}
		else
		{
		}
	}
}

bool GSState::MakeSnapshot(LPCTSTR path)
{
	CString fn;
	fn.Format(_T("%s_%s.bmp"), path, CTime::GetCurrentTime().Format(_T("%Y%m%d%H%M%S")));
	return false; // TODO: m_dev.SaveCurrent(fn);
}

bool GSState::DetectBadFrame(int& skip)
{
	DWORD FBP = m_context->FRAME.Block();
	DWORD FPSM = m_context->FRAME.PSM;

	bool TME = PRIM->TME;
	DWORD TBP0 = m_context->TEX0.TBP0;
	DWORD TPSM = m_context->TEX0.PSM;

	switch(m_crc)
	{
	case 0x21068223: // okami ntsc/us
	case 0x891f223f: // okami pal/fr

		if(skip == 0)
		{
			if(TME && FBP == 0x00e00 && FPSM == PSM_PSMCT32 && TBP0 == 0x00000 && TPSM == PSM_PSMCT32)
			{
				skip = 1000;
			}
		}
		else
		{
			if(TME && FBP == 0x00e00 && FPSM == PSM_PSMCT32 && TBP0 == 0x03800 && TPSM == PSM_PSMT4)
			{
				skip = 0;
			}
		}

		break;

	case 0x053D2239: // mgs3s1 ntsc/us (TODO: in-game has funny colors)
	case 0x086273D2: // mgs3 snake eater pal/fr
	case 0xAA31B5BF: // mgs3 snake eater 

		if(skip == 0)
		{
			if(TME && FBP == 0x02000 && FPSM == PSM_PSMCT32 && (TBP0 == 0x00000 || TBP0 == 0x01000) && TPSM == PSM_PSMCT24)
			{
				skip = 1000; // 76, 79
			}
			else if(TME && FBP == 0x02800 && FPSM == PSM_PSMCT24 && (TBP0 == 0x00000 || TBP0 == 0x01000) && TPSM == PSM_PSMCT32)
			{
				skip = 1000; // 69
			}
		}
		else 
		{
			if(!TME && (FBP == 0x00000 || FBP == 0x01000) && FPSM == PSM_PSMCT32)
			{
				skip = 0;
			}
		}

		break;

	case 0x278722BF: // dbz bt2 ntsc/us

		if(skip == 0)
		{
			if(TME && /*FBP == 0x00000 && FPSM == PSM_PSMCT16 &&*/ TBP0 == 0x02000 && TPSM == PSM_PSMZ16)
			{
				skip = 27;
			}
		}

		break;

	case 0x72B3802A: // sfex3 ntsc/us
	case 0x71521863: // sfex3 ntsc/us

		if(skip == 0)
		{
			if(TME && FBP == 0x00f00 && FPSM == PSM_PSMCT16 && (TBP0 == 0x00500 || TBP0 == 0x00000) && TPSM == PSM_PSMCT32)
			{
				skip = 4;
			}
		}

		break;

	case 0x28703748: // bully ntsc/us

		if(skip == 0)
		{
			if(TME && (FBP == 0x00000 || FBP == 0x01180) && (TBP0 == 0x00000 || TBP0 == 0x01180) && FBP == TBP0 && FPSM == PSM_PSMCT32 && FPSM == TPSM)
			{
				return false; // allowed for bully
			}

			if(TME && (FBP == 0x00000 || FBP == 0x01180) && FPSM == PSM_PSMCT16S && TBP0 == 0x02300 && TPSM == PSM_PSMZ16S)
			{
				skip = 6;
			}
		}
		else 
		{
			if(!TME && (FBP == 0x00000 || FBP == 0x01180) && FPSM == PSM_PSMCT32)
			{
				skip = 0;
			}
		}

		break;

	case 0xC19A374E: // shadow of the colossus ntsc/us

		if(skip == 0)
		{
			if(TME && FBP == 0x02b80 && FPSM == PSM_PSMCT24 && TBP0 == 0x01e80 && TPSM == PSM_PSMCT24)
			{
				skip = 9;
			}
			else if(TME && FBP == 0x01e80 && FPSM == PSM_PSMCT32 && TBP0 == 0x03880 && TPSM == PSM_PSMCT32)
			{
				skip = 8;
			}
		}

		break;

	case 0x3122B508: // one piece grand adventure ntsc/us

		if(skip == 0)
		{
			if(TME && FBP == 0x02d00 && FPSM == PSM_PSMCT16 && (TBP0 == 0x00000 || TBP0 == 0x00e00) && TPSM == PSM_PSMCT16)
			{
				skip = 3;
			}
		}

		break;

	case 0x6F8545DB: // ICO ntsc/us

		if(skip == 0)
		{
			if(TME && FBP == 0x00800 && FPSM == PSM_PSMCT32 && TBP0 == 0x03d00 && TPSM == PSM_PSMCT32)
			{
				skip = 3;
			}
			else if(TME && FBP == 0x00800 && FPSM == PSM_PSMCT32 && TBP0 == 0x02800 && TPSM == PSM_PSMT8H)
			{
				skip = 56;
			}
		}
		else
		{
			if(TME && TBP0 == 0x00800 && TPSM == PSM_PSMCT32)
			{
				skip = 0;
			}
		}

		break;

	case 0x44A61C8F: // gt4
	case 0x0086E35B: // gt4

		if(skip == 0)
		{
			if(TME && (FBP == 0x03440 || FBP >= 0x03e00) && FPSM == PSM_PSMCT32 && (TBP0 == 0x00000 || TBP0 == 0x01400) && TPSM == PSM_PSMT8)
			{
				skip = 880;
			}
			else if(TME && (FBP == 0x00000 || FBP == 0x01400) && FPSM == PSM_PSMCT24 && TBP0 >= 0x03420 && TPSM == PSM_PSMT8)
			{
				skip = 58;
			}
		}

		break;
	}

	if(skip == 0)
	{
		if(TME)
		{
			if(HasSharedBits(FBP, FPSM, TBP0, TPSM))
			{
				skip = 1;
			}

			// depth textures (bully, mgs3s1 intro)

			if(TPSM == PSM_PSMZ32 || TPSM == PSM_PSMZ24 || TPSM == PSM_PSMZ16 || TPSM == PSM_PSMZ16S)
			{
				// m_perfmon.Put(GSPerfMon::DepthTexture);
				skip = 1;
			}
		}
	}

	if(skip > 0)
	{
		skip--;

		return true;
	}

	return false;
}
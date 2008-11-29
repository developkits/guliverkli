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

#include "StdAfx.h"
#include "GSDrawScanline.h"
#include "GSTextureCacheSW.h"

GSDrawScanline::GSDrawScanline(GSState* state)
	: m_state(state)
	, m_fbo(NULL)
	, m_zbo(NULL)
{
	// w00t :P

	#define InitDS_IIP(iFPSM, iZPSM, iZTST, iIIP) \
		m_ds[iFPSM][iZPSM][iZTST][iIIP] = &GSDrawScanline::DrawScanlineT<iFPSM, iZPSM, iZTST, iIIP>; \

	#define InitDS_ZTST(iFPSM, iZPSM, iZTST) \
		InitDS_IIP(iFPSM, iZPSM, iZTST, 0) \
		InitDS_IIP(iFPSM, iZPSM, iZTST, 1) \

	#define InitDS_ZPSM(iFPSM, iZPSM) \
		InitDS_ZTST(iFPSM, iZPSM, 0) \
		InitDS_ZTST(iFPSM, iZPSM, 1) \
		InitDS_ZTST(iFPSM, iZPSM, 2) \
		InitDS_ZTST(iFPSM, iZPSM, 3) \

	#define InitDS_FPSM(iFPSM) \
		InitDS_ZPSM(iFPSM, 0) \
		InitDS_ZPSM(iFPSM, 1) \
		InitDS_ZPSM(iFPSM, 2) \

	#define InitDS() \
		InitDS_FPSM(0) \
		InitDS_FPSM(1) \
		InitDS_FPSM(2) \

	InitDS();

	InitEx();
}

GSDrawScanline::~GSDrawScanline()
{
	FreeOffsets();
}

// IDrawScanline

void GSDrawScanline::SetupDraw(Vertex* vertices, int count, const void* texture)
{
	GSDrawingEnvironment& env = m_state->m_env;
	GSDrawingContext* context = m_state->m_context;
	GIFRegPRIM* PRIM = m_state->PRIM;

	// m_sel

	m_sel.dw = 0;

	if(PRIM->AA1)
	{
		// TODO: automatic alpha blending (ABE=1, A=0 B=1 C=0 D=1)
	}

	m_sel.fpsm = GSUtil::EncodePSM(context->FRAME.PSM);
	m_sel.zpsm = GSUtil::EncodePSM(context->ZBUF.PSM);
	m_sel.ztst = context->TEST.ZTE && context->TEST.ZTST > 1 ? context->TEST.ZTST : context->ZBUF.ZMSK ? 0 : 1;
	m_sel.iip = PRIM->PRIM == GS_POINTLIST || PRIM->PRIM == GS_SPRITE ? 0 : PRIM->IIP;
	m_sel.tfx = PRIM->TME ? context->TEX0.TFX : 4;

	if(m_sel.tfx != 4)
	{
		m_sel.tcc = context->TEX0.TCC;
		m_sel.fst = PRIM->FST;
		m_sel.ltf = context->TEX1.LCM 
			? (context->TEX1.K <= 0 && (context->TEX1.MMAG & 1) || context->TEX1.K > 0 && (context->TEX1.MMIN & 1)) 
			: ((context->TEX1.MMAG & 1) | (context->TEX1.MMIN & 1));

		if(m_sel.fst == 0 && PRIM->PRIM == GS_SPRITE)
		{
			// skip per pixel division if q is constant

			m_sel.fst = 1;

			for(int i = 0; i < count; i += 2)
			{
				GSVector4 q = vertices[i + 1].t.zzzz();

				vertices[i + 0].t /= q;
				vertices[i + 1].t /= q;
			}
		}

		if(m_sel.fst && m_sel.ltf)
		{
			// if q is constant we can do the half pel shift for bilinear sampling on the vertices

			GSVector4 half(0.5f, 0.5f, 0.0f, 0.0f);

			for(int i = 0; i < count; i++)
			{
				vertices[i].t -= half;
			}
		}
	}

	m_sel.atst = context->TEST.ATE ? context->TEST.ATST : ATST_ALWAYS;
	m_sel.afail = context->TEST.ATE ? context->TEST.AFAIL : 0;
	m_sel.fge = PRIM->FGE;
	m_sel.date = context->FRAME.PSM != PSM_PSMCT24 ? context->TEST.DATE : 0;
	m_sel.abea = PRIM->ABE ? context->ALPHA.A : 3;
	m_sel.abeb = PRIM->ABE ? context->ALPHA.B : 3;
	m_sel.abec = PRIM->ABE ? context->ALPHA.C : 3;
	m_sel.abed = PRIM->ABE ? context->ALPHA.D : 3;
	m_sel.pabe = PRIM->ABE ? env.PABE.PABE : 0;
	m_sel.rfb = m_sel.date || m_sel.abe != 255 || m_sel.atst != 1 && m_sel.afail == 3 || context->FRAME.FBMSK != 0 && context->FRAME.FBMSK != 0xffffffff;
	m_sel.wzb = context->DepthWrite();
	m_sel.tlu = PRIM->TME && GSLocalMemory::m_psm[context->TEX0.PSM].pal > 0 ? 1 : 0;

	m_dsf = m_ds[m_sel.fpsm][m_sel.zpsm][m_sel.ztst][m_sel.iip];

	CRBMap<DWORD, DrawScanlinePtr>::CPair* pair = m_dsmap2.Lookup(m_sel);

	if(pair)
	{
		m_dsf = pair->m_value;
	}
	else
	{
		pair = m_dsmap.Lookup(m_sel);

		if(pair && pair->m_value)
		{
			m_dsf = pair->m_value;

			m_dsmap2.SetAt(pair->m_key, pair->m_value);
		}
		else if(!pair)
		{
			_tprintf(_T("*** [%d] fpsm %d zpsm %d ztst %d tfx %d tcc %d fst %d ltf %d atst %d afail %d fge %d rfb %d date %d abe %d\n"), 
				m_dsmap.GetCount(), 
				m_sel.fpsm, m_sel.zpsm, m_sel.ztst, 
				m_sel.tfx, m_sel.tcc, m_sel.fst, m_sel.ltf, 
				m_sel.atst, m_sel.afail, m_sel.fge, m_sel.rfb, m_sel.date, m_sel.abe);

			m_dsmap.SetAt(m_sel, NULL);

			if(FILE* fp = _tfopen(_T("c:\\1.txt"), _T("w")))
			{
				POSITION pos = m_dsmap.GetHeadPosition();

				while(pos) 
				{
					pair = m_dsmap.GetNext(pos);

					if(!pair->m_value)
					{
						_ftprintf(fp, _T("m_dsmap.SetAt(0x%08x, &GSDrawScanline::DrawScanlineExT<0x%08x>);\n"), pair->m_key, pair->m_key);
					}
				}

				fclose(fp);
			}
		}
	}

	// m_slenv

	SetupOffset(m_fbo, context->FRAME.Block(), context->FRAME.FBW, context->FRAME.PSM);
	SetupOffset(m_zbo, context->ZBUF.Block(), context->FRAME.FBW, context->ZBUF.PSM);

	m_slenv.vm = m_state->m_mem.m_vm32;
	m_slenv.fbr = m_fbo->row;
	m_slenv.zbr = m_zbo->row;
	m_slenv.fbc = m_fbo->col;
	m_slenv.zbc = m_zbo->col;
	m_slenv.fm = GSVector4i(context->FRAME.FBMSK);
	m_slenv.zm = GSVector4i(context->ZBUF.ZMSK ? 0xffffffff : 0);
	m_slenv.datm = GSVector4i(context->TEST.DATM ? 0x80000000 : 0);
	m_slenv.colclamp = GSVector4i(env.COLCLAMP.CLAMP ? 0xffffffff : 0x00ff00ff);
	m_slenv.fba = GSVector4i(context->FBA.FBA ? 0x80000000 : 0);
	m_slenv.aref = GSVector4i((int)context->TEST.AREF + (m_sel.atst == ATST_LESS ? -1 : m_sel.atst == ATST_GREATER ? +1 : 0));
	m_slenv.afix = GSVector4((float)(int)context->ALPHA.FIX);
	m_slenv.afix2 = m_slenv.afix * (2.0f / 256);
	m_slenv.fc = GSVector4((DWORD)env.FOGCOL.ai32[0]);

	if(m_sel.fpsm == 1)
	{
		m_slenv.fm |= GSVector4i::xff000000();
	}

	if(PRIM->TME)
	{
		const GSTextureCacheSW::GSTexture* t = (const GSTextureCacheSW::GSTexture*)texture;

		m_slenv.tex = t->m_buff;
		m_slenv.clut = m_state->m_mem.m_clut;
		m_slenv.tw = t->m_tw;

		short tw = (short)(1 << context->TEX0.TW);
		short th = (short)(1 << context->TEX0.TH);

		switch(context->CLAMP.WMS)
		{
		case CLAMP_REPEAT: 
			m_slenv.t.min.u16[0] = tw - 1;
			m_slenv.t.max.u16[0] = 0;
			m_slenv.t.mask.u32[0] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_slenv.t.min.u16[0] = 0;
			m_slenv.t.max.u16[0] = tw - 1;
			m_slenv.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_slenv.t.min.u16[0] = context->CLAMP.MINU;
			m_slenv.t.max.u16[0] = context->CLAMP.MAXU;
			m_slenv.t.mask.u32[0] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_slenv.t.min.u16[0] = context->CLAMP.MINU;
			m_slenv.t.max.u16[0] = context->CLAMP.MAXU;
			m_slenv.t.mask.u32[0] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		switch(context->CLAMP.WMT)
		{
		case CLAMP_REPEAT: 
			m_slenv.t.min.u16[4] = th - 1;
			m_slenv.t.max.u16[4] = 0;
			m_slenv.t.mask.u32[2] = 0xffffffff; 
			break;
		case CLAMP_CLAMP: 
			m_slenv.t.min.u16[4] = 0;
			m_slenv.t.max.u16[4] = th - 1;
			m_slenv.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_REPEAT: 
			m_slenv.t.min.u16[4] = context->CLAMP.MINV;
			m_slenv.t.max.u16[4] = context->CLAMP.MAXV;
			m_slenv.t.mask.u32[2] = 0; 
			break;
		case CLAMP_REGION_CLAMP: 
			m_slenv.t.min.u16[4] = context->CLAMP.MINV;
			m_slenv.t.max.u16[4] = context->CLAMP.MAXV;
			m_slenv.t.mask.u32[2] = 0xffffffff; 
			break;
		default: 
			__assume(0);
		}

		m_slenv.t.min = m_slenv.t.min.xxxxl().xxxxh();
		m_slenv.t.max = m_slenv.t.max.xxxxl().xxxxh();
		m_slenv.t.mask = m_slenv.t.mask.xxzz();
	}
}

void GSDrawScanline::DrawScanline(int top, int left, int right, const Vertex& v, const Vertex& dv)
{
	(this->*m_dsf)(top, left, right, v, dv);
}

void GSDrawScanline::FillRect(const GSVector4i& r, const Vertex& v)
{
	ASSERT(r.y >= 0);
	ASSERT(r.w >= 0);

	GSDrawingContext* context = m_state->m_context;

	DWORD fbp = context->FRAME.Block();
	DWORD fpsm = context->FRAME.PSM;
	DWORD zbp = context->ZBUF.Block();
	DWORD zpsm = context->ZBUF.PSM;
	DWORD bw = context->FRAME.FBW;

	if(!context->ZBUF.ZMSK)
	{
		m_state->m_mem.FillRect(r, (DWORD)(float)v.p.z, zpsm, zbp, bw);
	}

	DWORD c = v.c.rgba32();

	if(context->FBA.FBA)
	{
		c |= 0x80000000;
	}
	
	if(fpsm == PSM_PSMCT16 || fpsm == PSM_PSMCT16S)
	{
		c = ((c & 0xf8) >> 3) | ((c & 0xf800) >> 6) | ((c & 0xf80000) >> 9) | ((c & 0x80000000) >> 16);
	}

	m_state->m_mem.FillRect(r, c, fpsm, fbp, bw);

	// m_slenv.steps += r.Width() * r.Height();
}

void GSDrawScanline::SetupOffset(Offset*& o, DWORD bp, DWORD bw, DWORD psm)
{
	if(bw == 0) {ASSERT(0); return;}

	DWORD hash = bp | (bw << 14) | (psm << 20);

	if(!o || o->hash != hash)
	{
		CRBMap<DWORD, Offset*>::CPair* pair = m_omap.Lookup(hash);

		if(pair)
		{
			o = pair->m_value;
		}
		else
		{
			o = (Offset*)_aligned_malloc(sizeof(Offset), 16);

			o->hash = hash;

			GSLocalMemory::pixelAddress pa = GSLocalMemory::m_psm[psm].pa;

			for(int i = 0, j = 1024; i < j; i++)
			{
				o->row[i] = GSVector4i((int)pa(0, i, bp, bw));
			}

			int* p = (int*)_aligned_malloc(sizeof(int) * (2048 + 3) * 4, 16);

			for(int i = 0; i < 4; i++)
			{
				o->col[i] = &p[2048 * i + ((4 - (i & 3)) & 3)];

				memcpy(o->col[i], GSLocalMemory::m_psm[psm].rowOffset[0], sizeof(int) * 2048);
			}

			m_omap.SetAt(hash, o);
		}
	}
}

void GSDrawScanline::FreeOffsets()
{
	POSITION pos = m_omap.GetHeadPosition();

	while(pos)
	{
		Offset* o = m_omap.GetNextValue(pos);

		for(int i = 0; i < countof(o->col); i++)
		{
			_aligned_free(o->col);
		}

		_aligned_free(o);
	}

	m_omap.RemoveAll();
}

template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
void GSDrawScanline::DrawScanlineT(int top, int left, int right, const Vertex& v, const Vertex& dv)	
{
	GSVector4i fa_base = m_slenv.fbr[top];
	GSVector4i* fa_offset = (GSVector4i*)&m_slenv.fbc[left & 3][left];

	GSVector4i za_base = m_slenv.zbr[top];
	GSVector4i* za_offset = (GSVector4i*)&m_slenv.zbc[left & 3][left];

	GSVector4 ps0123 = GSVector4::ps0123();

	GSVector4 vp = v.p;
	GSVector4 dp = dv.p;
	GSVector4 z = vp.zzzz(); z += dp.zzzz() * ps0123;
	GSVector4 f = vp.wwww(); f += dp.wwww() * ps0123;

	GSVector4 vt = v.t;
	GSVector4 dt = dv.t;
	GSVector4 s = vt.xxxx(); s += dt.xxxx() * ps0123;
	GSVector4 t = vt.yyyy(); t += dt.yyyy() * ps0123;
	GSVector4 q = vt.zzzz(); q += dt.zzzz() * ps0123;

	GSVector4 vc = v.c;
	GSVector4 dc = dv.c;
	GSVector4 r = vc.xxxx(); if(iip) r += dc.xxxx() * ps0123;
	GSVector4 g = vc.yyyy(); if(iip) g += dc.yyyy() * ps0123;
	GSVector4 b = vc.zzzz(); if(iip) b += dc.zzzz() * ps0123;
	GSVector4 a = vc.wwww(); if(iip) a += dc.wwww() * ps0123;

	int steps = right - left;

	while(1)
	{
		do
		{
			GSVector4i za = za_base + GSVector4i::load<true>(za_offset);
			
			GSVector4i zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001(za));

			GSVector4i test;

			if(!TestZ(zpsm, ztst, zs, za, test))
			{
				continue;
			}

			int pixels = GSVector4i::store(GSVector4i::load(steps).min_i16(GSVector4i::load(4)));

			GSVector4 c[12];

			if(m_sel.tfx != TFX_NONE)
			{
				GSVector4 u = s;
				GSVector4 v = t;

				if(!m_sel.fst)
				{
					GSVector4 w = q.rcp();

					u *= w;
					v *= w;

					if(m_sel.ltf)
					{
						u -= 0.5f;
						v -= 0.5f;
					}
				}

				SampleTexture(pixels, ztst, m_sel.ltf, m_sel.tlu, test, u, v, c);
			}

			AlphaTFX(m_sel.tfx, m_sel.tcc, a, c[3]);

			GSVector4i fm = m_slenv.fm;
			GSVector4i zm = m_slenv.zm;

			if(!TestAlpha(m_sel.atst, m_sel.afail, c[3], fm, zm, test))
			{
				continue;
			}

			ColorTFX(m_sel.tfx, r, g, b, a, c[0], c[1], c[2]);

			if(m_sel.fge)
			{
				Fog(f, c[0], c[1], c[2]);
			}

			GSVector4i fa = fa_base + GSVector4i::load<true>(fa_offset);

			GSVector4i d = GSVector4i::zero();

			if(m_sel.rfb)
			{
				d = ReadFrameX(fpsm == 1 ? 0 : fpsm, fa);

				if(fpsm != 1 && m_sel.date)
				{
					test |= (d ^ m_slenv.datm).sra32(31);

					if(test.alltrue())
					{
						continue;
					}
				}
			}

			fm |= test;
			zm |= test;

			if(m_sel.abe != 255)
			{
	//			GSVector4::expand(d, c[4], c[5], c[6], c[7]);

				c[4] = (d << 24) >> 24;
				c[5] = (d << 16) >> 24;
				c[6] = (d <<  8) >> 24;
				c[7] = (d >> 24);

				if(fpsm == 1)
				{
					c[7] = GSVector4(128.0f);
				}

				c[8] = GSVector4::zero();
				c[9] = GSVector4::zero();
				c[10] = GSVector4::zero();
				c[11] = m_slenv.afix;

				DWORD abea = m_sel.abea;
				DWORD abeb = m_sel.abeb;
				DWORD abec = m_sel.abec;
				DWORD abed = m_sel.abed;

				GSVector4 r = (c[abea*4 + 0] - c[abeb*4 + 0]).mod2x(c[abec*4 + 3]) + c[abed*4 + 0];
				GSVector4 g = (c[abea*4 + 1] - c[abeb*4 + 1]).mod2x(c[abec*4 + 3]) + c[abed*4 + 1];
				GSVector4 b = (c[abea*4 + 2] - c[abeb*4 + 2]).mod2x(c[abec*4 + 3]) + c[abed*4 + 2];

				if(m_sel.pabe)
				{
					GSVector4 mask = c[3] >= GSVector4(128.0f);

					c[0] = c[0].blend8(r, mask);
					c[1] = c[1].blend8(g, mask);
					c[2] = c[2].blend8(b, mask);
				}
				else
				{
					c[0] = r;
					c[1] = g;
					c[2] = b;
				}
			}

			GSVector4i rb = GSVector4i(c[0]).ps32(GSVector4i(c[2]));
			GSVector4i ga = GSVector4i(c[1]).ps32(GSVector4i(c[3]));
			
			GSVector4i rg = rb.upl16(ga) & m_slenv.colclamp;
			GSVector4i ba = rb.uph16(ga) & m_slenv.colclamp;
			
			GSVector4i s = rg.upl32(ba).pu16(rg.uph32(ba));

			if(fpsm != 1)
			{
				s |= m_slenv.fba;
			}

			if(m_sel.rfb)
			{
				s = s.blend(d, fm);
			}

			WriteFrameAndZBufX(fpsm, fa, fm, s, ztst > 0 ? zpsm : 3, za, zm, zs, pixels);
		}
		while(0);

		if(steps <= 4) break;

		steps -= 4;

		fa_offset++;
		za_offset++;

		GSVector4 dp4 = dv.p * 4.0f;

		z += dp4.zzzz();
		f += dp4.wwww();

		GSVector4 dt4 = dv.t * 4.0f;

		s += dt4.xxxx();
		t += dt4.yyyy();
		q += dt4.zzzz();

		GSVector4 dc4 = dv.c * 4.0;

		if(iip) r += dc4.xxxx();
		if(iip) g += dc4.yyyy();
		if(iip) b += dc4.zzzz();
		if(iip) a += dc4.wwww();
	}
}

GSVector4i GSDrawScanline::Wrap(const GSVector4i& t)
{
	GSVector4i clamp = t.sat_i16(m_slenv.t.min, m_slenv.t.max);
	GSVector4i repeat = (t & m_slenv.t.min) | m_slenv.t.max;

	return clamp.blend8(repeat, m_slenv.t.mask);
}

void GSDrawScanline::SampleTexture(int pixels, DWORD ztst, DWORD ltf, DWORD tlu, const GSVector4i& test, const GSVector4& u, const GSVector4& v, GSVector4* c)
{
	const void* RESTRICT tex = m_slenv.tex;
	const DWORD* RESTRICT clut = m_slenv.clut;
	const DWORD tw = m_slenv.tw;

	if(ltf)
	{
		GSVector4 uf = u.floor();
		GSVector4 vf = v.floor();
		
		GSVector4 uff = u - uf;
		GSVector4 vff = v - vf;

		GSVector4i uv = GSVector4i(uf).ps32(GSVector4i(vf));

		GSVector4i uv0 = Wrap(uv);
		GSVector4i uv1 = Wrap(uv + GSVector4i::x0001(uv));

		int i = 0;

		if(tlu)
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				GSVector4 c00 = GSVector4(clut[((const BYTE*)tex)[(uv0.u16[i + 4] << tw) + uv0.u16[i]]]);
				GSVector4 c01 = GSVector4(clut[((const BYTE*)tex)[(uv0.u16[i + 4] << tw) + uv1.u16[i]]]);
				GSVector4 c10 = GSVector4(clut[((const BYTE*)tex)[(uv1.u16[i + 4] << tw) + uv0.u16[i]]]);
				GSVector4 c11 = GSVector4(clut[((const BYTE*)tex)[(uv1.u16[i + 4] << tw) + uv1.u16[i]]]);

				c00 = c00.lerp(c01, uff.v[i]);
				c10 = c10.lerp(c11, uff.v[i]);
				c00 = c00.lerp(c10, vff.v[i]);

				c[i] = c00;

			}
			while(++i < pixels);
		}
		else
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				GSVector4 c00 = GSVector4(((const DWORD*)tex)[(uv0.u16[i + 4] << tw) + uv0.u16[i]]);
				GSVector4 c01 = GSVector4(((const DWORD*)tex)[(uv0.u16[i + 4] << tw) + uv1.u16[i]]);
				GSVector4 c10 = GSVector4(((const DWORD*)tex)[(uv1.u16[i + 4] << tw) + uv0.u16[i]]);
				GSVector4 c11 = GSVector4(((const DWORD*)tex)[(uv1.u16[i + 4] << tw) + uv1.u16[i]]);

				c00 = c00.lerp(c01, uff.v[i]);
				c10 = c10.lerp(c11, uff.v[i]);
				c00 = c00.lerp(c10, vff.v[i]);

				c[i] = c00;
			}
			while(++i < pixels);
		}

		GSVector4::transpose(c[0], c[1], c[2], c[3]);
	}
	else
	{
		GSVector4i uv = Wrap(GSVector4i(u).ps32(GSVector4i(v)));

		GSVector4i c00;

		int i = 0;

		if(tlu)
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				c00.u32[i] = clut[((const BYTE*)tex)[(uv.u16[i + 4] << tw) + uv.u16[i]]];
			}
			while(++i < pixels);
		}
		else
		{
			do
			{
				if(ztst > 1 && test.u32[i])
				{
					continue;
				}

				c00.u32[i] = ((const DWORD*)tex)[(uv.u16[i + 4] << tw) + uv.u16[i]];
			}
			while(++i < pixels);
		}

		// GSVector4::expand(c00, c[0], c[1], c[2], c[3]);

		c[0] = (c00 << 24) >> 24;
		c[1] = (c00 << 16) >> 24;
		c[2] = (c00 <<  8) >> 24;
		c[3] = (c00 >> 24);
	}
}

void GSDrawScanline::ColorTFX(DWORD tfx, const GSVector4& rf, const GSVector4& gf, const GSVector4& bf, const GSVector4& af, GSVector4& rt, GSVector4& gt, GSVector4& bt)
{
	switch(tfx)
	{
	case TFX_MODULATE:
		rt = rt.mod2x(rf);
		gt = gt.mod2x(gf);
		bt = bt.mod2x(bf);
		rt = rt.clamp();
		gt = gt.clamp();
		bt = bt.clamp();
		break;
	case TFX_DECAL:
		break;
	case TFX_HIGHLIGHT:
	case TFX_HIGHLIGHT2:
		rt = rt.mod2x(rf) + af;
		gt = gt.mod2x(gf) + af;
		bt = bt.mod2x(bf) + af;
		rt = rt.clamp();
		gt = gt.clamp();
		bt = bt.clamp();
		break;
	case TFX_NONE:
		rt = rf;
		gt = gf;
		bt = bf;
		break;
	default:
		__assume(0);
	}
}

void GSDrawScanline::AlphaTFX(DWORD tfx, DWORD tcc, const GSVector4& af, GSVector4& at)
{
	switch(tfx)
	{
	case TFX_MODULATE: at = tcc ? at.mod2x(af).clamp() : af; break;
	case TFX_DECAL: break;
	case TFX_HIGHLIGHT: at = tcc ? (at + af).clamp() : af; break;
	case TFX_HIGHLIGHT2: if(!tcc) at = af; break;
	case TFX_NONE: at = af; break; 
	default: __assume(0);
	}
}

void GSDrawScanline::Fog(const GSVector4& f, GSVector4& r, GSVector4& g, GSVector4& b)
{
	GSVector4 fc = m_slenv.fc;

	r = fc.xxxx().lerp(r, f);
	g = fc.yyyy().lerp(g, f);
	b = fc.zzzz().lerp(b, f);
}

bool GSDrawScanline::TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& za, GSVector4i& test)
{
	if(ztst > 1)
	{
		GSVector4i zd = ReadZBufX(zpsm, za);

		GSVector4i zso = zs;
		GSVector4i zdo = zd;

		if(zpsm == 0)
		{
			GSVector4i o = GSVector4i::x80000000(zs);

			zso = zs - o;
			zdo = zd - o;
		}

		switch(ztst)
		{
		case ZTST_GEQUAL: test = zso < zdo; break;
		case ZTST_GREATER: test = zso <= zdo; break;
		default: __assume(0);
		}

		if(test.alltrue())
		{
			return false;
		}
	}
	else
	{
		test = GSVector4i::zero();
	}

	return true;
}

bool GSDrawScanline::TestAlpha(DWORD atst, DWORD afail, const GSVector4& a, GSVector4i& fm, GSVector4i& zm, GSVector4i& test)
{
	if(atst != 1)
	{
		GSVector4i t;

		switch(atst)
		{
		case ATST_NEVER: t = GSVector4i::invzero(); break;
		case ATST_ALWAYS: t = GSVector4i::zero(); break;
		case ATST_LESS: 
		case ATST_LEQUAL: t = GSVector4i(a) > m_slenv.aref; break;
		case ATST_EQUAL: t = GSVector4i(a) != m_slenv.aref; break;
		case ATST_GEQUAL: 
		case ATST_GREATER: t = GSVector4i(a) < m_slenv.aref; break;
		case ATST_NOTEQUAL: t = GSVector4i(a) == m_slenv.aref; break;
		default: __assume(0);
		}

		switch(afail)
		{
		case AFAIL_KEEP:
			fm |= t;
			zm |= t;
			test |= t;
			if(test.alltrue()) return false;
			break;
		case AFAIL_FB_ONLY:
			zm |= t;
			break;
		case AFAIL_ZB_ONLY:
			fm |= t;
			break;
		case AFAIL_RGB_ONLY: 
			fm |= t & GSVector4i::xff000000(t);
			zm |= t;
			break;
		default: 
			__assume(0);
		}
	}

	return true;
}

DWORD GSDrawScanline::ReadPixel32(DWORD* RESTRICT vm, DWORD addr)
{
	return vm[addr];
}

DWORD GSDrawScanline::ReadPixel24(DWORD* RESTRICT vm, DWORD addr)
{
	return vm[addr] & 0x00ffffff;
}

DWORD GSDrawScanline::ReadPixel16(WORD* RESTRICT vm, DWORD addr)
{
	return (DWORD)vm[addr];
}

void GSDrawScanline::WritePixel32(DWORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = c;
}

void GSDrawScanline::WritePixel24(DWORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = (vm[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSDrawScanline::WritePixel16(WORD* RESTRICT vm, DWORD addr, DWORD c) 
{
	vm[addr] = (WORD)c;
}

GSVector4i GSDrawScanline::ReadFrameX(int psm, const GSVector4i& addr) const
{
	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i c, r, g, b, a;

	switch(psm)
	{
	case 0:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm32);
		#else
		c = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		break;
	case 1:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm32);
		#else
		c = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		c = (c & GSVector4i::x00ffffff(addr)) | GSVector4i::x80000000(addr);
		break;
	case 2:
		#if _M_SSE >= 0x401
		c = addr.gather32_32(vm16);
		#else
		c = GSVector4i(
			ReadPixel16(vm16, addr.u32[0]),
			ReadPixel16(vm16, addr.u32[1]),
			ReadPixel16(vm16, addr.u32[2]),
			ReadPixel16(vm16, addr.u32[3]));
		#endif
		c = ((c & 0x8000) << 16) | ((c & 0x7c00) << 9) | ((c & 0x03e0) << 6) | ((c & 0x001f) << 3); 
		break;
	default: 
		ASSERT(0); 
		c = GSVector4i::zero();
	}
	
	return c;
}

GSVector4i GSDrawScanline::ReadZBufX(int psm, const GSVector4i& addr) const
{
	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i z;

	switch(psm)
	{
	case 0: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm32);
		#else
		z = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		break;
	case 1: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm32);
		#else
		z = GSVector4i(
			ReadPixel32(vm32, addr.u32[0]),
			ReadPixel32(vm32, addr.u32[1]),
			ReadPixel32(vm32, addr.u32[2]),
			ReadPixel32(vm32, addr.u32[3]));
		#endif
		z = z & GSVector4i::x00ffffff(addr);
		break;
	case 2: 
		#if _M_SSE >= 0x401
		z = addr.gather32_32(vm16);
		#else
		z = GSVector4i(
			ReadPixel16(vm16, addr.u32[0]),
			ReadPixel16(vm16, addr.u32[1]),
			ReadPixel16(vm16, addr.u32[2]),
			ReadPixel16(vm16, addr.u32[3]));
		#endif
		break;
	default: 
		ASSERT(0); 
		z = GSVector4i::zero();
	}

	return z;
}

void GSDrawScanline::WriteFrameAndZBufX(
	int fpsm, const GSVector4i& fa, const GSVector4i& fm, const GSVector4i& f, 
	int zpsm, const GSVector4i& za, const GSVector4i& zm, const GSVector4i& z, 
	int pixels)
{
	// FIXME: compiler problem or not enough xmm regs in x86 mode to store the address regs (fa, za)

	DWORD* RESTRICT vm32 = (DWORD*)m_slenv.vm;
	WORD* RESTRICT vm16 = (WORD*)m_slenv.vm;

	GSVector4i c = f;

	if(fpsm == 2)
	{
		GSVector4i rb = c & 0x00f800f8;
		GSVector4i ga = c & 0x8000f800;
		c = (ga >> 16) | (rb >> 9) | (ga >> 6) | (rb >> 3);
	}

	#if _M_SSE >= 0x401

	if(fm.extract32<0>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[0], c.extract32<0>()); break;
		case 1: WritePixel24(vm32, fa.u32[0], c.extract32<0>()); break;
		case 2: WritePixel16(vm16, fa.u32[0], c.extract16<0 * 2>()); break;
		}
	}

	if(zm.extract32<0>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[0], z.extract32<0>()); break;
		case 1: WritePixel24(vm32, za.u32[0], z.extract32<0>()); break;
		case 2: WritePixel16(vm16, za.u32[0], z.extract16<0 * 2>()); break;
		}
	}

	if(pixels <= 1) return;

	if(fm.extract32<1>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[1], c.extract32<1>()); break;
		case 1: WritePixel24(vm32, fa.u32[1], c.extract32<1>()); break;
		case 2: WritePixel16(vm16, fa.u32[1], c.extract16<1 * 2>()); break;
		}
	}

	if(zm.extract32<1>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[1], z.extract32<1>()); break;
		case 1: WritePixel24(vm32, za.u32[1], z.extract32<1>()); break;
		case 2: WritePixel16(vm16, za.u32[1], z.extract16<1 * 2>()); break;
		}
	}

	if(pixels <= 2) return;

	if(fm.extract32<2>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[2], c.extract32<2>()); break;
		case 1: WritePixel24(vm32, fa.u32[2], c.extract32<2>()); break;
		case 2: WritePixel16(vm16, fa.u32[2], c.extract16<2 * 2>()); break;
		}
	}

	if(zm.extract32<2>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[2], z.extract32<2>()); break;
		case 1: WritePixel24(vm32, za.u32[2], z.extract32<2>()); break;
		case 2: WritePixel16(vm16, za.u32[2], z.extract16<2 * 2>()); break;
		}
	}

	if(pixels <= 3) return;

	if(fm.extract32<3>() != 0xffffffff) 
	{
		switch(fpsm)
		{
		case 0: WritePixel32(vm32, fa.u32[3], c.extract32<3>()); break;
		case 1: WritePixel24(vm32, fa.u32[3], c.extract32<3>()); break;
		case 2: WritePixel16(vm16, fa.u32[3], c.extract16<3 * 2>()); break;
		}
	}

	if(zm.extract32<3>() != 0xffffffff) 
	{
		switch(zpsm)
		{
		case 0: WritePixel32(vm32, za.u32[3], z.extract32<3>()); break;
		case 1: WritePixel24(vm32, za.u32[3], z.extract32<3>()); break;
		case 2: WritePixel16(vm16, za.u32[3], z.extract16<3 * 2>()); break;
		}
	}

	#else

	int i = 0;

	do
	{
		if(fm.u32[i] != 0xffffffff)
		{
			switch(fpsm)
			{
			case 0: WritePixel32(vm32, fa.u32[i], c.u32[i]);  break;
			case 1: WritePixel24(vm32, fa.u32[i], c.u32[i]);  break;
			case 2: WritePixel16(vm16, fa.u32[i], c.u16[i * 2]);  break;
			}
		}

		if(zm.u32[i] != 0xffffffff) 
		{
			switch(zpsm)
			{
			case 0: WritePixel32(vm32, za.u32[i], z.u32[i]);  break;
			case 1: WritePixel24(vm32, za.u32[i], z.u32[i]);  break;
			case 2: WritePixel16(vm16, za.u32[i], z.u16[i * 2]);  break;
			}
		}
	}
	while(++i < pixels);

	#endif
}

//

void GSDrawScanline::InitEx()
{
	// ffx

	m_dsmap.SetAt(0x2420c265, &GSDrawScanline::DrawScanlineExT<0x2420c265>);
	m_dsmap.SetAt(0x2420e245, &GSDrawScanline::DrawScanlineExT<0x2420e245>);
	m_dsmap.SetAt(0x24402205, &GSDrawScanline::DrawScanlineExT<0x24402205>);
	m_dsmap.SetAt(0x24402245, &GSDrawScanline::DrawScanlineExT<0x24402245>);
	m_dsmap.SetAt(0x24402275, &GSDrawScanline::DrawScanlineExT<0x24402275>);
	m_dsmap.SetAt(0x2440c265, &GSDrawScanline::DrawScanlineExT<0x2440c265>);
	m_dsmap.SetAt(0x2440fc05, &GSDrawScanline::DrawScanlineExT<0x2440fc05>);
	m_dsmap.SetAt(0x2440fc45, &GSDrawScanline::DrawScanlineExT<0x2440fc45>);
	m_dsmap.SetAt(0x24411045, &GSDrawScanline::DrawScanlineExT<0x24411045>);
	m_dsmap.SetAt(0x24411075, &GSDrawScanline::DrawScanlineExT<0x24411075>);
	m_dsmap.SetAt(0x2444d465, &GSDrawScanline::DrawScanlineExT<0x2444d465>);
	m_dsmap.SetAt(0x24610265, &GSDrawScanline::DrawScanlineExT<0x24610265>);
	m_dsmap.SetAt(0x24802245, &GSDrawScanline::DrawScanlineExT<0x24802245>);
	m_dsmap.SetAt(0x24802275, &GSDrawScanline::DrawScanlineExT<0x24802275>);
	m_dsmap.SetAt(0x2480c265, &GSDrawScanline::DrawScanlineExT<0x2480c265>);
	m_dsmap.SetAt(0x2480e245, &GSDrawScanline::DrawScanlineExT<0x2480e245>);
	m_dsmap.SetAt(0x26402275, &GSDrawScanline::DrawScanlineExT<0x26402275>);
	m_dsmap.SetAt(0x2880c205, &GSDrawScanline::DrawScanlineExT<0x2880c205>);
	m_dsmap.SetAt(0x4ff02215, &GSDrawScanline::DrawScanlineExT<0x4ff02215>);
	m_dsmap.SetAt(0x4ff02896, &GSDrawScanline::DrawScanlineExT<0x4ff02896>);
	m_dsmap.SetAt(0x4ff0d475, &GSDrawScanline::DrawScanlineExT<0x4ff0d475>);
	m_dsmap.SetAt(0x4ff4c275, &GSDrawScanline::DrawScanlineExT<0x4ff4c275>);
	m_dsmap.SetAt(0x4ff4d475, &GSDrawScanline::DrawScanlineExT<0x4ff4d475>);
	m_dsmap.SetAt(0x64402215, &GSDrawScanline::DrawScanlineExT<0x64402215>);
	m_dsmap.SetAt(0x64402255, &GSDrawScanline::DrawScanlineExT<0x64402255>);
	m_dsmap.SetAt(0x64402815, &GSDrawScanline::DrawScanlineExT<0x64402815>);
	m_dsmap.SetAt(0x6440e215, &GSDrawScanline::DrawScanlineExT<0x6440e215>);
	m_dsmap.SetAt(0x6440e255, &GSDrawScanline::DrawScanlineExT<0x6440e255>);
	m_dsmap.SetAt(0x6440fc15, &GSDrawScanline::DrawScanlineExT<0x6440fc15>);
	m_dsmap.SetAt(0x6444c265, &GSDrawScanline::DrawScanlineExT<0x6444c265>);
	m_dsmap.SetAt(0x6444d465, &GSDrawScanline::DrawScanlineExT<0x6444d465>);
	m_dsmap.SetAt(0x66402275, &GSDrawScanline::DrawScanlineExT<0x66402275>);
	m_dsmap.SetAt(0x664038a5, &GSDrawScanline::DrawScanlineExT<0x664038a5>);
	m_dsmap.SetAt(0x6a902215, &GSDrawScanline::DrawScanlineExT<0x6a902215>);
	m_dsmap.SetAt(0xa420dc05, &GSDrawScanline::DrawScanlineExT<0xa420dc05>);
	m_dsmap.SetAt(0xa440d405, &GSDrawScanline::DrawScanlineExT<0xa440d405>);
	m_dsmap.SetAt(0xa440d425, &GSDrawScanline::DrawScanlineExT<0xa440d425>);
	m_dsmap.SetAt(0xa440d465, &GSDrawScanline::DrawScanlineExT<0xa440d465>);
	m_dsmap.SetAt(0xa440dc05, &GSDrawScanline::DrawScanlineExT<0xa440dc05>);
	m_dsmap.SetAt(0xa440fc05, &GSDrawScanline::DrawScanlineExT<0xa440fc05>);
	m_dsmap.SetAt(0xa444d465, &GSDrawScanline::DrawScanlineExT<0xa444d465>);
	m_dsmap.SetAt(0xa480d425, &GSDrawScanline::DrawScanlineExT<0xa480d425>);
	m_dsmap.SetAt(0xa480d465, &GSDrawScanline::DrawScanlineExT<0xa480d465>);
	m_dsmap.SetAt(0xa6803435, &GSDrawScanline::DrawScanlineExT<0xa6803435>);
	m_dsmap.SetAt(0xa680e425, &GSDrawScanline::DrawScanlineExT<0xa680e425>);
	m_dsmap.SetAt(0xcff03435, &GSDrawScanline::DrawScanlineExT<0xcff03435>);
	m_dsmap.SetAt(0xcff0d415, &GSDrawScanline::DrawScanlineExT<0xcff0d415>);
	m_dsmap.SetAt(0xcff0d475, &GSDrawScanline::DrawScanlineExT<0xcff0d475>);
	m_dsmap.SetAt(0xcff4d475, &GSDrawScanline::DrawScanlineExT<0xcff4d475>);
	m_dsmap.SetAt(0xe440d465, &GSDrawScanline::DrawScanlineExT<0xe440d465>);
	m_dsmap.SetAt(0xe440ec55, &GSDrawScanline::DrawScanlineExT<0xe440ec55>);
	m_dsmap.SetAt(0xe440fc15, &GSDrawScanline::DrawScanlineExT<0xe440fc15>);
	m_dsmap.SetAt(0xe440fc25, &GSDrawScanline::DrawScanlineExT<0xe440fc25>);
	m_dsmap.SetAt(0xe440fc55, &GSDrawScanline::DrawScanlineExT<0xe440fc55>);
	m_dsmap.SetAt(0xe444d465, &GSDrawScanline::DrawScanlineExT<0xe444d465>);
	m_dsmap.SetAt(0xe444fc75, &GSDrawScanline::DrawScanlineExT<0xe444fc75>);
	m_dsmap.SetAt(0xe484fc75, &GSDrawScanline::DrawScanlineExT<0xe484fc75>);
	m_dsmap.SetAt(0xe680e425, &GSDrawScanline::DrawScanlineExT<0xe680e425>);
	m_dsmap.SetAt(0x66402895, &GSDrawScanline::DrawScanlineExT<0x66402895>);
	m_dsmap.SetAt(0xa480d405, &GSDrawScanline::DrawScanlineExT<0xa480d405>);
	m_dsmap.SetAt(0xa480dc05, &GSDrawScanline::DrawScanlineExT<0xa480dc05>);
	m_dsmap.SetAt(0x24202245, &GSDrawScanline::DrawScanlineExT<0x24202245>);
	m_dsmap.SetAt(0xa440fc45, &GSDrawScanline::DrawScanlineExT<0xa440fc45>);

	// ffxii

	m_dsmap.SetAt(0x0ff02204, &GSDrawScanline::DrawScanlineExT<0x0ff02204>);
	m_dsmap.SetAt(0x0ff11c04, &GSDrawScanline::DrawScanlineExT<0x0ff11c04>);
	m_dsmap.SetAt(0x0ff11c14, &GSDrawScanline::DrawScanlineExT<0x0ff11c14>);
	m_dsmap.SetAt(0x24210224, &GSDrawScanline::DrawScanlineExT<0x24210224>);
	m_dsmap.SetAt(0x24410214, &GSDrawScanline::DrawScanlineExT<0x24410214>);
	m_dsmap.SetAt(0x24410254, &GSDrawScanline::DrawScanlineExT<0x24410254>);
	m_dsmap.SetAt(0x24411464, &GSDrawScanline::DrawScanlineExT<0x24411464>);
	m_dsmap.SetAt(0x24411c04, &GSDrawScanline::DrawScanlineExT<0x24411c04>);
	m_dsmap.SetAt(0x24431054, &GSDrawScanline::DrawScanlineExT<0x24431054>);
	m_dsmap.SetAt(0x244b0224, &GSDrawScanline::DrawScanlineExT<0x244b0224>);
	m_dsmap.SetAt(0x24810224, &GSDrawScanline::DrawScanlineExT<0x24810224>);
	m_dsmap.SetAt(0x24810264, &GSDrawScanline::DrawScanlineExT<0x24810264>);
	m_dsmap.SetAt(0x24811464, &GSDrawScanline::DrawScanlineExT<0x24811464>);
	m_dsmap.SetAt(0x24830254, &GSDrawScanline::DrawScanlineExT<0x24830254>);
	m_dsmap.SetAt(0x24831c04, &GSDrawScanline::DrawScanlineExT<0x24831c04>);
	m_dsmap.SetAt(0x25830214, &GSDrawScanline::DrawScanlineExT<0x25830214>);
	m_dsmap.SetAt(0x2a882204, &GSDrawScanline::DrawScanlineExT<0x2a882204>);
	m_dsmap.SetAt(0x2ff02224, &GSDrawScanline::DrawScanlineExT<0x2ff02224>);
	m_dsmap.SetAt(0x2ff30204, &GSDrawScanline::DrawScanlineExT<0x2ff30204>);
	m_dsmap.SetAt(0x2ff82204, &GSDrawScanline::DrawScanlineExT<0x2ff82204>);
	m_dsmap.SetAt(0x4ff02214, &GSDrawScanline::DrawScanlineExT<0x4ff02214>);
	m_dsmap.SetAt(0x4ff20214, &GSDrawScanline::DrawScanlineExT<0x4ff20214>);
	m_dsmap.SetAt(0x4ff4a264, &GSDrawScanline::DrawScanlineExT<0x4ff4a264>);
	m_dsmap.SetAt(0x64403054, &GSDrawScanline::DrawScanlineExT<0x64403054>);
	m_dsmap.SetAt(0x8ff11454, &GSDrawScanline::DrawScanlineExT<0x8ff11454>);
	m_dsmap.SetAt(0xa4411424, &GSDrawScanline::DrawScanlineExT<0xa4411424>);
	m_dsmap.SetAt(0xa4411454, &GSDrawScanline::DrawScanlineExT<0xa4411454>);
	m_dsmap.SetAt(0xa4411464, &GSDrawScanline::DrawScanlineExT<0xa4411464>);
	m_dsmap.SetAt(0xa4411c14, &GSDrawScanline::DrawScanlineExT<0xa4411c14>);
	m_dsmap.SetAt(0xa4411c54, &GSDrawScanline::DrawScanlineExT<0xa4411c54>);
	m_dsmap.SetAt(0xa4430c14, &GSDrawScanline::DrawScanlineExT<0xa4430c14>);
	m_dsmap.SetAt(0xa4431054, &GSDrawScanline::DrawScanlineExT<0xa4431054>);
	m_dsmap.SetAt(0xa4431464, &GSDrawScanline::DrawScanlineExT<0xa4431464>);
	m_dsmap.SetAt(0xa4431c14, &GSDrawScanline::DrawScanlineExT<0xa4431c14>);
	m_dsmap.SetAt(0xa4431c54, &GSDrawScanline::DrawScanlineExT<0xa4431c54>);
	m_dsmap.SetAt(0xa4811424, &GSDrawScanline::DrawScanlineExT<0xa4811424>);
	m_dsmap.SetAt(0xa4811464, &GSDrawScanline::DrawScanlineExT<0xa4811464>);
	m_dsmap.SetAt(0xa4830c14, &GSDrawScanline::DrawScanlineExT<0xa4830c14>);
	m_dsmap.SetAt(0xa4831464, &GSDrawScanline::DrawScanlineExT<0xa4831464>);
	m_dsmap.SetAt(0xa4911424, &GSDrawScanline::DrawScanlineExT<0xa4911424>);
	m_dsmap.SetAt(0xa4911464, &GSDrawScanline::DrawScanlineExT<0xa4911464>);
	m_dsmap.SetAt(0xcff4a464, &GSDrawScanline::DrawScanlineExT<0xcff4a464>);
	m_dsmap.SetAt(0xcff4b464, &GSDrawScanline::DrawScanlineExT<0xcff4b464>);
	m_dsmap.SetAt(0xe4203054, &GSDrawScanline::DrawScanlineExT<0xe4203054>);
	m_dsmap.SetAt(0xe445b464, &GSDrawScanline::DrawScanlineExT<0xe445b464>);
	m_dsmap.SetAt(0xe445d464, &GSDrawScanline::DrawScanlineExT<0xe445d464>);
	m_dsmap.SetAt(0xe485b464, &GSDrawScanline::DrawScanlineExT<0xe485b464>);

	// kingdom hearts

	m_dsmap.SetAt(0x0ff02205, &GSDrawScanline::DrawScanlineExT<0x0ff02205>);
	m_dsmap.SetAt(0x0ff03c04, &GSDrawScanline::DrawScanlineExT<0x0ff03c04>);
	m_dsmap.SetAt(0x0ff03c05, &GSDrawScanline::DrawScanlineExT<0x0ff03c05>);
	m_dsmap.SetAt(0x24403c04, &GSDrawScanline::DrawScanlineExT<0x24403c04>);
	m_dsmap.SetAt(0x2440d824, &GSDrawScanline::DrawScanlineExT<0x2440d824>);
	m_dsmap.SetAt(0x28102204, &GSDrawScanline::DrawScanlineExT<0x28102204>);
	m_dsmap.SetAt(0x2ff03c00, &GSDrawScanline::DrawScanlineExT<0x2ff03c00>);
	m_dsmap.SetAt(0x62a4d468, &GSDrawScanline::DrawScanlineExT<0x62a4d468>);
	m_dsmap.SetAt(0x6420c254, &GSDrawScanline::DrawScanlineExT<0x6420c254>);
	m_dsmap.SetAt(0x64402c14, &GSDrawScanline::DrawScanlineExT<0x64402c14>);
	m_dsmap.SetAt(0x6440c214, &GSDrawScanline::DrawScanlineExT<0x6440c214>);
	m_dsmap.SetAt(0x6440c254, &GSDrawScanline::DrawScanlineExT<0x6440c254>);
	m_dsmap.SetAt(0x6440d464, &GSDrawScanline::DrawScanlineExT<0x6440d464>);
	m_dsmap.SetAt(0x6480c254, &GSDrawScanline::DrawScanlineExT<0x6480c254>);
	m_dsmap.SetAt(0x6ff02218, &GSDrawScanline::DrawScanlineExT<0x6ff02218>);
	m_dsmap.SetAt(0x6ff0c228, &GSDrawScanline::DrawScanlineExT<0x6ff0c228>);
	m_dsmap.SetAt(0xa440d434, &GSDrawScanline::DrawScanlineExT<0xa440d434>);
	m_dsmap.SetAt(0xa440d474, &GSDrawScanline::DrawScanlineExT<0xa440d474>);
	m_dsmap.SetAt(0xa440dc04, &GSDrawScanline::DrawScanlineExT<0xa440dc04>);
	m_dsmap.SetAt(0xa4445464, &GSDrawScanline::DrawScanlineExT<0xa4445464>);
	m_dsmap.SetAt(0xa444d464, &GSDrawScanline::DrawScanlineExT<0xa444d464>);
	m_dsmap.SetAt(0xa480d434, &GSDrawScanline::DrawScanlineExT<0xa480d434>);
	m_dsmap.SetAt(0xa480d474, &GSDrawScanline::DrawScanlineExT<0xa480d474>);
	m_dsmap.SetAt(0xa480dc34, &GSDrawScanline::DrawScanlineExT<0xa480dc34>);
	m_dsmap.SetAt(0xa484d474, &GSDrawScanline::DrawScanlineExT<0xa484d474>);
	m_dsmap.SetAt(0xcff4d464, &GSDrawScanline::DrawScanlineExT<0xcff4d464>);
	m_dsmap.SetAt(0xe4402c14, &GSDrawScanline::DrawScanlineExT<0xe4402c14>);
	m_dsmap.SetAt(0xe4402c54, &GSDrawScanline::DrawScanlineExT<0xe4402c54>);
	m_dsmap.SetAt(0xe4403414, &GSDrawScanline::DrawScanlineExT<0xe4403414>);
	m_dsmap.SetAt(0xe4403c14, &GSDrawScanline::DrawScanlineExT<0xe4403c14>);
	m_dsmap.SetAt(0xe4403c54, &GSDrawScanline::DrawScanlineExT<0xe4403c54>);
	m_dsmap.SetAt(0xe440dc14, &GSDrawScanline::DrawScanlineExT<0xe440dc14>);
	m_dsmap.SetAt(0xe440dc54, &GSDrawScanline::DrawScanlineExT<0xe440dc54>);
	m_dsmap.SetAt(0xe444b464, &GSDrawScanline::DrawScanlineExT<0xe444b464>);
	m_dsmap.SetAt(0xe444b468, &GSDrawScanline::DrawScanlineExT<0xe444b468>);
	m_dsmap.SetAt(0xe444d464, &GSDrawScanline::DrawScanlineExT<0xe444d464>);
	m_dsmap.SetAt(0xe4802c14, &GSDrawScanline::DrawScanlineExT<0xe4802c14>);
	m_dsmap.SetAt(0xe480dc14, &GSDrawScanline::DrawScanlineExT<0xe480dc14>);
	m_dsmap.SetAt(0xe480dc54, &GSDrawScanline::DrawScanlineExT<0xe480dc54>);

	// dbzbt3

	m_dsmap.SetAt(0x24402204, &GSDrawScanline::DrawScanlineExT<0x24402204>);
	m_dsmap.SetAt(0x24402884, &GSDrawScanline::DrawScanlineExT<0x24402884>);
	m_dsmap.SetAt(0x24402c84, &GSDrawScanline::DrawScanlineExT<0x24402c84>);
	m_dsmap.SetAt(0x24403804, &GSDrawScanline::DrawScanlineExT<0x24403804>);
	m_dsmap.SetAt(0x24403884, &GSDrawScanline::DrawScanlineExT<0x24403884>);
	m_dsmap.SetAt(0x2440fc84, &GSDrawScanline::DrawScanlineExT<0x2440fc84>);
	m_dsmap.SetAt(0x2448e464, &GSDrawScanline::DrawScanlineExT<0x2448e464>);
	m_dsmap.SetAt(0x24802264, &GSDrawScanline::DrawScanlineExT<0x24802264>);
	m_dsmap.SetAt(0x24882204, &GSDrawScanline::DrawScanlineExT<0x24882204>);
	m_dsmap.SetAt(0x25202204, &GSDrawScanline::DrawScanlineExT<0x25202204>);
	m_dsmap.SetAt(0x25403804, &GSDrawScanline::DrawScanlineExT<0x25403804>);
	m_dsmap.SetAt(0x26402204, &GSDrawScanline::DrawScanlineExT<0x26402204>);
	m_dsmap.SetAt(0x26402206, &GSDrawScanline::DrawScanlineExT<0x26402206>);
	m_dsmap.SetAt(0x26402244, &GSDrawScanline::DrawScanlineExT<0x26402244>);
	m_dsmap.SetAt(0x26403804, &GSDrawScanline::DrawScanlineExT<0x26403804>);
	m_dsmap.SetAt(0x2640fc84, &GSDrawScanline::DrawScanlineExT<0x2640fc84>);
	m_dsmap.SetAt(0x26802204, &GSDrawScanline::DrawScanlineExT<0x26802204>);
	m_dsmap.SetAt(0x26803804, &GSDrawScanline::DrawScanlineExT<0x26803804>);
	m_dsmap.SetAt(0x2ff02c86, &GSDrawScanline::DrawScanlineExT<0x2ff02c86>);
	m_dsmap.SetAt(0x64402214, &GSDrawScanline::DrawScanlineExT<0x64402214>);
	m_dsmap.SetAt(0xa4402c84, &GSDrawScanline::DrawScanlineExT<0xa4402c84>);
	m_dsmap.SetAt(0xa4402c86, &GSDrawScanline::DrawScanlineExT<0xa4402c86>);
	m_dsmap.SetAt(0xa4402cb4, &GSDrawScanline::DrawScanlineExT<0xa4402cb4>);
	m_dsmap.SetAt(0xa4403424, &GSDrawScanline::DrawScanlineExT<0xa4403424>);
	m_dsmap.SetAt(0xa4403464, &GSDrawScanline::DrawScanlineExT<0xa4403464>);
	m_dsmap.SetAt(0xa4403c04, &GSDrawScanline::DrawScanlineExT<0xa4403c04>);
	m_dsmap.SetAt(0xa440b464, &GSDrawScanline::DrawScanlineExT<0xa440b464>);
	m_dsmap.SetAt(0xa440bc04, &GSDrawScanline::DrawScanlineExT<0xa440bc04>);
	m_dsmap.SetAt(0xa440d424, &GSDrawScanline::DrawScanlineExT<0xa440d424>);
	m_dsmap.SetAt(0xa440d464, &GSDrawScanline::DrawScanlineExT<0xa440d464>);
	m_dsmap.SetAt(0xa440fc04, &GSDrawScanline::DrawScanlineExT<0xa440fc04>);
	m_dsmap.SetAt(0xa443bc04, &GSDrawScanline::DrawScanlineExT<0xa443bc04>);
	m_dsmap.SetAt(0xa4483464, &GSDrawScanline::DrawScanlineExT<0xa4483464>);
	m_dsmap.SetAt(0xa4483c04, &GSDrawScanline::DrawScanlineExT<0xa4483c04>);
	m_dsmap.SetAt(0xa4802c84, &GSDrawScanline::DrawScanlineExT<0xa4802c84>);
	m_dsmap.SetAt(0xa4803464, &GSDrawScanline::DrawScanlineExT<0xa4803464>);
	m_dsmap.SetAt(0xa4883c04, &GSDrawScanline::DrawScanlineExT<0xa4883c04>);
	m_dsmap.SetAt(0xa5803c04, &GSDrawScanline::DrawScanlineExT<0xa5803c04>);
	m_dsmap.SetAt(0xa6202c86, &GSDrawScanline::DrawScanlineExT<0xa6202c86>);
	m_dsmap.SetAt(0xe4403464, &GSDrawScanline::DrawScanlineExT<0xe4403464>);
	m_dsmap.SetAt(0xe440b464, &GSDrawScanline::DrawScanlineExT<0xe440b464>);
	m_dsmap.SetAt(0xe4419464, &GSDrawScanline::DrawScanlineExT<0xe4419464>);
	m_dsmap.SetAt(0xe58034e4, &GSDrawScanline::DrawScanlineExT<0xe58034e4>);
	m_dsmap.SetAt(0xe62034e4, &GSDrawScanline::DrawScanlineExT<0xe62034e4>);
	m_dsmap.SetAt(0xe6403464, &GSDrawScanline::DrawScanlineExT<0xe6403464>);

	// xenosaga

	m_dsmap.SetAt(0x0ff10234, &GSDrawScanline::DrawScanlineExT<0x0ff10234>);
	m_dsmap.SetAt(0x24402234, &GSDrawScanline::DrawScanlineExT<0x24402234>);
	m_dsmap.SetAt(0x24402264, &GSDrawScanline::DrawScanlineExT<0x24402264>);
	m_dsmap.SetAt(0x24402274, &GSDrawScanline::DrawScanlineExT<0x24402274>);
	m_dsmap.SetAt(0x24403464, &GSDrawScanline::DrawScanlineExT<0x24403464>);
	m_dsmap.SetAt(0x24403834, &GSDrawScanline::DrawScanlineExT<0x24403834>);
	m_dsmap.SetAt(0x24407464, &GSDrawScanline::DrawScanlineExT<0x24407464>);
	m_dsmap.SetAt(0x24430264, &GSDrawScanline::DrawScanlineExT<0x24430264>);
	m_dsmap.SetAt(0x2480c264, &GSDrawScanline::DrawScanlineExT<0x2480c264>);
	m_dsmap.SetAt(0x26408434, &GSDrawScanline::DrawScanlineExT<0x26408434>);
	m_dsmap.SetAt(0x26802264, &GSDrawScanline::DrawScanlineExT<0x26802264>);
	m_dsmap.SetAt(0x26810214, &GSDrawScanline::DrawScanlineExT<0x26810214>);
	m_dsmap.SetAt(0x2ff10234, &GSDrawScanline::DrawScanlineExT<0x2ff10234>);
	m_dsmap.SetAt(0x4ff02c34, &GSDrawScanline::DrawScanlineExT<0x4ff02c34>);
	m_dsmap.SetAt(0x4ff0dc34, &GSDrawScanline::DrawScanlineExT<0x4ff0dc34>);
	m_dsmap.SetAt(0x60003c14, &GSDrawScanline::DrawScanlineExT<0x60003c14>);
	m_dsmap.SetAt(0x6000c214, &GSDrawScanline::DrawScanlineExT<0x6000c214>);
	m_dsmap.SetAt(0x64803c14, &GSDrawScanline::DrawScanlineExT<0x64803c14>);
	m_dsmap.SetAt(0x6600c274, &GSDrawScanline::DrawScanlineExT<0x6600c274>);
	m_dsmap.SetAt(0x66403c14, &GSDrawScanline::DrawScanlineExT<0x66403c14>);
	m_dsmap.SetAt(0x66803464, &GSDrawScanline::DrawScanlineExT<0x66803464>);
	m_dsmap.SetAt(0x6680b464, &GSDrawScanline::DrawScanlineExT<0x6680b464>);
	m_dsmap.SetAt(0x6a902224, &GSDrawScanline::DrawScanlineExT<0x6a902224>);
	m_dsmap.SetAt(0x6ff02264, &GSDrawScanline::DrawScanlineExT<0x6ff02264>);
	m_dsmap.SetAt(0x6ff0b464, &GSDrawScanline::DrawScanlineExT<0x6ff0b464>);
	m_dsmap.SetAt(0xa420d424, &GSDrawScanline::DrawScanlineExT<0xa420d424>);
	m_dsmap.SetAt(0xa4402c34, &GSDrawScanline::DrawScanlineExT<0xa4402c34>);
	m_dsmap.SetAt(0xa4407464, &GSDrawScanline::DrawScanlineExT<0xa4407464>);
	m_dsmap.SetAt(0xa4410c14, &GSDrawScanline::DrawScanlineExT<0xa4410c14>);
	m_dsmap.SetAt(0xa480d424, &GSDrawScanline::DrawScanlineExT<0xa480d424>);
	m_dsmap.SetAt(0xa6203464, &GSDrawScanline::DrawScanlineExT<0xa6203464>);
	m_dsmap.SetAt(0xa6803464, &GSDrawScanline::DrawScanlineExT<0xa6803464>);
	m_dsmap.SetAt(0xa8911434, &GSDrawScanline::DrawScanlineExT<0xa8911434>);
	m_dsmap.SetAt(0xe000cc14, &GSDrawScanline::DrawScanlineExT<0xe000cc14>);
	m_dsmap.SetAt(0xe000cc24, &GSDrawScanline::DrawScanlineExT<0xe000cc24>);
	m_dsmap.SetAt(0xe000cc34, &GSDrawScanline::DrawScanlineExT<0xe000cc34>);
	m_dsmap.SetAt(0xe110cc34, &GSDrawScanline::DrawScanlineExT<0xe110cc34>);
	m_dsmap.SetAt(0xe290cc14, &GSDrawScanline::DrawScanlineExT<0xe290cc14>);
	m_dsmap.SetAt(0xe440cc14, &GSDrawScanline::DrawScanlineExT<0xe440cc14>);
	m_dsmap.SetAt(0xe440cc24, &GSDrawScanline::DrawScanlineExT<0xe440cc24>);
	m_dsmap.SetAt(0xe440cc34, &GSDrawScanline::DrawScanlineExT<0xe440cc34>);
	m_dsmap.SetAt(0xe440d424, &GSDrawScanline::DrawScanlineExT<0xe440d424>);
	m_dsmap.SetAt(0xe6803464, &GSDrawScanline::DrawScanlineExT<0xe6803464>);
	m_dsmap.SetAt(0xe680b464, &GSDrawScanline::DrawScanlineExT<0xe680b464>);
	m_dsmap.SetAt(0xe680cc14, &GSDrawScanline::DrawScanlineExT<0xe680cc14>);
	m_dsmap.SetAt(0xeff03464, &GSDrawScanline::DrawScanlineExT<0xeff03464>);
	m_dsmap.SetAt(0xeff0b464, &GSDrawScanline::DrawScanlineExT<0xeff0b464>);
	m_dsmap.SetAt(0x0ff10c04, &GSDrawScanline::DrawScanlineExT<0x0ff10c04>);
	m_dsmap.SetAt(0x26430c34, &GSDrawScanline::DrawScanlineExT<0x26430c34>);
	m_dsmap.SetAt(0x4ff03c34, &GSDrawScanline::DrawScanlineExT<0x4ff03c34>);
	m_dsmap.SetAt(0x6440c264, &GSDrawScanline::DrawScanlineExT<0x6440c264>);
	m_dsmap.SetAt(0x64420214, &GSDrawScanline::DrawScanlineExT<0x64420214>);
	m_dsmap.SetAt(0x6ff03c34, &GSDrawScanline::DrawScanlineExT<0x6ff03c34>);
	m_dsmap.SetAt(0xe440cc64, &GSDrawScanline::DrawScanlineExT<0xe440cc64>);

	// xenosaga 2

	m_dsmap.SetAt(0x0ff10204, &GSDrawScanline::DrawScanlineExT<0x0ff10204>);
	m_dsmap.SetAt(0x0ff42244, &GSDrawScanline::DrawScanlineExT<0x0ff42244>);
	m_dsmap.SetAt(0x24445464, &GSDrawScanline::DrawScanlineExT<0x24445464>);
	m_dsmap.SetAt(0x26803c04, &GSDrawScanline::DrawScanlineExT<0x26803c04>);
	m_dsmap.SetAt(0x28903464, &GSDrawScanline::DrawScanlineExT<0x28903464>);
	m_dsmap.SetAt(0x2ff02204, &GSDrawScanline::DrawScanlineExT<0x2ff02204>);
	m_dsmap.SetAt(0x2ff03c04, &GSDrawScanline::DrawScanlineExT<0x2ff03c04>);
	m_dsmap.SetAt(0x2ff90204, &GSDrawScanline::DrawScanlineExT<0x2ff90204>);
	m_dsmap.SetAt(0x4ff02c14, &GSDrawScanline::DrawScanlineExT<0x4ff02c14>);
	m_dsmap.SetAt(0x4ff42264, &GSDrawScanline::DrawScanlineExT<0x4ff42264>);
	m_dsmap.SetAt(0x4ff43464, &GSDrawScanline::DrawScanlineExT<0x4ff43464>);
	m_dsmap.SetAt(0x4ff4b464, &GSDrawScanline::DrawScanlineExT<0x4ff4b464>);
	m_dsmap.SetAt(0x64403c14, &GSDrawScanline::DrawScanlineExT<0x64403c14>);
	m_dsmap.SetAt(0x66202214, &GSDrawScanline::DrawScanlineExT<0x66202214>);
	m_dsmap.SetAt(0x66402214, &GSDrawScanline::DrawScanlineExT<0x66402214>);
	m_dsmap.SetAt(0x66802214, &GSDrawScanline::DrawScanlineExT<0x66802214>);
	m_dsmap.SetAt(0x8ff43444, &GSDrawScanline::DrawScanlineExT<0x8ff43444>);
	m_dsmap.SetAt(0xa4405464, &GSDrawScanline::DrawScanlineExT<0xa4405464>);
	m_dsmap.SetAt(0xa4443464, &GSDrawScanline::DrawScanlineExT<0xa4443464>);
	m_dsmap.SetAt(0xa4a13404, &GSDrawScanline::DrawScanlineExT<0xa4a13404>);
	m_dsmap.SetAt(0xa520d424, &GSDrawScanline::DrawScanlineExT<0xa520d424>);
	m_dsmap.SetAt(0xa540d424, &GSDrawScanline::DrawScanlineExT<0xa540d424>);
	m_dsmap.SetAt(0xa580d424, &GSDrawScanline::DrawScanlineExT<0xa580d424>);
	m_dsmap.SetAt(0xcff03464, &GSDrawScanline::DrawScanlineExT<0xcff03464>);
	m_dsmap.SetAt(0xcff43464, &GSDrawScanline::DrawScanlineExT<0xcff43464>);
	m_dsmap.SetAt(0xe000dc34, &GSDrawScanline::DrawScanlineExT<0xe000dc34>);
	m_dsmap.SetAt(0xe440dc34, &GSDrawScanline::DrawScanlineExT<0xe440dc34>);
	m_dsmap.SetAt(0xe4803464, &GSDrawScanline::DrawScanlineExT<0xe4803464>);
	m_dsmap.SetAt(0xe4803c14, &GSDrawScanline::DrawScanlineExT<0xe4803c14>);

	// tales of abyss

	m_dsmap.SetAt(0x0ff02208, &GSDrawScanline::DrawScanlineExT<0x0ff02208>);
	m_dsmap.SetAt(0x0ff03c88, &GSDrawScanline::DrawScanlineExT<0x0ff03c88>);
	m_dsmap.SetAt(0x2091a048, &GSDrawScanline::DrawScanlineExT<0x2091a048>);
	m_dsmap.SetAt(0x20a02248, &GSDrawScanline::DrawScanlineExT<0x20a02248>);
	m_dsmap.SetAt(0x20a1b448, &GSDrawScanline::DrawScanlineExT<0x20a1b448>);
	m_dsmap.SetAt(0x24402248, &GSDrawScanline::DrawScanlineExT<0x24402248>);
	m_dsmap.SetAt(0x24442268, &GSDrawScanline::DrawScanlineExT<0x24442268>);
	m_dsmap.SetAt(0x25403c88, &GSDrawScanline::DrawScanlineExT<0x25403c88>);
	m_dsmap.SetAt(0x2680d448, &GSDrawScanline::DrawScanlineExT<0x2680d448>);
	m_dsmap.SetAt(0x2a803448, &GSDrawScanline::DrawScanlineExT<0x2a803448>);
	m_dsmap.SetAt(0x2a803c08, &GSDrawScanline::DrawScanlineExT<0x2a803c08>);
	m_dsmap.SetAt(0x2a805448, &GSDrawScanline::DrawScanlineExT<0x2a805448>);
	m_dsmap.SetAt(0x2a80d448, &GSDrawScanline::DrawScanlineExT<0x2a80d448>);
	m_dsmap.SetAt(0x2a81b448, &GSDrawScanline::DrawScanlineExT<0x2a81b448>);
	m_dsmap.SetAt(0x2a81b468, &GSDrawScanline::DrawScanlineExT<0x2a81b468>);
	m_dsmap.SetAt(0x2ff02208, &GSDrawScanline::DrawScanlineExT<0x2ff02208>);
	m_dsmap.SetAt(0x2ff03c88, &GSDrawScanline::DrawScanlineExT<0x2ff03c88>);
	m_dsmap.SetAt(0x4ff02218, &GSDrawScanline::DrawScanlineExT<0x4ff02218>);
	m_dsmap.SetAt(0x60a02268, &GSDrawScanline::DrawScanlineExT<0x60a02268>);
	m_dsmap.SetAt(0x60a1b468, &GSDrawScanline::DrawScanlineExT<0x60a1b468>);
	m_dsmap.SetAt(0x6445b458, &GSDrawScanline::DrawScanlineExT<0x6445b458>);
	m_dsmap.SetAt(0xa441b448, &GSDrawScanline::DrawScanlineExT<0xa441b448>);
	m_dsmap.SetAt(0xa441b468, &GSDrawScanline::DrawScanlineExT<0xa441b468>);
	m_dsmap.SetAt(0xa445b448, &GSDrawScanline::DrawScanlineExT<0xa445b448>);
	m_dsmap.SetAt(0xa445b468, &GSDrawScanline::DrawScanlineExT<0xa445b468>);
	m_dsmap.SetAt(0xa464f468, &GSDrawScanline::DrawScanlineExT<0xa464f468>);
	m_dsmap.SetAt(0xa481b448, &GSDrawScanline::DrawScanlineExT<0xa481b448>);
	m_dsmap.SetAt(0xa481b468, &GSDrawScanline::DrawScanlineExT<0xa481b468>);
	m_dsmap.SetAt(0xa484f468, &GSDrawScanline::DrawScanlineExT<0xa484f468>);
	m_dsmap.SetAt(0xa485b468, &GSDrawScanline::DrawScanlineExT<0xa485b468>);
	m_dsmap.SetAt(0xaff02cb8, &GSDrawScanline::DrawScanlineExT<0xaff02cb8>);
	m_dsmap.SetAt(0xe441b468, &GSDrawScanline::DrawScanlineExT<0xe441b468>);
	m_dsmap.SetAt(0xe4443468, &GSDrawScanline::DrawScanlineExT<0xe4443468>);
	m_dsmap.SetAt(0xe445a468, &GSDrawScanline::DrawScanlineExT<0xe445a468>);
	m_dsmap.SetAt(0xe445b468, &GSDrawScanline::DrawScanlineExT<0xe445b468>);
	m_dsmap.SetAt(0xe481b468, &GSDrawScanline::DrawScanlineExT<0xe481b468>);
	m_dsmap.SetAt(0xe485b468, &GSDrawScanline::DrawScanlineExT<0xe485b468>);

	// persona 4

	m_dsmap.SetAt(0x0ff1a248, &GSDrawScanline::DrawScanlineExT<0x0ff1a248>);
	m_dsmap.SetAt(0x24210248, &GSDrawScanline::DrawScanlineExT<0x24210248>);
	m_dsmap.SetAt(0x2441a268, &GSDrawScanline::DrawScanlineExT<0x2441a268>);
	m_dsmap.SetAt(0x2441b468, &GSDrawScanline::DrawScanlineExT<0x2441b468>);
	m_dsmap.SetAt(0x24810248, &GSDrawScanline::DrawScanlineExT<0x24810248>);
	m_dsmap.SetAt(0x24811048, &GSDrawScanline::DrawScanlineExT<0x24811048>);
	m_dsmap.SetAt(0x24842268, &GSDrawScanline::DrawScanlineExT<0x24842268>);
	m_dsmap.SetAt(0x25411048, &GSDrawScanline::DrawScanlineExT<0x25411048>);
	m_dsmap.SetAt(0x2541b048, &GSDrawScanline::DrawScanlineExT<0x2541b048>);
	m_dsmap.SetAt(0x26842068, &GSDrawScanline::DrawScanlineExT<0x26842068>);
	m_dsmap.SetAt(0x2ff82248, &GSDrawScanline::DrawScanlineExT<0x2ff82248>);
	m_dsmap.SetAt(0x4ff42268, &GSDrawScanline::DrawScanlineExT<0x4ff42268>);
	m_dsmap.SetAt(0x64402268, &GSDrawScanline::DrawScanlineExT<0x64402268>);
	m_dsmap.SetAt(0x64442268, &GSDrawScanline::DrawScanlineExT<0x64442268>);
	m_dsmap.SetAt(0x6445a268, &GSDrawScanline::DrawScanlineExT<0x6445a268>);
	m_dsmap.SetAt(0x64802268, &GSDrawScanline::DrawScanlineExT<0x64802268>);
	m_dsmap.SetAt(0x66842068, &GSDrawScanline::DrawScanlineExT<0x66842068>);
	m_dsmap.SetAt(0xa4251468, &GSDrawScanline::DrawScanlineExT<0xa4251468>);
	m_dsmap.SetAt(0xa4411448, &GSDrawScanline::DrawScanlineExT<0xa4411448>);
	m_dsmap.SetAt(0xa4411468, &GSDrawScanline::DrawScanlineExT<0xa4411468>);
	m_dsmap.SetAt(0xa4811468, &GSDrawScanline::DrawScanlineExT<0xa4811468>);
	m_dsmap.SetAt(0xa4851468, &GSDrawScanline::DrawScanlineExT<0xa4851468>);
	m_dsmap.SetAt(0xa541b448, &GSDrawScanline::DrawScanlineExT<0xa541b448>);
	m_dsmap.SetAt(0xa541b468, &GSDrawScanline::DrawScanlineExT<0xa541b468>);
	m_dsmap.SetAt(0xa6843468, &GSDrawScanline::DrawScanlineExT<0xa6843468>);
	m_dsmap.SetAt(0xe441a468, &GSDrawScanline::DrawScanlineExT<0xe441a468>);
	m_dsmap.SetAt(0xe441d468, &GSDrawScanline::DrawScanlineExT<0xe441d468>);
	m_dsmap.SetAt(0xe443b468, &GSDrawScanline::DrawScanlineExT<0xe443b468>);
	m_dsmap.SetAt(0xe447b468, &GSDrawScanline::DrawScanlineExT<0xe447b468>);
	m_dsmap.SetAt(0xe6843468, &GSDrawScanline::DrawScanlineExT<0xe6843468>);

	// ffx-2

	m_dsmap.SetAt(0x20002806, &GSDrawScanline::DrawScanlineExT<0x20002806>);
	m_dsmap.SetAt(0x24402805, &GSDrawScanline::DrawScanlineExT<0x24402805>);
	m_dsmap.SetAt(0x2440c245, &GSDrawScanline::DrawScanlineExT<0x2440c245>);
	m_dsmap.SetAt(0x2440c445, &GSDrawScanline::DrawScanlineExT<0x2440c445>);
	m_dsmap.SetAt(0x2440d065, &GSDrawScanline::DrawScanlineExT<0x2440d065>);
	m_dsmap.SetAt(0x2440d805, &GSDrawScanline::DrawScanlineExT<0x2440d805>);
	m_dsmap.SetAt(0x24410215, &GSDrawScanline::DrawScanlineExT<0x24410215>);
	m_dsmap.SetAt(0x24411c19, &GSDrawScanline::DrawScanlineExT<0x24411c19>);
	m_dsmap.SetAt(0x24490214, &GSDrawScanline::DrawScanlineExT<0x24490214>);
	m_dsmap.SetAt(0x24491854, &GSDrawScanline::DrawScanlineExT<0x24491854>);
	m_dsmap.SetAt(0x244b1814, &GSDrawScanline::DrawScanlineExT<0x244b1814>);
	m_dsmap.SetAt(0x2480c225, &GSDrawScanline::DrawScanlineExT<0x2480c225>);
	m_dsmap.SetAt(0x2a902205, &GSDrawScanline::DrawScanlineExT<0x2a902205>);
	m_dsmap.SetAt(0x2ff02805, &GSDrawScanline::DrawScanlineExT<0x2ff02805>);
	m_dsmap.SetAt(0x2ff0c245, &GSDrawScanline::DrawScanlineExT<0x2ff0c245>);
	m_dsmap.SetAt(0x4ff03829, &GSDrawScanline::DrawScanlineExT<0x4ff03829>);
	m_dsmap.SetAt(0x4ff0c215, &GSDrawScanline::DrawScanlineExT<0x4ff0c215>);
	m_dsmap.SetAt(0x6440d065, &GSDrawScanline::DrawScanlineExT<0x6440d065>);
	m_dsmap.SetAt(0x6440d815, &GSDrawScanline::DrawScanlineExT<0x6440d815>);
	m_dsmap.SetAt(0x6440e225, &GSDrawScanline::DrawScanlineExT<0x6440e225>);
	m_dsmap.SetAt(0x6440e275, &GSDrawScanline::DrawScanlineExT<0x6440e275>);
	m_dsmap.SetAt(0x64420215, &GSDrawScanline::DrawScanlineExT<0x64420215>);
	m_dsmap.SetAt(0x64802219, &GSDrawScanline::DrawScanlineExT<0x64802219>);
	m_dsmap.SetAt(0x64802259, &GSDrawScanline::DrawScanlineExT<0x64802259>);
	m_dsmap.SetAt(0x64802269, &GSDrawScanline::DrawScanlineExT<0x64802269>);
	m_dsmap.SetAt(0x6480c215, &GSDrawScanline::DrawScanlineExT<0x6480c215>);
	m_dsmap.SetAt(0x66402215, &GSDrawScanline::DrawScanlineExT<0x66402215>);
	m_dsmap.SetAt(0x6880c265, &GSDrawScanline::DrawScanlineExT<0x6880c265>);
	m_dsmap.SetAt(0x6880d065, &GSDrawScanline::DrawScanlineExT<0x6880d065>);
	m_dsmap.SetAt(0xa420d425, &GSDrawScanline::DrawScanlineExT<0xa420d425>);
	m_dsmap.SetAt(0xa420d465, &GSDrawScanline::DrawScanlineExT<0xa420d465>);
	m_dsmap.SetAt(0xa440cc05, &GSDrawScanline::DrawScanlineExT<0xa440cc05>);
	m_dsmap.SetAt(0xa440dc25, &GSDrawScanline::DrawScanlineExT<0xa440dc25>);
	m_dsmap.SetAt(0xa4410815, &GSDrawScanline::DrawScanlineExT<0xa4410815>);
	m_dsmap.SetAt(0xa4490814, &GSDrawScanline::DrawScanlineExT<0xa4490814>);
	m_dsmap.SetAt(0xa480dc25, &GSDrawScanline::DrawScanlineExT<0xa480dc25>);
	m_dsmap.SetAt(0xa484d465, &GSDrawScanline::DrawScanlineExT<0xa484d465>);
	m_dsmap.SetAt(0xa880d465, &GSDrawScanline::DrawScanlineExT<0xa880d465>);
	m_dsmap.SetAt(0xaff02c45, &GSDrawScanline::DrawScanlineExT<0xaff02c45>);
	m_dsmap.SetAt(0xaff0cc05, &GSDrawScanline::DrawScanlineExT<0xaff0cc05>);
	m_dsmap.SetAt(0xaff0cc45, &GSDrawScanline::DrawScanlineExT<0xaff0cc45>);
	m_dsmap.SetAt(0xe004d465, &GSDrawScanline::DrawScanlineExT<0xe004d465>);
	m_dsmap.SetAt(0xe440d425, &GSDrawScanline::DrawScanlineExT<0xe440d425>);
	m_dsmap.SetAt(0xe440ec15, &GSDrawScanline::DrawScanlineExT<0xe440ec15>);
	m_dsmap.SetAt(0xe440ec25, &GSDrawScanline::DrawScanlineExT<0xe440ec25>);
	m_dsmap.SetAt(0xe440fc75, &GSDrawScanline::DrawScanlineExT<0xe440fc75>);
	m_dsmap.SetAt(0xe480ac15, &GSDrawScanline::DrawScanlineExT<0xe480ac15>);
	m_dsmap.SetAt(0xe480ec15, &GSDrawScanline::DrawScanlineExT<0xe480ec15>);
	m_dsmap.SetAt(0xe480fc15, &GSDrawScanline::DrawScanlineExT<0xe480fc15>);
	m_dsmap.SetAt(0xe484d465, &GSDrawScanline::DrawScanlineExT<0xe484d465>);
	m_dsmap.SetAt(0xe6402815, &GSDrawScanline::DrawScanlineExT<0xe6402815>);
	m_dsmap.SetAt(0xe880d465, &GSDrawScanline::DrawScanlineExT<0xe880d465>);

	// 12riven

	m_dsmap.SetAt(0x24402208, &GSDrawScanline::DrawScanlineExT<0x24402208>);
	m_dsmap.SetAt(0x24403c08, &GSDrawScanline::DrawScanlineExT<0x24403c08>);
	m_dsmap.SetAt(0xa4402c08, &GSDrawScanline::DrawScanlineExT<0xa4402c08>);
	m_dsmap.SetAt(0xa4402c48, &GSDrawScanline::DrawScanlineExT<0xa4402c48>);

	// onimusha 3

	m_dsmap.SetAt(0x0ff02c28, &GSDrawScanline::DrawScanlineExT<0x0ff02c28>);
	m_dsmap.SetAt(0x0ff02c8a, &GSDrawScanline::DrawScanlineExT<0x0ff02c8a>);
	m_dsmap.SetAt(0x0ff03808, &GSDrawScanline::DrawScanlineExT<0x0ff03808>);
	m_dsmap.SetAt(0x0ff03c08, &GSDrawScanline::DrawScanlineExT<0x0ff03c08>);
	m_dsmap.SetAt(0x0ff03c0a, &GSDrawScanline::DrawScanlineExT<0x0ff03c0a>);
	m_dsmap.SetAt(0x2440bc28, &GSDrawScanline::DrawScanlineExT<0x2440bc28>);
	m_dsmap.SetAt(0x24803c48, &GSDrawScanline::DrawScanlineExT<0x24803c48>);
	m_dsmap.SetAt(0x26402c08, &GSDrawScanline::DrawScanlineExT<0x26402c08>);
	m_dsmap.SetAt(0x26802228, &GSDrawScanline::DrawScanlineExT<0x26802228>);
	m_dsmap.SetAt(0x26803c08, &GSDrawScanline::DrawScanlineExT<0x26803c08>);
	m_dsmap.SetAt(0x26803c88, &GSDrawScanline::DrawScanlineExT<0x26803c88>);
	m_dsmap.SetAt(0x2ff02238, &GSDrawScanline::DrawScanlineExT<0x2ff02238>);
	m_dsmap.SetAt(0x2ff02c8a, &GSDrawScanline::DrawScanlineExT<0x2ff02c8a>);
	m_dsmap.SetAt(0x2ff82208, &GSDrawScanline::DrawScanlineExT<0x2ff82208>);
	m_dsmap.SetAt(0x2ff82c08, &GSDrawScanline::DrawScanlineExT<0x2ff82c08>);
	m_dsmap.SetAt(0x6461d468, &GSDrawScanline::DrawScanlineExT<0x6461d468>);
	m_dsmap.SetAt(0x6481c268, &GSDrawScanline::DrawScanlineExT<0x6481c268>);
	m_dsmap.SetAt(0x6641c228, &GSDrawScanline::DrawScanlineExT<0x6641c228>);
	m_dsmap.SetAt(0x6ff1c228, &GSDrawScanline::DrawScanlineExT<0x6ff1c228>);
	m_dsmap.SetAt(0x8ff4d048, &GSDrawScanline::DrawScanlineExT<0x8ff4d048>);
	m_dsmap.SetAt(0x8ff4d448, &GSDrawScanline::DrawScanlineExT<0x8ff4d448>);
	m_dsmap.SetAt(0xa4403468, &GSDrawScanline::DrawScanlineExT<0xa4403468>);
	m_dsmap.SetAt(0xa4803c08, &GSDrawScanline::DrawScanlineExT<0xa4803c08>);
	m_dsmap.SetAt(0xa8902c88, &GSDrawScanline::DrawScanlineExT<0xa8902c88>);
	m_dsmap.SetAt(0xaa102c88, &GSDrawScanline::DrawScanlineExT<0xaa102c88>);
	m_dsmap.SetAt(0xaff02c88, &GSDrawScanline::DrawScanlineExT<0xaff02c88>);
	m_dsmap.SetAt(0xaff03c88, &GSDrawScanline::DrawScanlineExT<0xaff03c88>);
	m_dsmap.SetAt(0xcff4b468, &GSDrawScanline::DrawScanlineExT<0xcff4b468>);
	m_dsmap.SetAt(0xcff5d068, &GSDrawScanline::DrawScanlineExT<0xcff5d068>);
	m_dsmap.SetAt(0xe411d428, &GSDrawScanline::DrawScanlineExT<0xe411d428>);
	m_dsmap.SetAt(0xe421d428, &GSDrawScanline::DrawScanlineExT<0xe421d428>);
	m_dsmap.SetAt(0xe4403c28, &GSDrawScanline::DrawScanlineExT<0xe4403c28>);
	m_dsmap.SetAt(0xe441d428, &GSDrawScanline::DrawScanlineExT<0xe441d428>);
	m_dsmap.SetAt(0xe441dc28, &GSDrawScanline::DrawScanlineExT<0xe441dc28>);
	m_dsmap.SetAt(0xe441dc68, &GSDrawScanline::DrawScanlineExT<0xe441dc68>);
	m_dsmap.SetAt(0xe445d468, &GSDrawScanline::DrawScanlineExT<0xe445d468>);
	m_dsmap.SetAt(0xe461dc28, &GSDrawScanline::DrawScanlineExT<0xe461dc28>);
	m_dsmap.SetAt(0xe4803c28, &GSDrawScanline::DrawScanlineExT<0xe4803c28>);
	m_dsmap.SetAt(0xe481d468, &GSDrawScanline::DrawScanlineExT<0xe481d468>);
	m_dsmap.SetAt(0xe481dc28, &GSDrawScanline::DrawScanlineExT<0xe481dc28>);
	m_dsmap.SetAt(0xe640bc28, &GSDrawScanline::DrawScanlineExT<0xe640bc28>);
	m_dsmap.SetAt(0xe640ec28, &GSDrawScanline::DrawScanlineExT<0xe640ec28>);
	m_dsmap.SetAt(0xe641cc28, &GSDrawScanline::DrawScanlineExT<0xe641cc28>);
	m_dsmap.SetAt(0xe644b468, &GSDrawScanline::DrawScanlineExT<0xe644b468>);

	// nba 2k8

	m_dsmap.SetAt(0x24403446, &GSDrawScanline::DrawScanlineExT<0x24403446>);
	m_dsmap.SetAt(0x2448d466, &GSDrawScanline::DrawScanlineExT<0x2448d466>);
	m_dsmap.SetAt(0x60002256, &GSDrawScanline::DrawScanlineExT<0x60002256>);
	m_dsmap.SetAt(0x6440b466, &GSDrawScanline::DrawScanlineExT<0x6440b466>);
	m_dsmap.SetAt(0x6480b466, &GSDrawScanline::DrawScanlineExT<0x6480b466>);
	m_dsmap.SetAt(0x6a80b456, &GSDrawScanline::DrawScanlineExT<0x6a80b456>);
	m_dsmap.SetAt(0x6a80b466, &GSDrawScanline::DrawScanlineExT<0x6a80b466>);
	m_dsmap.SetAt(0xa440b466, &GSDrawScanline::DrawScanlineExT<0xa440b466>);
	m_dsmap.SetAt(0xa480b466, &GSDrawScanline::DrawScanlineExT<0xa480b466>);
	m_dsmap.SetAt(0xaa80b466, &GSDrawScanline::DrawScanlineExT<0xaa80b466>);
	m_dsmap.SetAt(0xe440a466, &GSDrawScanline::DrawScanlineExT<0xe440a466>);
	m_dsmap.SetAt(0xe440b466, &GSDrawScanline::DrawScanlineExT<0xe440b466>);
	m_dsmap.SetAt(0xe440c466, &GSDrawScanline::DrawScanlineExT<0xe440c466>);
	m_dsmap.SetAt(0xe440d466, &GSDrawScanline::DrawScanlineExT<0xe440d466>);
	m_dsmap.SetAt(0xea80a466, &GSDrawScanline::DrawScanlineExT<0xea80a466>);
	m_dsmap.SetAt(0xea80b066, &GSDrawScanline::DrawScanlineExT<0xea80b066>);
	m_dsmap.SetAt(0xea80b466, &GSDrawScanline::DrawScanlineExT<0xea80b466>);

	// svr2k8

	m_dsmap.SetAt(0x24412244, &GSDrawScanline::DrawScanlineExT<0x24412244>);
	m_dsmap.SetAt(0x2441a244, &GSDrawScanline::DrawScanlineExT<0x2441a244>);
	m_dsmap.SetAt(0x2441c244, &GSDrawScanline::DrawScanlineExT<0x2441c244>);
	m_dsmap.SetAt(0x2441d044, &GSDrawScanline::DrawScanlineExT<0x2441d044>);
	m_dsmap.SetAt(0x2441d444, &GSDrawScanline::DrawScanlineExT<0x2441d444>);
	m_dsmap.SetAt(0x2481a244, &GSDrawScanline::DrawScanlineExT<0x2481a244>);
	m_dsmap.SetAt(0x26242244, &GSDrawScanline::DrawScanlineExT<0x26242244>);
	m_dsmap.SetAt(0x2a910214, &GSDrawScanline::DrawScanlineExT<0x2a910214>);
	m_dsmap.SetAt(0x2ff31815, &GSDrawScanline::DrawScanlineExT<0x2ff31815>);
	m_dsmap.SetAt(0x4ff02216, &GSDrawScanline::DrawScanlineExT<0x4ff02216>);
	m_dsmap.SetAt(0x64403064, &GSDrawScanline::DrawScanlineExT<0x64403064>);
	m_dsmap.SetAt(0x6440dc24, &GSDrawScanline::DrawScanlineExT<0x6440dc24>);
	m_dsmap.SetAt(0x6441a214, &GSDrawScanline::DrawScanlineExT<0x6441a214>);
	m_dsmap.SetAt(0x6444d064, &GSDrawScanline::DrawScanlineExT<0x6444d064>);
	m_dsmap.SetAt(0x6a9c2274, &GSDrawScanline::DrawScanlineExT<0x6a9c2274>);
	m_dsmap.SetAt(0xa441b444, &GSDrawScanline::DrawScanlineExT<0xa441b444>);
	m_dsmap.SetAt(0xa441bc04, &GSDrawScanline::DrawScanlineExT<0xa441bc04>);
	m_dsmap.SetAt(0xa441d444, &GSDrawScanline::DrawScanlineExT<0xa441d444>);
	m_dsmap.SetAt(0xa441dc04, &GSDrawScanline::DrawScanlineExT<0xa441dc04>);
	m_dsmap.SetAt(0xa444b464, &GSDrawScanline::DrawScanlineExT<0xa444b464>);
	m_dsmap.SetAt(0xa44c3474, &GSDrawScanline::DrawScanlineExT<0xa44c3474>);
	m_dsmap.SetAt(0xa481b444, &GSDrawScanline::DrawScanlineExT<0xa481b444>);
	m_dsmap.SetAt(0xa481b464, &GSDrawScanline::DrawScanlineExT<0xa481b464>);
	m_dsmap.SetAt(0xa4843444, &GSDrawScanline::DrawScanlineExT<0xa4843444>);
	m_dsmap.SetAt(0xa484d464, &GSDrawScanline::DrawScanlineExT<0xa484d464>);
	m_dsmap.SetAt(0xa485d464, &GSDrawScanline::DrawScanlineExT<0xa485d464>);
	m_dsmap.SetAt(0xcff0dc24, &GSDrawScanline::DrawScanlineExT<0xcff0dc24>);
	m_dsmap.SetAt(0xcff0dc26, &GSDrawScanline::DrawScanlineExT<0xcff0dc26>);
	m_dsmap.SetAt(0xe440d454, &GSDrawScanline::DrawScanlineExT<0xe440d454>);
	m_dsmap.SetAt(0xe441b464, &GSDrawScanline::DrawScanlineExT<0xe441b464>);
	m_dsmap.SetAt(0xe441b5e4, &GSDrawScanline::DrawScanlineExT<0xe441b5e4>);
	m_dsmap.SetAt(0xe441bc14, &GSDrawScanline::DrawScanlineExT<0xe441bc14>);
	m_dsmap.SetAt(0xe4443464, &GSDrawScanline::DrawScanlineExT<0xe4443464>);
	m_dsmap.SetAt(0xe481b454, &GSDrawScanline::DrawScanlineExT<0xe481b454>);
	m_dsmap.SetAt(0xe484d464, &GSDrawScanline::DrawScanlineExT<0xe484d464>);
	m_dsmap.SetAt(0xe64c3474, &GSDrawScanline::DrawScanlineExT<0xe64c3474>);

	// rumble roses

	m_dsmap.SetAt(0x0ff03805, &GSDrawScanline::DrawScanlineExT<0x0ff03805>);
	m_dsmap.SetAt(0x24410244, &GSDrawScanline::DrawScanlineExT<0x24410244>);
	m_dsmap.SetAt(0x24430214, &GSDrawScanline::DrawScanlineExT<0x24430214>);
	m_dsmap.SetAt(0x2ff10204, &GSDrawScanline::DrawScanlineExT<0x2ff10204>);
	m_dsmap.SetAt(0x6440c224, &GSDrawScanline::DrawScanlineExT<0x6440c224>);
	m_dsmap.SetAt(0x6440d824, &GSDrawScanline::DrawScanlineExT<0x6440d824>);
	m_dsmap.SetAt(0x64443064, &GSDrawScanline::DrawScanlineExT<0x64443064>);
	m_dsmap.SetAt(0x6640d824, &GSDrawScanline::DrawScanlineExT<0x6640d824>);
	m_dsmap.SetAt(0x66443064, &GSDrawScanline::DrawScanlineExT<0x66443064>);
	m_dsmap.SetAt(0xe440d464, &GSDrawScanline::DrawScanlineExT<0xe440d464>);
	m_dsmap.SetAt(0xe440d5e4, &GSDrawScanline::DrawScanlineExT<0xe440d5e4>);
	m_dsmap.SetAt(0xe440dc24, &GSDrawScanline::DrawScanlineExT<0xe440dc24>);
	m_dsmap.SetAt(0xe441d464, &GSDrawScanline::DrawScanlineExT<0xe441d464>);
	m_dsmap.SetAt(0xe443d5e4, &GSDrawScanline::DrawScanlineExT<0xe443d5e4>);
	m_dsmap.SetAt(0xe445d064, &GSDrawScanline::DrawScanlineExT<0xe445d064>);
	m_dsmap.SetAt(0xe447d464, &GSDrawScanline::DrawScanlineExT<0xe447d464>);
	m_dsmap.SetAt(0xe5843464, &GSDrawScanline::DrawScanlineExT<0xe5843464>);
	m_dsmap.SetAt(0xe6443464, &GSDrawScanline::DrawScanlineExT<0xe6443464>);
	m_dsmap.SetAt(0xe644d464, &GSDrawScanline::DrawScanlineExT<0xe644d464>);
	m_dsmap.SetAt(0xe645d064, &GSDrawScanline::DrawScanlineExT<0xe645d064>);
	m_dsmap.SetAt(0xe645d464, &GSDrawScanline::DrawScanlineExT<0xe645d464>);

	// disgaea 2

	m_dsmap.SetAt(0x6441d064, &GSDrawScanline::DrawScanlineExT<0x6441d064>);
	m_dsmap.SetAt(0x6a80c224, &GSDrawScanline::DrawScanlineExT<0x6a80c224>);
	m_dsmap.SetAt(0xe2a0d474, &GSDrawScanline::DrawScanlineExT<0xe2a0d474>);
	m_dsmap.SetAt(0xe440dc64, &GSDrawScanline::DrawScanlineExT<0xe440dc64>);
	m_dsmap.SetAt(0xe481d464, &GSDrawScanline::DrawScanlineExT<0xe481d464>);
	m_dsmap.SetAt(0xe620d464, &GSDrawScanline::DrawScanlineExT<0xe620d464>);

	// Gundam Seed Destiny OMNI VS ZAFT II PLUS 

	m_dsmap.SetAt(0x0ff12205, &GSDrawScanline::DrawScanlineExT<0x0ff12205>);
	m_dsmap.SetAt(0x4ff12215, &GSDrawScanline::DrawScanlineExT<0x4ff12215>);
	m_dsmap.SetAt(0xa4402c05, &GSDrawScanline::DrawScanlineExT<0xa4402c05>);
	m_dsmap.SetAt(0xa4403445, &GSDrawScanline::DrawScanlineExT<0xa4403445>);
	m_dsmap.SetAt(0xa4403c05, &GSDrawScanline::DrawScanlineExT<0xa4403c05>);
	m_dsmap.SetAt(0xa441b475, &GSDrawScanline::DrawScanlineExT<0xa441b475>);
	m_dsmap.SetAt(0xa445b475, &GSDrawScanline::DrawScanlineExT<0xa445b475>);
	m_dsmap.SetAt(0xa4803445, &GSDrawScanline::DrawScanlineExT<0xa4803445>);
	m_dsmap.SetAt(0xa481b475, &GSDrawScanline::DrawScanlineExT<0xa481b475>);
	m_dsmap.SetAt(0xa485b475, &GSDrawScanline::DrawScanlineExT<0xa485b475>);
	m_dsmap.SetAt(0xcff1b475, &GSDrawScanline::DrawScanlineExT<0xcff1b475>);
	m_dsmap.SetAt(0xcff5b475, &GSDrawScanline::DrawScanlineExT<0xcff5b475>);
	m_dsmap.SetAt(0xe445b475, &GSDrawScanline::DrawScanlineExT<0xe445b475>);

	// grandia 3

	m_dsmap.SetAt(0x20a0224a, &GSDrawScanline::DrawScanlineExT<0x20a0224a>);
	m_dsmap.SetAt(0x24202200, &GSDrawScanline::DrawScanlineExT<0x24202200>);
	m_dsmap.SetAt(0x24471460, &GSDrawScanline::DrawScanlineExT<0x24471460>);
	m_dsmap.SetAt(0x24803c00, &GSDrawScanline::DrawScanlineExT<0x24803c00>);
	m_dsmap.SetAt(0x24830240, &GSDrawScanline::DrawScanlineExT<0x24830240>);
	m_dsmap.SetAt(0x24830260, &GSDrawScanline::DrawScanlineExT<0x24830260>);
	m_dsmap.SetAt(0x26402200, &GSDrawScanline::DrawScanlineExT<0x26402200>);
	m_dsmap.SetAt(0x26402c00, &GSDrawScanline::DrawScanlineExT<0x26402c00>);
	m_dsmap.SetAt(0x26403c00, &GSDrawScanline::DrawScanlineExT<0x26403c00>);
	m_dsmap.SetAt(0x26403c20, &GSDrawScanline::DrawScanlineExT<0x26403c20>);
	m_dsmap.SetAt(0x26433c00, &GSDrawScanline::DrawScanlineExT<0x26433c00>);
	m_dsmap.SetAt(0x26433c20, &GSDrawScanline::DrawScanlineExT<0x26433c20>);
	m_dsmap.SetAt(0x2ff02200, &GSDrawScanline::DrawScanlineExT<0x2ff02200>);
	m_dsmap.SetAt(0x4ff02210, &GSDrawScanline::DrawScanlineExT<0x4ff02210>);
	m_dsmap.SetAt(0x4ff0221a, &GSDrawScanline::DrawScanlineExT<0x4ff0221a>);
	m_dsmap.SetAt(0x6445a260, &GSDrawScanline::DrawScanlineExT<0x6445a260>);
	m_dsmap.SetAt(0x6447a260, &GSDrawScanline::DrawScanlineExT<0x6447a260>);
	m_dsmap.SetAt(0x66403c20, &GSDrawScanline::DrawScanlineExT<0x66403c20>);
	m_dsmap.SetAt(0x66433c20, &GSDrawScanline::DrawScanlineExT<0x66433c20>);
	m_dsmap.SetAt(0xa0903440, &GSDrawScanline::DrawScanlineExT<0xa0903440>);
	m_dsmap.SetAt(0xa0931460, &GSDrawScanline::DrawScanlineExT<0xa0931460>);
	m_dsmap.SetAt(0xa4271460, &GSDrawScanline::DrawScanlineExT<0xa4271460>);
	m_dsmap.SetAt(0xa4403440, &GSDrawScanline::DrawScanlineExT<0xa4403440>);
	m_dsmap.SetAt(0xa4471470, &GSDrawScanline::DrawScanlineExT<0xa4471470>);
	m_dsmap.SetAt(0xa4831440, &GSDrawScanline::DrawScanlineExT<0xa4831440>);
	m_dsmap.SetAt(0xa4831460, &GSDrawScanline::DrawScanlineExT<0xa4831460>);
	m_dsmap.SetAt(0xa4871460, &GSDrawScanline::DrawScanlineExT<0xa4871460>);
	m_dsmap.SetAt(0xa8191460, &GSDrawScanline::DrawScanlineExT<0xa8191460>);
	m_dsmap.SetAt(0xe441b460, &GSDrawScanline::DrawScanlineExT<0xe441b460>);
	m_dsmap.SetAt(0xe443b460, &GSDrawScanline::DrawScanlineExT<0xe443b460>);
	m_dsmap.SetAt(0xe445b460, &GSDrawScanline::DrawScanlineExT<0xe445b460>);
	m_dsmap.SetAt(0xe445d460, &GSDrawScanline::DrawScanlineExT<0xe445d460>);
	m_dsmap.SetAt(0xe447b460, &GSDrawScanline::DrawScanlineExT<0xe447b460>);
	m_dsmap.SetAt(0xe447d460, &GSDrawScanline::DrawScanlineExT<0xe447d460>);

	// shadow of the colossus

	m_dsmap.SetAt(0x0ff02805, &GSDrawScanline::DrawScanlineExT<0x0ff02805>);
	m_dsmap.SetAt(0x0ff10214, &GSDrawScanline::DrawScanlineExT<0x0ff10214>);
	m_dsmap.SetAt(0x0ff10224, &GSDrawScanline::DrawScanlineExT<0x0ff10224>);
	m_dsmap.SetAt(0x0ff10254, &GSDrawScanline::DrawScanlineExT<0x0ff10254>);
	m_dsmap.SetAt(0x0ff10264, &GSDrawScanline::DrawScanlineExT<0x0ff10264>);
	m_dsmap.SetAt(0x0ff10c14, &GSDrawScanline::DrawScanlineExT<0x0ff10c14>);
	m_dsmap.SetAt(0x0ff11c15, &GSDrawScanline::DrawScanlineExT<0x0ff11c15>);
	m_dsmap.SetAt(0x24411814, &GSDrawScanline::DrawScanlineExT<0x24411814>);
	m_dsmap.SetAt(0x24411c14, &GSDrawScanline::DrawScanlineExT<0x24411c14>);
	m_dsmap.SetAt(0x24411c64, &GSDrawScanline::DrawScanlineExT<0x24411c64>);
	m_dsmap.SetAt(0x24451064, &GSDrawScanline::DrawScanlineExT<0x24451064>);
	m_dsmap.SetAt(0x24491c24, &GSDrawScanline::DrawScanlineExT<0x24491c24>);
	m_dsmap.SetAt(0x24810214, &GSDrawScanline::DrawScanlineExT<0x24810214>);
	m_dsmap.SetAt(0x24810254, &GSDrawScanline::DrawScanlineExT<0x24810254>);
	m_dsmap.SetAt(0x24811c14, &GSDrawScanline::DrawScanlineExT<0x24811c14>);
	m_dsmap.SetAt(0x25411814, &GSDrawScanline::DrawScanlineExT<0x25411814>);
	m_dsmap.SetAt(0x26410214, &GSDrawScanline::DrawScanlineExT<0x26410214>);
	m_dsmap.SetAt(0x26810224, &GSDrawScanline::DrawScanlineExT<0x26810224>);
	m_dsmap.SetAt(0x34451464, &GSDrawScanline::DrawScanlineExT<0x34451464>);
	m_dsmap.SetAt(0x4ff0c224, &GSDrawScanline::DrawScanlineExT<0x4ff0c224>);
	m_dsmap.SetAt(0x64453464, &GSDrawScanline::DrawScanlineExT<0x64453464>);
	m_dsmap.SetAt(0x6445a264, &GSDrawScanline::DrawScanlineExT<0x6445a264>);
	m_dsmap.SetAt(0x6445b464, &GSDrawScanline::DrawScanlineExT<0x6445b464>);
	m_dsmap.SetAt(0xa4251464, &GSDrawScanline::DrawScanlineExT<0xa4251464>);
	m_dsmap.SetAt(0xa4451464, &GSDrawScanline::DrawScanlineExT<0xa4451464>);
	m_dsmap.SetAt(0xa4851464, &GSDrawScanline::DrawScanlineExT<0xa4851464>);
	m_dsmap.SetAt(0xe441cc64, &GSDrawScanline::DrawScanlineExT<0xe441cc64>);
	m_dsmap.SetAt(0xe441dc24, &GSDrawScanline::DrawScanlineExT<0xe441dc24>);
	m_dsmap.SetAt(0xe441dc64, &GSDrawScanline::DrawScanlineExT<0xe441dc64>);
	m_dsmap.SetAt(0xe4453464, &GSDrawScanline::DrawScanlineExT<0xe4453464>);
	m_dsmap.SetAt(0xe445a464, &GSDrawScanline::DrawScanlineExT<0xe445a464>);
	m_dsmap.SetAt(0xe485a464, &GSDrawScanline::DrawScanlineExT<0xe485a464>);
	m_dsmap.SetAt(0xa4843464, &GSDrawScanline::DrawScanlineExT<0xa4843464>);
	m_dsmap.SetAt(0xe441b454, &GSDrawScanline::DrawScanlineExT<0xe441b454>);
	m_dsmap.SetAt(0xe481b464, &GSDrawScanline::DrawScanlineExT<0xe481b464>);

	// dq8

	m_dsmap.SetAt(0x0ff02244, &GSDrawScanline::DrawScanlineExT<0x0ff02244>);
	m_dsmap.SetAt(0x0ff02c04, &GSDrawScanline::DrawScanlineExT<0x0ff02c04>);
	m_dsmap.SetAt(0x0ff02c85, &GSDrawScanline::DrawScanlineExT<0x0ff02c85>);
	m_dsmap.SetAt(0x0ff0dc04, &GSDrawScanline::DrawScanlineExT<0x0ff0dc04>);
	m_dsmap.SetAt(0x22a02c04, &GSDrawScanline::DrawScanlineExT<0x22a02c04>);
	m_dsmap.SetAt(0x24202204, &GSDrawScanline::DrawScanlineExT<0x24202204>);
	m_dsmap.SetAt(0x2420a264, &GSDrawScanline::DrawScanlineExT<0x2420a264>);
	m_dsmap.SetAt(0x24402244, &GSDrawScanline::DrawScanlineExT<0x24402244>);
	m_dsmap.SetAt(0x2440bc04, &GSDrawScanline::DrawScanlineExT<0x2440bc04>);
	m_dsmap.SetAt(0x2440dc04, &GSDrawScanline::DrawScanlineExT<0x2440dc04>);
	m_dsmap.SetAt(0x2444b464, &GSDrawScanline::DrawScanlineExT<0x2444b464>);
	m_dsmap.SetAt(0x24603c04, &GSDrawScanline::DrawScanlineExT<0x24603c04>);
	m_dsmap.SetAt(0x2480a264, &GSDrawScanline::DrawScanlineExT<0x2480a264>);
	m_dsmap.SetAt(0x2484b464, &GSDrawScanline::DrawScanlineExT<0x2484b464>);
	m_dsmap.SetAt(0x26203c04, &GSDrawScanline::DrawScanlineExT<0x26203c04>);
	m_dsmap.SetAt(0x2640bc04, &GSDrawScanline::DrawScanlineExT<0x2640bc04>);
	m_dsmap.SetAt(0x28803c04, &GSDrawScanline::DrawScanlineExT<0x28803c04>);
	m_dsmap.SetAt(0x28803d84, &GSDrawScanline::DrawScanlineExT<0x28803d84>);
	m_dsmap.SetAt(0x2ff03c06, &GSDrawScanline::DrawScanlineExT<0x2ff03c06>);
	m_dsmap.SetAt(0x6440a264, &GSDrawScanline::DrawScanlineExT<0x6440a264>);
	m_dsmap.SetAt(0x6444b064, &GSDrawScanline::DrawScanlineExT<0x6444b064>);
	m_dsmap.SetAt(0x6444b464, &GSDrawScanline::DrawScanlineExT<0x6444b464>);
	m_dsmap.SetAt(0x6484b464, &GSDrawScanline::DrawScanlineExT<0x6484b464>);
	m_dsmap.SetAt(0x8ff03c04, &GSDrawScanline::DrawScanlineExT<0x8ff03c04>);
	m_dsmap.SetAt(0x8ff03c05, &GSDrawScanline::DrawScanlineExT<0x8ff03c05>);
	m_dsmap.SetAt(0xa4203c05, &GSDrawScanline::DrawScanlineExT<0xa4203c05>);
	m_dsmap.SetAt(0xa4402c04, &GSDrawScanline::DrawScanlineExT<0xa4402c04>);
	m_dsmap.SetAt(0xa4803c04, &GSDrawScanline::DrawScanlineExT<0xa4803c04>);
	m_dsmap.SetAt(0xa480b464, &GSDrawScanline::DrawScanlineExT<0xa480b464>);
	m_dsmap.SetAt(0xe444b454, &GSDrawScanline::DrawScanlineExT<0xe444b454>);
	m_dsmap.SetAt(0xe484b464, &GSDrawScanline::DrawScanlineExT<0xe484b464>);
	m_dsmap.SetAt(0xa4403c24, &GSDrawScanline::DrawScanlineExT<0xa4403c24>);
	m_dsmap.SetAt(0xa4803c24, &GSDrawScanline::DrawScanlineExT<0xa4803c24>);

	// suikoden 5

	m_dsmap.SetAt(0x2441b448, &GSDrawScanline::DrawScanlineExT<0x2441b448>);
	m_dsmap.SetAt(0x60203468, &GSDrawScanline::DrawScanlineExT<0x60203468>);
	m_dsmap.SetAt(0x64402258, &GSDrawScanline::DrawScanlineExT<0x64402258>);
	m_dsmap.SetAt(0x6441a468, &GSDrawScanline::DrawScanlineExT<0x6441a468>);
	m_dsmap.SetAt(0x6441b468, &GSDrawScanline::DrawScanlineExT<0x6441b468>);
	m_dsmap.SetAt(0x64803468, &GSDrawScanline::DrawScanlineExT<0x64803468>);
	m_dsmap.SetAt(0xa421b448, &GSDrawScanline::DrawScanlineExT<0xa421b448>);
	m_dsmap.SetAt(0xe6803468, &GSDrawScanline::DrawScanlineExT<0xe6803468>);

	// culdcept

	m_dsmap.SetAt(0x0ff02200, &GSDrawScanline::DrawScanlineExT<0x0ff02200>);
	m_dsmap.SetAt(0x0ff03880, &GSDrawScanline::DrawScanlineExT<0x0ff03880>);
	m_dsmap.SetAt(0x2440e266, &GSDrawScanline::DrawScanlineExT<0x2440e266>);
	m_dsmap.SetAt(0x2440e466, &GSDrawScanline::DrawScanlineExT<0x2440e466>);
	m_dsmap.SetAt(0x2440ec26, &GSDrawScanline::DrawScanlineExT<0x2440ec26>);
	m_dsmap.SetAt(0x2440f466, &GSDrawScanline::DrawScanlineExT<0x2440f466>);
	m_dsmap.SetAt(0x2480e266, &GSDrawScanline::DrawScanlineExT<0x2480e266>);
	m_dsmap.SetAt(0x2680e266, &GSDrawScanline::DrawScanlineExT<0x2680e266>);
	m_dsmap.SetAt(0x2680f566, &GSDrawScanline::DrawScanlineExT<0x2680f566>);
	m_dsmap.SetAt(0x2680fc26, &GSDrawScanline::DrawScanlineExT<0x2680fc26>);
	m_dsmap.SetAt(0x2a10f566, &GSDrawScanline::DrawScanlineExT<0x2a10f566>);
	m_dsmap.SetAt(0x4ff0e266, &GSDrawScanline::DrawScanlineExT<0x4ff0e266>);
	m_dsmap.SetAt(0x4ff0e466, &GSDrawScanline::DrawScanlineExT<0x4ff0e466>);
	m_dsmap.SetAt(0x4ff0ec26, &GSDrawScanline::DrawScanlineExT<0x4ff0ec26>);
	m_dsmap.SetAt(0x4ff0f526, &GSDrawScanline::DrawScanlineExT<0x4ff0f526>);
	m_dsmap.SetAt(0x4ff0f566, &GSDrawScanline::DrawScanlineExT<0x4ff0f566>);
	m_dsmap.SetAt(0x4ff0fc26, &GSDrawScanline::DrawScanlineExT<0x4ff0fc26>);
	m_dsmap.SetAt(0x4ff4ec26, &GSDrawScanline::DrawScanlineExT<0x4ff4ec26>);
	m_dsmap.SetAt(0x4ff4f526, &GSDrawScanline::DrawScanlineExT<0x4ff4f526>);
	m_dsmap.SetAt(0x4ff4f566, &GSDrawScanline::DrawScanlineExT<0x4ff4f566>);
	m_dsmap.SetAt(0x4ff4fc26, &GSDrawScanline::DrawScanlineExT<0x4ff4fc26>);
	m_dsmap.SetAt(0x6440ec26, &GSDrawScanline::DrawScanlineExT<0x6440ec26>);
	m_dsmap.SetAt(0x6440f466, &GSDrawScanline::DrawScanlineExT<0x6440f466>);
	m_dsmap.SetAt(0x6440fc26, &GSDrawScanline::DrawScanlineExT<0x6440fc26>);
	m_dsmap.SetAt(0x6444fc26, &GSDrawScanline::DrawScanlineExT<0x6444fc26>);
	m_dsmap.SetAt(0xa440ec26, &GSDrawScanline::DrawScanlineExT<0xa440ec26>);
	m_dsmap.SetAt(0xcff0ec26, &GSDrawScanline::DrawScanlineExT<0xcff0ec26>);
	m_dsmap.SetAt(0xcff0fc26, &GSDrawScanline::DrawScanlineExT<0xcff0fc26>);
	m_dsmap.SetAt(0xcff4ec26, &GSDrawScanline::DrawScanlineExT<0xcff4ec26>);
	m_dsmap.SetAt(0xcff4fc26, &GSDrawScanline::DrawScanlineExT<0xcff4fc26>);

	// resident evil 4

	m_dsmap.SetAt(0x0ff02c24, &GSDrawScanline::DrawScanlineExT<0x0ff02c24>);
	m_dsmap.SetAt(0x0ff02c84, &GSDrawScanline::DrawScanlineExT<0x0ff02c84>);
	m_dsmap.SetAt(0x0ff03884, &GSDrawScanline::DrawScanlineExT<0x0ff03884>);
	m_dsmap.SetAt(0x0ff03c00, &GSDrawScanline::DrawScanlineExT<0x0ff03c00>);
	m_dsmap.SetAt(0x22a03c84, &GSDrawScanline::DrawScanlineExT<0x22a03c84>);
	m_dsmap.SetAt(0x2440c204, &GSDrawScanline::DrawScanlineExT<0x2440c204>);
	m_dsmap.SetAt(0x2440d404, &GSDrawScanline::DrawScanlineExT<0x2440d404>);
	m_dsmap.SetAt(0x24802204, &GSDrawScanline::DrawScanlineExT<0x24802204>);
	m_dsmap.SetAt(0x24803c04, &GSDrawScanline::DrawScanlineExT<0x24803c04>);
	m_dsmap.SetAt(0x2480c224, &GSDrawScanline::DrawScanlineExT<0x2480c224>);
	m_dsmap.SetAt(0x2481c264, &GSDrawScanline::DrawScanlineExT<0x2481c264>);
	m_dsmap.SetAt(0x26403c84, &GSDrawScanline::DrawScanlineExT<0x26403c84>);
	m_dsmap.SetAt(0x2640c224, &GSDrawScanline::DrawScanlineExT<0x2640c224>);
	m_dsmap.SetAt(0x26802224, &GSDrawScanline::DrawScanlineExT<0x26802224>);
	m_dsmap.SetAt(0x26803c84, &GSDrawScanline::DrawScanlineExT<0x26803c84>);
	m_dsmap.SetAt(0x28902204, &GSDrawScanline::DrawScanlineExT<0x28902204>);
	m_dsmap.SetAt(0x2a403c84, &GSDrawScanline::DrawScanlineExT<0x2a403c84>);
	m_dsmap.SetAt(0x6640d464, &GSDrawScanline::DrawScanlineExT<0x6640d464>);
	m_dsmap.SetAt(0x66420214, &GSDrawScanline::DrawScanlineExT<0x66420214>);
	m_dsmap.SetAt(0xa2a02c84, &GSDrawScanline::DrawScanlineExT<0xa2a02c84>);
	m_dsmap.SetAt(0xa440d444, &GSDrawScanline::DrawScanlineExT<0xa440d444>);
	m_dsmap.SetAt(0xa480d444, &GSDrawScanline::DrawScanlineExT<0xa480d444>);
	m_dsmap.SetAt(0xa480d464, &GSDrawScanline::DrawScanlineExT<0xa480d464>);
	m_dsmap.SetAt(0xa680d444, &GSDrawScanline::DrawScanlineExT<0xa680d444>);
	m_dsmap.SetAt(0xaff02c84, &GSDrawScanline::DrawScanlineExT<0xaff02c84>);
	m_dsmap.SetAt(0xaff03484, &GSDrawScanline::DrawScanlineExT<0xaff03484>);
	m_dsmap.SetAt(0xe580d464, &GSDrawScanline::DrawScanlineExT<0xe580d464>);

	// persona 3

	m_dsmap.SetAt(0x0ff02248, &GSDrawScanline::DrawScanlineExT<0x0ff02248>);
	m_dsmap.SetAt(0x24402268, &GSDrawScanline::DrawScanlineExT<0x24402268>);
	m_dsmap.SetAt(0x24410448, &GSDrawScanline::DrawScanlineExT<0x24410448>);
	m_dsmap.SetAt(0x2441a448, &GSDrawScanline::DrawScanlineExT<0x2441a448>);
	m_dsmap.SetAt(0x2445a468, &GSDrawScanline::DrawScanlineExT<0x2445a468>);
	m_dsmap.SetAt(0x4ff02268, &GSDrawScanline::DrawScanlineExT<0x4ff02268>);
	m_dsmap.SetAt(0xa4811448, &GSDrawScanline::DrawScanlineExT<0xa4811448>);
	m_dsmap.SetAt(0xa581b448, &GSDrawScanline::DrawScanlineExT<0xa581b448>);
	m_dsmap.SetAt(0xa6a1b448, &GSDrawScanline::DrawScanlineExT<0xa6a1b448>);
	m_dsmap.SetAt(0xe4461468, &GSDrawScanline::DrawScanlineExT<0xe4461468>);

	// bully

	m_dsmap.SetAt(0x24802804, &GSDrawScanline::DrawScanlineExT<0x24802804>);
	m_dsmap.SetAt(0x25403c84, &GSDrawScanline::DrawScanlineExT<0x25403c84>);
	m_dsmap.SetAt(0x26102204, &GSDrawScanline::DrawScanlineExT<0x26102204>);
	m_dsmap.SetAt(0x26803884, &GSDrawScanline::DrawScanlineExT<0x26803884>);
	m_dsmap.SetAt(0x2a402205, &GSDrawScanline::DrawScanlineExT<0x2a402205>);
	m_dsmap.SetAt(0x2a802c86, &GSDrawScanline::DrawScanlineExT<0x2a802c86>);
	m_dsmap.SetAt(0x2ff03884, &GSDrawScanline::DrawScanlineExT<0x2ff03884>);
	m_dsmap.SetAt(0x4ff02264, &GSDrawScanline::DrawScanlineExT<0x4ff02264>);
	m_dsmap.SetAt(0x64402264, &GSDrawScanline::DrawScanlineExT<0x64402264>);
	m_dsmap.SetAt(0x64a02214, &GSDrawScanline::DrawScanlineExT<0x64a02214>);
	m_dsmap.SetAt(0x6ff20235, &GSDrawScanline::DrawScanlineExT<0x6ff20235>);
	m_dsmap.SetAt(0xa441b424, &GSDrawScanline::DrawScanlineExT<0xa441b424>);
	m_dsmap.SetAt(0xa441b464, &GSDrawScanline::DrawScanlineExT<0xa441b464>);
	m_dsmap.SetAt(0xa441bc24, &GSDrawScanline::DrawScanlineExT<0xa441bc24>);
	m_dsmap.SetAt(0xa441bc44, &GSDrawScanline::DrawScanlineExT<0xa441bc44>);
	m_dsmap.SetAt(0xa445b464, &GSDrawScanline::DrawScanlineExT<0xa445b464>);
	m_dsmap.SetAt(0xa681b464, &GSDrawScanline::DrawScanlineExT<0xa681b464>);
	m_dsmap.SetAt(0xa681bc04, &GSDrawScanline::DrawScanlineExT<0xa681bc04>);
	m_dsmap.SetAt(0xa681bc24, &GSDrawScanline::DrawScanlineExT<0xa681bc24>);
	m_dsmap.SetAt(0xa681bc44, &GSDrawScanline::DrawScanlineExT<0xa681bc44>);
	m_dsmap.SetAt(0xa681bc64, &GSDrawScanline::DrawScanlineExT<0xa681bc64>);
	m_dsmap.SetAt(0xe4a53464, &GSDrawScanline::DrawScanlineExT<0xe4a53464>);
	m_dsmap.SetAt(0xe884d464, &GSDrawScanline::DrawScanlineExT<0xe884d464>);

	// tomoyo after 

	m_dsmap.SetAt(0x0ff0220a, &GSDrawScanline::DrawScanlineExT<0x0ff0220a>);
	m_dsmap.SetAt(0x4ff02c19, &GSDrawScanline::DrawScanlineExT<0x4ff02c19>);
	m_dsmap.SetAt(0x4ff03c19, &GSDrawScanline::DrawScanlineExT<0x4ff03c19>);
	m_dsmap.SetAt(0x64202268, &GSDrawScanline::DrawScanlineExT<0x64202268>);
	m_dsmap.SetAt(0x64402c68, &GSDrawScanline::DrawScanlineExT<0x64402c68>);
	m_dsmap.SetAt(0xe4203c68, &GSDrawScanline::DrawScanlineExT<0xe4203c68>);
	m_dsmap.SetAt(0xe4402c68, &GSDrawScanline::DrawScanlineExT<0xe4402c68>);
	m_dsmap.SetAt(0xe4403c68, &GSDrawScanline::DrawScanlineExT<0xe4403c68>);
	m_dsmap.SetAt(0xe4802c68, &GSDrawScanline::DrawScanlineExT<0xe4802c68>);
	m_dsmap.SetAt(0xe5402c68, &GSDrawScanline::DrawScanlineExT<0xe5402c68>);

	// okami

	m_dsmap.SetAt(0x0ff02c88, &GSDrawScanline::DrawScanlineExT<0x0ff02c88>);
	m_dsmap.SetAt(0x24402c08, &GSDrawScanline::DrawScanlineExT<0x24402c08>);
	m_dsmap.SetAt(0x24403c88, &GSDrawScanline::DrawScanlineExT<0x24403c88>);
	m_dsmap.SetAt(0x24411468, &GSDrawScanline::DrawScanlineExT<0x24411468>);
	m_dsmap.SetAt(0x25403c08, &GSDrawScanline::DrawScanlineExT<0x25403c08>);
	m_dsmap.SetAt(0x26203c88, &GSDrawScanline::DrawScanlineExT<0x26203c88>);
	m_dsmap.SetAt(0x26403888, &GSDrawScanline::DrawScanlineExT<0x26403888>);
	m_dsmap.SetAt(0x26403c88, &GSDrawScanline::DrawScanlineExT<0x26403c88>);
	m_dsmap.SetAt(0x26803888, &GSDrawScanline::DrawScanlineExT<0x26803888>);
	m_dsmap.SetAt(0x2ff02c88, &GSDrawScanline::DrawScanlineExT<0x2ff02c88>);
	m_dsmap.SetAt(0x4ff02c18, &GSDrawScanline::DrawScanlineExT<0x4ff02c18>);
	m_dsmap.SetAt(0x62902c18, &GSDrawScanline::DrawScanlineExT<0x62902c18>);
	m_dsmap.SetAt(0x64402218, &GSDrawScanline::DrawScanlineExT<0x64402218>);
	m_dsmap.SetAt(0x64402238, &GSDrawScanline::DrawScanlineExT<0x64402238>);
	m_dsmap.SetAt(0x6440dc18, &GSDrawScanline::DrawScanlineExT<0x6440dc18>);
	m_dsmap.SetAt(0x6444c268, &GSDrawScanline::DrawScanlineExT<0x6444c268>);
	m_dsmap.SetAt(0xa440d428, &GSDrawScanline::DrawScanlineExT<0xa440d428>);
	m_dsmap.SetAt(0xa480d428, &GSDrawScanline::DrawScanlineExT<0xa480d428>);
	m_dsmap.SetAt(0xa8903c88, &GSDrawScanline::DrawScanlineExT<0xa8903c88>);
	m_dsmap.SetAt(0xaff02ca8, &GSDrawScanline::DrawScanlineExT<0xaff02ca8>);
	m_dsmap.SetAt(0xaff03888, &GSDrawScanline::DrawScanlineExT<0xaff03888>);
	m_dsmap.SetAt(0xe4403418, &GSDrawScanline::DrawScanlineExT<0xe4403418>);
	m_dsmap.SetAt(0xe440d418, &GSDrawScanline::DrawScanlineExT<0xe440d418>);
	m_dsmap.SetAt(0xe440d428, &GSDrawScanline::DrawScanlineExT<0xe440d428>);
	m_dsmap.SetAt(0xe440dc18, &GSDrawScanline::DrawScanlineExT<0xe440dc18>);
	m_dsmap.SetAt(0xe440dc28, &GSDrawScanline::DrawScanlineExT<0xe440dc28>);
	m_dsmap.SetAt(0xe444d468, &GSDrawScanline::DrawScanlineExT<0xe444d468>);

	// sfex3

	m_dsmap.SetAt(0x0ff0280a, &GSDrawScanline::DrawScanlineExT<0x0ff0280a>);
	m_dsmap.SetAt(0x0ff0380a, &GSDrawScanline::DrawScanlineExT<0x0ff0380a>);
	m_dsmap.SetAt(0x0ff0c278, &GSDrawScanline::DrawScanlineExT<0x0ff0c278>);
	m_dsmap.SetAt(0x2428e248, &GSDrawScanline::DrawScanlineExT<0x2428e248>);
	m_dsmap.SetAt(0x2440280a, &GSDrawScanline::DrawScanlineExT<0x2440280a>);
	m_dsmap.SetAt(0x24802278, &GSDrawScanline::DrawScanlineExT<0x24802278>);
	m_dsmap.SetAt(0x2480e278, &GSDrawScanline::DrawScanlineExT<0x2480e278>);
	m_dsmap.SetAt(0x4ff02258, &GSDrawScanline::DrawScanlineExT<0x4ff02258>);
	m_dsmap.SetAt(0x66402258, &GSDrawScanline::DrawScanlineExT<0x66402258>);
	m_dsmap.SetAt(0x66a02218, &GSDrawScanline::DrawScanlineExT<0x66a02218>);
	m_dsmap.SetAt(0x6a90c238, &GSDrawScanline::DrawScanlineExT<0x6a90c238>);
	m_dsmap.SetAt(0xa428e5c8, &GSDrawScanline::DrawScanlineExT<0xa428e5c8>);
	m_dsmap.SetAt(0xa4402cb8, &GSDrawScanline::DrawScanlineExT<0xa4402cb8>);
	m_dsmap.SetAt(0xa4803438, &GSDrawScanline::DrawScanlineExT<0xa4803438>);
	m_dsmap.SetAt(0xa480d478, &GSDrawScanline::DrawScanlineExT<0xa480d478>);
	m_dsmap.SetAt(0xa480dc38, &GSDrawScanline::DrawScanlineExT<0xa480dc38>);
	m_dsmap.SetAt(0xa480fc38, &GSDrawScanline::DrawScanlineExT<0xa480fc38>);
	m_dsmap.SetAt(0xcff0c5e8, &GSDrawScanline::DrawScanlineExT<0xcff0c5e8>);
	m_dsmap.SetAt(0xe4402c18, &GSDrawScanline::DrawScanlineExT<0xe4402c18>);
	m_dsmap.SetAt(0xe4403468, &GSDrawScanline::DrawScanlineExT<0xe4403468>);
	m_dsmap.SetAt(0xe440c5e8, &GSDrawScanline::DrawScanlineExT<0xe440c5e8>);
	m_dsmap.SetAt(0xe440cc18, &GSDrawScanline::DrawScanlineExT<0xe440cc18>);
	m_dsmap.SetAt(0xe440d5f8, &GSDrawScanline::DrawScanlineExT<0xe440d5f8>);

	// tokyo bus guide

	m_dsmap.SetAt(0x2444f470, &GSDrawScanline::DrawScanlineExT<0x2444f470>);
	m_dsmap.SetAt(0x4ff0e230, &GSDrawScanline::DrawScanlineExT<0x4ff0e230>);
	m_dsmap.SetAt(0x4ff0e270, &GSDrawScanline::DrawScanlineExT<0x4ff0e270>);
	m_dsmap.SetAt(0x4ff4f470, &GSDrawScanline::DrawScanlineExT<0x4ff4f470>);
	m_dsmap.SetAt(0x6440e230, &GSDrawScanline::DrawScanlineExT<0x6440e230>);
	m_dsmap.SetAt(0x6440e270, &GSDrawScanline::DrawScanlineExT<0x6440e270>);
	m_dsmap.SetAt(0x64420210, &GSDrawScanline::DrawScanlineExT<0x64420210>);
	m_dsmap.SetAt(0x64453450, &GSDrawScanline::DrawScanlineExT<0x64453450>);
	m_dsmap.SetAt(0x8ff4d470, &GSDrawScanline::DrawScanlineExT<0x8ff4d470>);
	m_dsmap.SetAt(0x8ff4f470, &GSDrawScanline::DrawScanlineExT<0x8ff4f470>);
	m_dsmap.SetAt(0xa4411450, &GSDrawScanline::DrawScanlineExT<0xa4411450>);
	m_dsmap.SetAt(0xcff4d470, &GSDrawScanline::DrawScanlineExT<0xcff4d470>);
	m_dsmap.SetAt(0xcff4f470, &GSDrawScanline::DrawScanlineExT<0xcff4f470>);
	m_dsmap.SetAt(0xe440d470, &GSDrawScanline::DrawScanlineExT<0xe440d470>);
	m_dsmap.SetAt(0xe440f430, &GSDrawScanline::DrawScanlineExT<0xe440f430>);
	m_dsmap.SetAt(0xe440fc30, &GSDrawScanline::DrawScanlineExT<0xe440fc30>);
	m_dsmap.SetAt(0xe444f470, &GSDrawScanline::DrawScanlineExT<0xe444f470>);

	// katamary damacy

	m_dsmap.SetAt(0x24402c05, &GSDrawScanline::DrawScanlineExT<0x24402c05>);
	m_dsmap.SetAt(0x2440ec04, &GSDrawScanline::DrawScanlineExT<0x2440ec04>);
	m_dsmap.SetAt(0x2440f805, &GSDrawScanline::DrawScanlineExT<0x2440f805>);
	m_dsmap.SetAt(0x2440f824, &GSDrawScanline::DrawScanlineExT<0x2440f824>);
	m_dsmap.SetAt(0x24410204, &GSDrawScanline::DrawScanlineExT<0x24410204>);
	m_dsmap.SetAt(0x24410802, &GSDrawScanline::DrawScanlineExT<0x24410802>);
	m_dsmap.SetAt(0x24410816, &GSDrawScanline::DrawScanlineExT<0x24410816>);
	m_dsmap.SetAt(0x24442264, &GSDrawScanline::DrawScanlineExT<0x24442264>);
	m_dsmap.SetAt(0x6440e214, &GSDrawScanline::DrawScanlineExT<0x6440e214>);
	m_dsmap.SetAt(0x6440e224, &GSDrawScanline::DrawScanlineExT<0x6440e224>);
	m_dsmap.SetAt(0x6440ec24, &GSDrawScanline::DrawScanlineExT<0x6440ec24>);
	m_dsmap.SetAt(0x6442e214, &GSDrawScanline::DrawScanlineExT<0x6442e214>);
	m_dsmap.SetAt(0x644a0214, &GSDrawScanline::DrawScanlineExT<0x644a0214>);
	m_dsmap.SetAt(0x6484f064, &GSDrawScanline::DrawScanlineExT<0x6484f064>);
	m_dsmap.SetAt(0x6ff02214, &GSDrawScanline::DrawScanlineExT<0x6ff02214>);
	m_dsmap.SetAt(0xa440ec24, &GSDrawScanline::DrawScanlineExT<0xa440ec24>);
	m_dsmap.SetAt(0xa44cf444, &GSDrawScanline::DrawScanlineExT<0xa44cf444>);
	m_dsmap.SetAt(0xa480f464, &GSDrawScanline::DrawScanlineExT<0xa480f464>);
	m_dsmap.SetAt(0xe440ec14, &GSDrawScanline::DrawScanlineExT<0xe440ec14>);
	m_dsmap.SetAt(0xe440ec24, &GSDrawScanline::DrawScanlineExT<0xe440ec24>);
	m_dsmap.SetAt(0xe440f464, &GSDrawScanline::DrawScanlineExT<0xe440f464>);
	m_dsmap.SetAt(0xe444f464, &GSDrawScanline::DrawScanlineExT<0xe444f464>);
	m_dsmap.SetAt(0xe480ec24, &GSDrawScanline::DrawScanlineExT<0xe480ec24>);
	m_dsmap.SetAt(0xe480f464, &GSDrawScanline::DrawScanlineExT<0xe480f464>);
	m_dsmap.SetAt(0xe484f464, &GSDrawScanline::DrawScanlineExT<0xe484f464>);
	m_dsmap.SetAt(0xe810cc14, &GSDrawScanline::DrawScanlineExT<0xe810cc14>);

	// kh2

	m_dsmap.SetAt(0x0ff0dc05, &GSDrawScanline::DrawScanlineExT<0x0ff0dc05>);
	m_dsmap.SetAt(0x2440dc00, &GSDrawScanline::DrawScanlineExT<0x2440dc00>);
	m_dsmap.SetAt(0x2480c274, &GSDrawScanline::DrawScanlineExT<0x2480c274>);
	m_dsmap.SetAt(0x26403805, &GSDrawScanline::DrawScanlineExT<0x26403805>);
	m_dsmap.SetAt(0x26803805, &GSDrawScanline::DrawScanlineExT<0x26803805>);
	m_dsmap.SetAt(0x4ff03894, &GSDrawScanline::DrawScanlineExT<0x4ff03894>);
	m_dsmap.SetAt(0x64402814, &GSDrawScanline::DrawScanlineExT<0x64402814>);
	m_dsmap.SetAt(0x6440c220, &GSDrawScanline::DrawScanlineExT<0x6440c220>);
	m_dsmap.SetAt(0x6444d464, &GSDrawScanline::DrawScanlineExT<0x6444d464>);
	m_dsmap.SetAt(0x64802214, &GSDrawScanline::DrawScanlineExT<0x64802214>);
	m_dsmap.SetAt(0xa4402c45, &GSDrawScanline::DrawScanlineExT<0xa4402c45>);
	m_dsmap.SetAt(0xa4447464, &GSDrawScanline::DrawScanlineExT<0xa4447464>);
	m_dsmap.SetAt(0xe2a4d464, &GSDrawScanline::DrawScanlineExT<0xe2a4d464>);
	m_dsmap.SetAt(0xe4803c54, &GSDrawScanline::DrawScanlineExT<0xe4803c54>);

	// the punisher

	m_dsmap.SetAt(0x2423d064, &GSDrawScanline::DrawScanlineExT<0x2423d064>);
	m_dsmap.SetAt(0x2423d464, &GSDrawScanline::DrawScanlineExT<0x2423d464>);
	m_dsmap.SetAt(0x2443c204, &GSDrawScanline::DrawScanlineExT<0x2443c204>);
	m_dsmap.SetAt(0x2443c206, &GSDrawScanline::DrawScanlineExT<0x2443c206>);
	m_dsmap.SetAt(0x2443dc24, &GSDrawScanline::DrawScanlineExT<0x2443dc24>);
	m_dsmap.SetAt(0x2483dc24, &GSDrawScanline::DrawScanlineExT<0x2483dc24>);
	m_dsmap.SetAt(0x2683c204, &GSDrawScanline::DrawScanlineExT<0x2683c204>);
	m_dsmap.SetAt(0x2a83c246, &GSDrawScanline::DrawScanlineExT<0x2a83c246>);
	m_dsmap.SetAt(0x6443d5e4, &GSDrawScanline::DrawScanlineExT<0x6443d5e4>);
	m_dsmap.SetAt(0x6a83d464, &GSDrawScanline::DrawScanlineExT<0x6a83d464>);
	m_dsmap.SetAt(0x6a83d564, &GSDrawScanline::DrawScanlineExT<0x6a83d564>);
	m_dsmap.SetAt(0xa423d464, &GSDrawScanline::DrawScanlineExT<0xa423d464>);
	m_dsmap.SetAt(0xa443cc04, &GSDrawScanline::DrawScanlineExT<0xa443cc04>);
	m_dsmap.SetAt(0xa443d464, &GSDrawScanline::DrawScanlineExT<0xa443d464>);
	m_dsmap.SetAt(0xa443dc04, &GSDrawScanline::DrawScanlineExT<0xa443dc04>);
	m_dsmap.SetAt(0xa443dc24, &GSDrawScanline::DrawScanlineExT<0xa443dc24>);
	m_dsmap.SetAt(0xa483d444, &GSDrawScanline::DrawScanlineExT<0xa483d444>);
	m_dsmap.SetAt(0xa483d464, &GSDrawScanline::DrawScanlineExT<0xa483d464>);
	m_dsmap.SetAt(0xa483dc24, &GSDrawScanline::DrawScanlineExT<0xa483dc24>);
	m_dsmap.SetAt(0xa683d464, &GSDrawScanline::DrawScanlineExT<0xa683d464>);
	m_dsmap.SetAt(0xe443d464, &GSDrawScanline::DrawScanlineExT<0xe443d464>);
	m_dsmap.SetAt(0xe443d564, &GSDrawScanline::DrawScanlineExT<0xe443d564>);
	m_dsmap.SetAt(0xea83d464, &GSDrawScanline::DrawScanlineExT<0xea83d464>);
	m_dsmap.SetAt(0xea83d564, &GSDrawScanline::DrawScanlineExT<0xea83d564>);

	// gt4

	m_dsmap.SetAt(0x0ff02201, &GSDrawScanline::DrawScanlineExT<0x0ff02201>);
	m_dsmap.SetAt(0x0ff02c05, &GSDrawScanline::DrawScanlineExT<0x0ff02c05>);
	m_dsmap.SetAt(0x0ff03c44, &GSDrawScanline::DrawScanlineExT<0x0ff03c44>);
	m_dsmap.SetAt(0x24402241, &GSDrawScanline::DrawScanlineExT<0x24402241>);
	m_dsmap.SetAt(0x24402441, &GSDrawScanline::DrawScanlineExT<0x24402441>);
	m_dsmap.SetAt(0x25803c04, &GSDrawScanline::DrawScanlineExT<0x25803c04>);
	m_dsmap.SetAt(0x25843464, &GSDrawScanline::DrawScanlineExT<0x25843464>);
	m_dsmap.SetAt(0x258c3464, &GSDrawScanline::DrawScanlineExT<0x258c3464>);
	m_dsmap.SetAt(0x26402205, &GSDrawScanline::DrawScanlineExT<0x26402205>);
	m_dsmap.SetAt(0x26402245, &GSDrawScanline::DrawScanlineExT<0x26402245>);
	m_dsmap.SetAt(0x26403c05, &GSDrawScanline::DrawScanlineExT<0x26403c05>);
	m_dsmap.SetAt(0x2a902204, &GSDrawScanline::DrawScanlineExT<0x2a902204>);
	m_dsmap.SetAt(0x2ff02201, &GSDrawScanline::DrawScanlineExT<0x2ff02201>);
	m_dsmap.SetAt(0x2ff03446, &GSDrawScanline::DrawScanlineExT<0x2ff03446>);
	m_dsmap.SetAt(0x2ff42264, &GSDrawScanline::DrawScanlineExT<0x2ff42264>);
	m_dsmap.SetAt(0x6440ec14, &GSDrawScanline::DrawScanlineExT<0x6440ec14>);
	m_dsmap.SetAt(0x64443464, &GSDrawScanline::DrawScanlineExT<0x64443464>);
	m_dsmap.SetAt(0x8ff03c01, &GSDrawScanline::DrawScanlineExT<0x8ff03c01>);
	m_dsmap.SetAt(0x8ff03c44, &GSDrawScanline::DrawScanlineExT<0x8ff03c44>);
	m_dsmap.SetAt(0xa4402445, &GSDrawScanline::DrawScanlineExT<0xa4402445>);
	m_dsmap.SetAt(0xa4403441, &GSDrawScanline::DrawScanlineExT<0xa4403441>);
	m_dsmap.SetAt(0xa440d445, &GSDrawScanline::DrawScanlineExT<0xa440d445>);
	m_dsmap.SetAt(0xa44435e4, &GSDrawScanline::DrawScanlineExT<0xa44435e4>);
	m_dsmap.SetAt(0xa5843464, &GSDrawScanline::DrawScanlineExT<0xa5843464>);
	m_dsmap.SetAt(0xa6802c00, &GSDrawScanline::DrawScanlineExT<0xa6802c00>);
	m_dsmap.SetAt(0xaff02c00, &GSDrawScanline::DrawScanlineExT<0xaff02c00>);
	m_dsmap.SetAt(0xaff02c81, &GSDrawScanline::DrawScanlineExT<0xaff02c81>);
	m_dsmap.SetAt(0xaff43464, &GSDrawScanline::DrawScanlineExT<0xaff43464>);
	m_dsmap.SetAt(0xcff4c464, &GSDrawScanline::DrawScanlineExT<0xcff4c464>);
	m_dsmap.SetAt(0x24802244, &GSDrawScanline::DrawScanlineExT<0x24802244>);
	m_dsmap.SetAt(0xa4403465, &GSDrawScanline::DrawScanlineExT<0xa4403465>);
	m_dsmap.SetAt(0xa4442464, &GSDrawScanline::DrawScanlineExT<0xa4442464>);

	// ico

	m_dsmap.SetAt(0x0ff03d00, &GSDrawScanline::DrawScanlineExT<0x0ff03d00>);
	m_dsmap.SetAt(0x0ff05c00, &GSDrawScanline::DrawScanlineExT<0x0ff05c00>);
	m_dsmap.SetAt(0x24402200, &GSDrawScanline::DrawScanlineExT<0x24402200>);
	m_dsmap.SetAt(0x24403c00, &GSDrawScanline::DrawScanlineExT<0x24403c00>);
	m_dsmap.SetAt(0x2448dc00, &GSDrawScanline::DrawScanlineExT<0x2448dc00>);
	m_dsmap.SetAt(0x24802200, &GSDrawScanline::DrawScanlineExT<0x24802200>);
	m_dsmap.SetAt(0x24802220, &GSDrawScanline::DrawScanlineExT<0x24802220>);
	m_dsmap.SetAt(0x26802200, &GSDrawScanline::DrawScanlineExT<0x26802200>);
	m_dsmap.SetAt(0x26802220, &GSDrawScanline::DrawScanlineExT<0x26802220>);
	m_dsmap.SetAt(0x26803c00, &GSDrawScanline::DrawScanlineExT<0x26803c00>);
	m_dsmap.SetAt(0x4ff02220, &GSDrawScanline::DrawScanlineExT<0x4ff02220>);
	m_dsmap.SetAt(0x64402210, &GSDrawScanline::DrawScanlineExT<0x64402210>);
	m_dsmap.SetAt(0x64802260, &GSDrawScanline::DrawScanlineExT<0x64802260>);
	m_dsmap.SetAt(0xa4402c20, &GSDrawScanline::DrawScanlineExT<0xa4402c20>);
	m_dsmap.SetAt(0xa4403c00, &GSDrawScanline::DrawScanlineExT<0xa4403c00>);
	m_dsmap.SetAt(0xa441dc20, &GSDrawScanline::DrawScanlineExT<0xa441dc20>);
	m_dsmap.SetAt(0xa481d460, &GSDrawScanline::DrawScanlineExT<0xa481d460>);
	m_dsmap.SetAt(0xa621d460, &GSDrawScanline::DrawScanlineExT<0xa621d460>);
	m_dsmap.SetAt(0xa681d460, &GSDrawScanline::DrawScanlineExT<0xa681d460>);
	m_dsmap.SetAt(0xcff1d460, &GSDrawScanline::DrawScanlineExT<0xcff1d460>);
	m_dsmap.SetAt(0xe441d460, &GSDrawScanline::DrawScanlineExT<0xe441d460>);
	m_dsmap.SetAt(0xe481d460, &GSDrawScanline::DrawScanlineExT<0xe481d460>);

	// kuon

	m_dsmap.SetAt(0x0ff02202, &GSDrawScanline::DrawScanlineExT<0x0ff02202>);
	m_dsmap.SetAt(0x24402202, &GSDrawScanline::DrawScanlineExT<0x24402202>);
	m_dsmap.SetAt(0x24430215, &GSDrawScanline::DrawScanlineExT<0x24430215>);
	m_dsmap.SetAt(0x26203465, &GSDrawScanline::DrawScanlineExT<0x26203465>);
	m_dsmap.SetAt(0x2a403801, &GSDrawScanline::DrawScanlineExT<0x2a403801>);
	m_dsmap.SetAt(0x2ff03801, &GSDrawScanline::DrawScanlineExT<0x2ff03801>);
	m_dsmap.SetAt(0x2ff03c02, &GSDrawScanline::DrawScanlineExT<0x2ff03c02>);
	m_dsmap.SetAt(0x2ff31c05, &GSDrawScanline::DrawScanlineExT<0x2ff31c05>);
	m_dsmap.SetAt(0x66403c15, &GSDrawScanline::DrawScanlineExT<0x66403c15>);
	m_dsmap.SetAt(0x66803815, &GSDrawScanline::DrawScanlineExT<0x66803815>);
	m_dsmap.SetAt(0x6ff02215, &GSDrawScanline::DrawScanlineExT<0x6ff02215>);
	m_dsmap.SetAt(0xa4411c15, &GSDrawScanline::DrawScanlineExT<0xa4411c15>);
	m_dsmap.SetAt(0xa4431815, &GSDrawScanline::DrawScanlineExT<0xa4431815>);
	m_dsmap.SetAt(0xa4831065, &GSDrawScanline::DrawScanlineExT<0xa4831065>);
	m_dsmap.SetAt(0xa4831455, &GSDrawScanline::DrawScanlineExT<0xa4831455>);
	m_dsmap.SetAt(0xe445d065, &GSDrawScanline::DrawScanlineExT<0xe445d065>);
	m_dsmap.SetAt(0xe445d465, &GSDrawScanline::DrawScanlineExT<0xe445d465>);
	m_dsmap.SetAt(0xeff5d065, &GSDrawScanline::DrawScanlineExT<0xeff5d065>);

	// guitar hero

	m_dsmap.SetAt(0x0ff038aa, &GSDrawScanline::DrawScanlineExT<0x0ff038aa>);
	m_dsmap.SetAt(0x2442c24a, &GSDrawScanline::DrawScanlineExT<0x2442c24a>);
	m_dsmap.SetAt(0x2442c26a, &GSDrawScanline::DrawScanlineExT<0x2442c26a>);
	m_dsmap.SetAt(0x2442d44a, &GSDrawScanline::DrawScanlineExT<0x2442d44a>);
	m_dsmap.SetAt(0x2480226a, &GSDrawScanline::DrawScanlineExT<0x2480226a>);
	m_dsmap.SetAt(0x2680224a, &GSDrawScanline::DrawScanlineExT<0x2680224a>);
	m_dsmap.SetAt(0x2680226a, &GSDrawScanline::DrawScanlineExT<0x2680226a>);
	m_dsmap.SetAt(0x2680390a, &GSDrawScanline::DrawScanlineExT<0x2680390a>);
	m_dsmap.SetAt(0x2a80390a, &GSDrawScanline::DrawScanlineExT<0x2a80390a>);
	m_dsmap.SetAt(0x4ff03c9a, &GSDrawScanline::DrawScanlineExT<0x4ff03c9a>);
	m_dsmap.SetAt(0x6442c27a, &GSDrawScanline::DrawScanlineExT<0x6442c27a>);
	m_dsmap.SetAt(0x6444d47a, &GSDrawScanline::DrawScanlineExT<0x6444d47a>);
	m_dsmap.SetAt(0x6446c27a, &GSDrawScanline::DrawScanlineExT<0x6446c27a>);
	m_dsmap.SetAt(0xa440d46a, &GSDrawScanline::DrawScanlineExT<0xa440d46a>);
	m_dsmap.SetAt(0xa442d44a, &GSDrawScanline::DrawScanlineExT<0xa442d44a>);
	m_dsmap.SetAt(0xa442d46a, &GSDrawScanline::DrawScanlineExT<0xa442d46a>);
	m_dsmap.SetAt(0xa446d46a, &GSDrawScanline::DrawScanlineExT<0xa446d46a>);
	m_dsmap.SetAt(0xa480344a, &GSDrawScanline::DrawScanlineExT<0xa480344a>);
	m_dsmap.SetAt(0xa480346a, &GSDrawScanline::DrawScanlineExT<0xa480346a>);
	m_dsmap.SetAt(0xa48034ea, &GSDrawScanline::DrawScanlineExT<0xa48034ea>);
	m_dsmap.SetAt(0xa480d46a, &GSDrawScanline::DrawScanlineExT<0xa480d46a>);
	m_dsmap.SetAt(0xa680344a, &GSDrawScanline::DrawScanlineExT<0xa680344a>);
	m_dsmap.SetAt(0xa68034ea, &GSDrawScanline::DrawScanlineExT<0xa68034ea>);
	m_dsmap.SetAt(0xa680356a, &GSDrawScanline::DrawScanlineExT<0xa680356a>);
	m_dsmap.SetAt(0xa6803c2a, &GSDrawScanline::DrawScanlineExT<0xa6803c2a>);
	m_dsmap.SetAt(0xa680d44a, &GSDrawScanline::DrawScanlineExT<0xa680d44a>);
	m_dsmap.SetAt(0xa684356a, &GSDrawScanline::DrawScanlineExT<0xa684356a>);
	m_dsmap.SetAt(0xcff0347a, &GSDrawScanline::DrawScanlineExT<0xcff0347a>);
	m_dsmap.SetAt(0xcff034ea, &GSDrawScanline::DrawScanlineExT<0xcff034ea>);
	m_dsmap.SetAt(0xcff034fa, &GSDrawScanline::DrawScanlineExT<0xcff034fa>);
	m_dsmap.SetAt(0xcff0d47a, &GSDrawScanline::DrawScanlineExT<0xcff0d47a>);
	m_dsmap.SetAt(0xe440d45a, &GSDrawScanline::DrawScanlineExT<0xe440d45a>);
	m_dsmap.SetAt(0xe442d45a, &GSDrawScanline::DrawScanlineExT<0xe442d45a>);
	m_dsmap.SetAt(0xe442d46a, &GSDrawScanline::DrawScanlineExT<0xe442d46a>);
	m_dsmap.SetAt(0xe442d47a, &GSDrawScanline::DrawScanlineExT<0xe442d47a>);
	m_dsmap.SetAt(0xe446d47a, &GSDrawScanline::DrawScanlineExT<0xe446d47a>);
	m_dsmap.SetAt(0xe480345a, &GSDrawScanline::DrawScanlineExT<0xe480345a>);

	// virtual tennis 2

	m_dsmap.SetAt(0x0ff10215, &GSDrawScanline::DrawScanlineExT<0x0ff10215>);
	m_dsmap.SetAt(0x24231065, &GSDrawScanline::DrawScanlineExT<0x24231065>);
	m_dsmap.SetAt(0x28830215, &GSDrawScanline::DrawScanlineExT<0x28830215>);
	m_dsmap.SetAt(0x2aa30215, &GSDrawScanline::DrawScanlineExT<0x2aa30215>);
	m_dsmap.SetAt(0x4ff0e265, &GSDrawScanline::DrawScanlineExT<0x4ff0e265>);
	m_dsmap.SetAt(0x4ff20215, &GSDrawScanline::DrawScanlineExT<0x4ff20215>);
	m_dsmap.SetAt(0x6440e265, &GSDrawScanline::DrawScanlineExT<0x6440e265>);
	m_dsmap.SetAt(0x6480e265, &GSDrawScanline::DrawScanlineExT<0x6480e265>);
	m_dsmap.SetAt(0x66402c15, &GSDrawScanline::DrawScanlineExT<0x66402c15>);
	m_dsmap.SetAt(0xa680f445, &GSDrawScanline::DrawScanlineExT<0xa680f445>);
	m_dsmap.SetAt(0xcff0f475, &GSDrawScanline::DrawScanlineExT<0xcff0f475>);
	m_dsmap.SetAt(0xe440ec65, &GSDrawScanline::DrawScanlineExT<0xe440ec65>);
	m_dsmap.SetAt(0xe440ede5, &GSDrawScanline::DrawScanlineExT<0xe440ede5>);
	m_dsmap.SetAt(0xe440f465, &GSDrawScanline::DrawScanlineExT<0xe440f465>);
	m_dsmap.SetAt(0xe440f475, &GSDrawScanline::DrawScanlineExT<0xe440f475>);
	m_dsmap.SetAt(0xe640f475, &GSDrawScanline::DrawScanlineExT<0xe640f475>);
	m_dsmap.SetAt(0xe680f475, &GSDrawScanline::DrawScanlineExT<0xe680f475>);

	// bios

	m_dsmap.SetAt(0x0ff03404, &GSDrawScanline::DrawScanlineExT<0x0ff03404>);
	m_dsmap.SetAt(0x0ff03c24, &GSDrawScanline::DrawScanlineExT<0x0ff03c24>);
	m_dsmap.SetAt(0x24403c24, &GSDrawScanline::DrawScanlineExT<0x24403c24>);
	m_dsmap.SetAt(0x24803404, &GSDrawScanline::DrawScanlineExT<0x24803404>);
	m_dsmap.SetAt(0x24803444, &GSDrawScanline::DrawScanlineExT<0x24803444>);
	m_dsmap.SetAt(0x25803404, &GSDrawScanline::DrawScanlineExT<0x25803404>);
	m_dsmap.SetAt(0x26202204, &GSDrawScanline::DrawScanlineExT<0x26202204>);
	m_dsmap.SetAt(0x26403c04, &GSDrawScanline::DrawScanlineExT<0x26403c04>);
	m_dsmap.SetAt(0x26803464, &GSDrawScanline::DrawScanlineExT<0x26803464>);
	m_dsmap.SetAt(0x4ff02230, &GSDrawScanline::DrawScanlineExT<0x4ff02230>);
	m_dsmap.SetAt(0x4ff02250, &GSDrawScanline::DrawScanlineExT<0x4ff02250>);
	m_dsmap.SetAt(0x4ff02260, &GSDrawScanline::DrawScanlineExT<0x4ff02260>);
	m_dsmap.SetAt(0x4ff03020, &GSDrawScanline::DrawScanlineExT<0x4ff03020>);
	m_dsmap.SetAt(0x4ff03060, &GSDrawScanline::DrawScanlineExT<0x4ff03060>);
	m_dsmap.SetAt(0x4ff03464, &GSDrawScanline::DrawScanlineExT<0x4ff03464>);
	m_dsmap.SetAt(0x4ff03470, &GSDrawScanline::DrawScanlineExT<0x4ff03470>);
	m_dsmap.SetAt(0x4ff03c10, &GSDrawScanline::DrawScanlineExT<0x4ff03c10>);
	m_dsmap.SetAt(0x4ff03c20, &GSDrawScanline::DrawScanlineExT<0x4ff03c20>);
	m_dsmap.SetAt(0x4ff03c30, &GSDrawScanline::DrawScanlineExT<0x4ff03c30>);
	m_dsmap.SetAt(0x60002060, &GSDrawScanline::DrawScanlineExT<0x60002060>);
	m_dsmap.SetAt(0x64203410, &GSDrawScanline::DrawScanlineExT<0x64203410>);
	m_dsmap.SetAt(0x64203420, &GSDrawScanline::DrawScanlineExT<0x64203420>);
	m_dsmap.SetAt(0x64402260, &GSDrawScanline::DrawScanlineExT<0x64402260>);
	m_dsmap.SetAt(0x64403c10, &GSDrawScanline::DrawScanlineExT<0x64403c10>);
	m_dsmap.SetAt(0x64803420, &GSDrawScanline::DrawScanlineExT<0x64803420>);
	m_dsmap.SetAt(0x64803c10, &GSDrawScanline::DrawScanlineExT<0x64803c10>);
	m_dsmap.SetAt(0x66202c10, &GSDrawScanline::DrawScanlineExT<0x66202c10>);
	m_dsmap.SetAt(0x66402210, &GSDrawScanline::DrawScanlineExT<0x66402210>);
	m_dsmap.SetAt(0x66402250, &GSDrawScanline::DrawScanlineExT<0x66402250>);
	m_dsmap.SetAt(0x66802c10, &GSDrawScanline::DrawScanlineExT<0x66802c10>);
	m_dsmap.SetAt(0xe440dc10, &GSDrawScanline::DrawScanlineExT<0xe440dc10>);

	// one piece grand battle 3

	m_dsmap.SetAt(0x0ff02c86, &GSDrawScanline::DrawScanlineExT<0x0ff02c86>);
	m_dsmap.SetAt(0x2440c264, &GSDrawScanline::DrawScanlineExT<0x2440c264>);
	m_dsmap.SetAt(0x25602204, &GSDrawScanline::DrawScanlineExT<0x25602204>);
	m_dsmap.SetAt(0x26202c84, &GSDrawScanline::DrawScanlineExT<0x26202c84>);
	m_dsmap.SetAt(0x26204c84, &GSDrawScanline::DrawScanlineExT<0x26204c84>);
	m_dsmap.SetAt(0x2990cc84, &GSDrawScanline::DrawScanlineExT<0x2990cc84>);
	m_dsmap.SetAt(0x64402254, &GSDrawScanline::DrawScanlineExT<0x64402254>);
	m_dsmap.SetAt(0x64403454, &GSDrawScanline::DrawScanlineExT<0x64403454>);
	m_dsmap.SetAt(0x64482254, &GSDrawScanline::DrawScanlineExT<0x64482254>);
	m_dsmap.SetAt(0x65402254, &GSDrawScanline::DrawScanlineExT<0x65402254>);
	m_dsmap.SetAt(0x66220214, &GSDrawScanline::DrawScanlineExT<0x66220214>);
	m_dsmap.SetAt(0x6ff02254, &GSDrawScanline::DrawScanlineExT<0x6ff02254>);
	m_dsmap.SetAt(0xa4403444, &GSDrawScanline::DrawScanlineExT<0xa4403444>);
	m_dsmap.SetAt(0xe0a03464, &GSDrawScanline::DrawScanlineExT<0xe0a03464>);
	m_dsmap.SetAt(0xe4403454, &GSDrawScanline::DrawScanlineExT<0xe4403454>);
	m_dsmap.SetAt(0xe4483454, &GSDrawScanline::DrawScanlineExT<0xe4483454>);
	m_dsmap.SetAt(0xe4803454, &GSDrawScanline::DrawScanlineExT<0xe4803454>);
	m_dsmap.SetAt(0xeff03454, &GSDrawScanline::DrawScanlineExT<0xeff03454>);

	// mana khemia

	m_dsmap.SetAt(0x24402209, &GSDrawScanline::DrawScanlineExT<0x24402209>);
	m_dsmap.SetAt(0x24402c09, &GSDrawScanline::DrawScanlineExT<0x24402c09>);
	m_dsmap.SetAt(0x24403c02, &GSDrawScanline::DrawScanlineExT<0x24403c02>);
	m_dsmap.SetAt(0x4ff02219, &GSDrawScanline::DrawScanlineExT<0x4ff02219>);
	m_dsmap.SetAt(0x64402c19, &GSDrawScanline::DrawScanlineExT<0x64402c19>);
	m_dsmap.SetAt(0x64403c29, &GSDrawScanline::DrawScanlineExT<0x64403c29>);
	m_dsmap.SetAt(0xa4402c09, &GSDrawScanline::DrawScanlineExT<0xa4402c09>);
	m_dsmap.SetAt(0xa4403c09, &GSDrawScanline::DrawScanlineExT<0xa4403c09>);
	m_dsmap.SetAt(0xa4442c09, &GSDrawScanline::DrawScanlineExT<0xa4442c09>);
	m_dsmap.SetAt(0xe445b469, &GSDrawScanline::DrawScanlineExT<0xe445b469>);
	m_dsmap.SetAt(0xe445bc29, &GSDrawScanline::DrawScanlineExT<0xe445bc29>);

	// enthusai

	m_dsmap.SetAt(0x0ff11894, &GSDrawScanline::DrawScanlineExT<0x0ff11894>);
	m_dsmap.SetAt(0x24403801, &GSDrawScanline::DrawScanlineExT<0x24403801>);
	m_dsmap.SetAt(0x24430224, &GSDrawScanline::DrawScanlineExT<0x24430224>);
	m_dsmap.SetAt(0x24831064, &GSDrawScanline::DrawScanlineExT<0x24831064>);
	m_dsmap.SetAt(0x25831064, &GSDrawScanline::DrawScanlineExT<0x25831064>);
	m_dsmap.SetAt(0x258b1064, &GSDrawScanline::DrawScanlineExT<0x258b1064>);
	m_dsmap.SetAt(0x26a10214, &GSDrawScanline::DrawScanlineExT<0x26a10214>);
	m_dsmap.SetAt(0x2ff31098, &GSDrawScanline::DrawScanlineExT<0x2ff31098>);
	m_dsmap.SetAt(0x4ff42224, &GSDrawScanline::DrawScanlineExT<0x4ff42224>);
	m_dsmap.SetAt(0x6441a224, &GSDrawScanline::DrawScanlineExT<0x6441a224>);
	m_dsmap.SetAt(0x6441a264, &GSDrawScanline::DrawScanlineExT<0x6441a264>);
	m_dsmap.SetAt(0x64434224, &GSDrawScanline::DrawScanlineExT<0x64434224>);
	m_dsmap.SetAt(0x64434264, &GSDrawScanline::DrawScanlineExT<0x64434264>);
	m_dsmap.SetAt(0xa4430c24, &GSDrawScanline::DrawScanlineExT<0xa4430c24>);
	m_dsmap.SetAt(0xa4471064, &GSDrawScanline::DrawScanlineExT<0xa4471064>);
	m_dsmap.SetAt(0xa4471464, &GSDrawScanline::DrawScanlineExT<0xa4471464>);
	m_dsmap.SetAt(0xa4831064, &GSDrawScanline::DrawScanlineExT<0xa4831064>);
	m_dsmap.SetAt(0xa5231454, &GSDrawScanline::DrawScanlineExT<0xa5231454>);
	m_dsmap.SetAt(0xa5831064, &GSDrawScanline::DrawScanlineExT<0xa5831064>);
	m_dsmap.SetAt(0xa5831454, &GSDrawScanline::DrawScanlineExT<0xa5831454>);
	m_dsmap.SetAt(0xcff02468, &GSDrawScanline::DrawScanlineExT<0xcff02468>);
	m_dsmap.SetAt(0xcff0a068, &GSDrawScanline::DrawScanlineExT<0xcff0a068>);
	m_dsmap.SetAt(0xcff1bc24, &GSDrawScanline::DrawScanlineExT<0xcff1bc24>);
	m_dsmap.SetAt(0xcff42468, &GSDrawScanline::DrawScanlineExT<0xcff42468>);
	m_dsmap.SetAt(0xcff4a068, &GSDrawScanline::DrawScanlineExT<0xcff4a068>);
	m_dsmap.SetAt(0xcff4b064, &GSDrawScanline::DrawScanlineExT<0xcff4b064>);
	m_dsmap.SetAt(0xe441bc24, &GSDrawScanline::DrawScanlineExT<0xe441bc24>);
	m_dsmap.SetAt(0xe4434c24, &GSDrawScanline::DrawScanlineExT<0xe4434c24>);
	m_dsmap.SetAt(0xe443a468, &GSDrawScanline::DrawScanlineExT<0xe443a468>);
	m_dsmap.SetAt(0xe443b064, &GSDrawScanline::DrawScanlineExT<0xe443b064>);
	m_dsmap.SetAt(0xe447b064, &GSDrawScanline::DrawScanlineExT<0xe447b064>);
	m_dsmap.SetAt(0xe447b464, &GSDrawScanline::DrawScanlineExT<0xe447b464>);

	// ar tonelico

	m_dsmap.SetAt(0xa4802c09, &GSDrawScanline::DrawScanlineExT<0xa4802c09>);
	m_dsmap.SetAt(0xa485bc29, &GSDrawScanline::DrawScanlineExT<0xa485bc29>);
	m_dsmap.SetAt(0xe441bc29, &GSDrawScanline::DrawScanlineExT<0xe441bc29>);
/*
	// dmc (fixme)

	// mgs3s1

	// nfs mw

	// wild arms 5

	// rouge galaxy

	// God of War

	// dbzbt2
*/
}

template<DWORD sel>
void GSDrawScanline::DrawScanlineExT(int top, int left, int right, const Vertex& v, const Vertex& dv)
{
	const DWORD fpsm = (sel >> 0) & 3;
	const DWORD zpsm = (sel >> 2) & 3;
	const DWORD ztst = (sel >> 4) & 3;
	const DWORD iip = (sel >> 6) & 1;
	const DWORD tfx = (sel >> 7) & 7;
	const DWORD tcc = (sel >> 10) & 1;
	const DWORD fst = (sel >> 11) & 1;
	const DWORD ltf = (sel >> 12) & 1;
	const DWORD atst = (sel >> 13) & 7;
	const DWORD afail = (sel >> 16) & 3;
	const DWORD fge = (sel >> 18) & 1;
	const DWORD date = (sel >> 19) & 1;
	const DWORD abe = (sel >> 20) & 255;
	const DWORD abea = (sel >> 20) & 3;
	const DWORD abeb = (sel >> 22) & 3;
	const DWORD abec = (sel >> 24) & 3;
	const DWORD abed = (sel >> 26) & 3;
	const DWORD pabe = (sel >> 28) & 1;
	const DWORD rfb = (sel >> 29) & 1;
	const DWORD wzb = (sel >> 30) & 1;
	const DWORD tlu = (sel >> 31) & 1;

	GSVector4i fa_base = m_slenv.fbr[top];
	GSVector4i* fa_offset = (GSVector4i*)&m_slenv.fbc[left & 3][left];

	GSVector4i za_base = m_slenv.zbr[top];
	GSVector4i* za_offset = (GSVector4i*)&m_slenv.zbc[left & 3][left];

	GSVector4 ps0123 = GSVector4::ps0123();

	GSVector4 vp = v.p;
	GSVector4 dp = dv.p;
	GSVector4 z = vp.zzzz(); z += dp.zzzz() * ps0123;
	GSVector4 f = vp.wwww(); f += dp.wwww() * ps0123;

	GSVector4 vt = v.t;
	GSVector4 dt = dv.t;
	GSVector4 s = vt.xxxx(); s += dt.xxxx() * ps0123;
	GSVector4 t = vt.yyyy(); t += dt.yyyy() * ps0123;
	GSVector4 q = vt.zzzz(); q += dt.zzzz() * ps0123;

	GSVector4 vc = v.c;
	GSVector4 dc = dv.c;
	GSVector4 r = vc.xxxx(); if(iip) r += dc.xxxx() * ps0123;
	GSVector4 g = vc.yyyy(); if(iip) g += dc.yyyy() * ps0123;
	GSVector4 b = vc.zzzz(); if(iip) b += dc.zzzz() * ps0123;
	GSVector4 a = vc.wwww(); if(iip) a += dc.wwww() * ps0123;

	int steps = right - left;

	while(1)
	{
		do
		{
			GSVector4i za = za_base + GSVector4i::load<true>(za_offset);
			
			GSVector4i zs = (GSVector4i(z * 0.5f) << 1) | (GSVector4i(z) & GSVector4i::x00000001(za));

			GSVector4i test;

			if(!TestZ(zpsm, ztst, zs, za, test))
			{
				continue;
			}

			int pixels = GSVector4i::store(GSVector4i::load(steps).min_i16(GSVector4i::load(4)));

			GSVector4 c[12];

			if(tfx != TFX_NONE)
			{
				GSVector4 u = s;
				GSVector4 v = t;

				if(!fst)
				{
					GSVector4 w = q.rcp();

					u *= w;
					v *= w;

					if(ltf)
					{
						u -= 0.5f;
						v -= 0.5f;
					}
				}

				SampleTexture(pixels, ztst, ltf, tlu, test, u, v, c);
			}

			AlphaTFX(tfx, tcc, a, c[3]);

			GSVector4i fm = m_slenv.fm;
			GSVector4i zm = m_slenv.zm;

			if(!TestAlpha(atst, afail, c[3], fm, zm, test))
			{
				continue;
			}

			ColorTFX(tfx, r, g, b, a, c[0], c[1], c[2]);

			if(fge)
			{
				Fog(f, c[0], c[1], c[2]);
			}

			GSVector4i fa = fa_base + GSVector4i::load<true>(fa_offset);

			GSVector4i d = GSVector4i::zero();

			if(rfb)
			{
				d = ReadFrameX(fpsm == 1 ? 0 : fpsm, fa);

				if(fpsm != 1 && date)
				{
					test |= (d ^ m_slenv.datm).sra32(31);

					if(test.alltrue())
					{
						continue;
					}
				}
			}

			fm |= test;
			zm |= test;

			if(abe != 255)
			{
	//			GSVector4::expand(d, c[4], c[5], c[6], c[7]);

				c[4] = (d << 24) >> 24;
				c[5] = (d << 16) >> 24;
				c[6] = (d <<  8) >> 24;
				c[7] = (d >> 24);

				if(fpsm == 1)
				{
					c[7] = GSVector4(128.0f);
				}

				c[8] = GSVector4::zero();
				c[9] = GSVector4::zero();
				c[10] = GSVector4::zero();
				c[11] = m_slenv.afix;

				/*
				GSVector4 r = (c[abea*4 + 0] - c[abeb*4 + 0]).mod2x(c[abec*4 + 3]) + c[abed*4 + 0];
				GSVector4 g = (c[abea*4 + 1] - c[abeb*4 + 1]).mod2x(c[abec*4 + 3]) + c[abed*4 + 1];
				GSVector4 b = (c[abea*4 + 2] - c[abeb*4 + 2]).mod2x(c[abec*4 + 3]) + c[abed*4 + 2];
				*/

				GSVector4 r, g, b; 

				if(abea != abeb)
				{
					r = c[abea*4 + 0];
					g = c[abea*4 + 1];
					b = c[abea*4 + 2];

					if(abeb != 2)
					{
						r -= c[abeb*4 + 0];
						g -= c[abeb*4 + 1];
						b -= c[abeb*4 + 2];
					}

					if(!(fpsm == 1 && abec == 1))
					{
						if(abec == 2)
						{
							r *= m_slenv.afix2;
							g *= m_slenv.afix2;
							b *= m_slenv.afix2;
						}
						else
						{
							r = r.mod2x(c[abec*4 + 3]);
							g = g.mod2x(c[abec*4 + 3]);
							b = b.mod2x(c[abec*4 + 3]);
						}
					}

					if(abed < 2)
					{
						r += c[abed*4 + 0];
						g += c[abed*4 + 1];
						b += c[abed*4 + 2];
					}
				}
				else
				{
					r = c[abed*4 + 0];
					g = c[abed*4 + 1];
					b = c[abed*4 + 2];
				}

				if(pabe)
				{
					GSVector4 mask = c[3] >= GSVector4(128.0f);

					c[0] = c[0].blend8(r, mask);
					c[1] = c[1].blend8(g, mask);
					c[2] = c[2].blend8(b, mask);
				}
				else
				{
					c[0] = r;
					c[1] = g;
					c[2] = b;
				}
			}

			GSVector4i rb = GSVector4i(c[0]).ps32(GSVector4i(c[2]));
			GSVector4i ga = GSVector4i(c[1]).ps32(GSVector4i(c[3]));
			
			GSVector4i rg = rb.upl16(ga) & m_slenv.colclamp;
			GSVector4i ba = rb.uph16(ga) & m_slenv.colclamp;
			
			GSVector4i s = rg.upl32(ba).pu16(rg.uph32(ba));

			if(fpsm != 1)
			{
				s |= m_slenv.fba;
			}

			if(rfb)
			{
				s = s.blend(d, fm);
			}

			WriteFrameAndZBufX(fpsm == 1 && rfb ? 0 : fpsm, fa, fm, s, wzb ? zpsm : 3, za, zm, zs, pixels);
		}
		while(0);

		if(steps <= 4) break;

		steps -= 4;

		fa_offset++;
		za_offset++;

		GSVector4 dp4 = dv.p * 4.0f;

		z += dp4.zzzz();
		f += dp4.wwww();

		GSVector4 dt4 = dv.t * 4.0f;

		s += dt4.xxxx();
		t += dt4.yyyy();
		q += dt4.zzzz();

		GSVector4 dc4 = dv.c * 4.0f;

		if(iip) r += dc4.xxxx();
		if(iip) g += dc4.yyyy();
		if(iip) b += dc4.zzzz();
		if(iip) a += dc4.wwww();
	}
}
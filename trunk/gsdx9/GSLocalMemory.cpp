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
 *	Special Notes: 
 *
 *	Based on Page.c from GSSoft
 *	Copyright (C) 2002-2004 GSsoft Team
 *
 */
 
#include "StdAfx.h"
#include "GSLocalMemory.h"
#include "x86.h"

//

static class cdscd
{
	CString toBin(UINT64 n, int bits)
	{
		CString str;
		while(bits-- > 0) str += n&(1i64<<bits) ? '1' : '0';
		return str;
	}
public:
	cdscd()
	{
		return;
/*
		union
		{
			struct {DWORD mant:23; DWORD exp:8; DWORD sign:1;} fi;
			DWORD i;
			float f;
		} nf, nf2;

		union
		{
			struct {UINT64 mant:52; UINT64 exp:11; UINT64 sign:1;} di;
			UINT64 i;
			double d;
		} nd;

		unsigned int control_word = 0;
		int err;
		
//		nd.d = 0;
//		TRACE(_T("%016I64x (%I64x:%I64x:%I64x), %f\n"), nd.i, nd.di.sign, nd.di.exp, nd.di.mant, nd.d);

//		nf.f = nd.d;
//		TRACE(_T("%08x (%x:%x:%x), %f\n"), nf.i, nf.fi.sign, nf.fi.exp, nf.fi.mant, nf.f);

		nf2.fi.sign = 0;
		nf2.fi.exp = 0x7f;
		nf2.fi.mant = 1;

		//err = _controlfp_s(&control_word, _RC_CHOP, _MCW_RC);

		nf.f = 0;

		for(int i = 0; i < 20; i++)
		{
			nd.d = (double)nf.f + (double)nf2.f;
			TRACE(_T("%016I64x (%I64x:%03I64x:%s), %f\n"), nd.i, nd.di.sign, nd.di.exp, toBin(nd.di.mant, 52), nd.d);
			*((UINT64*)&nd.d) &= ~(1ui64<<(52-24));
			TRACE(_T("%016I64x (%I64x:%03I64x:%s), %f\n"), nd.i, nd.di.sign, nd.di.exp, toBin(nd.di.mant, 52), nd.d);
			nf.f = (float)nd.d;
			TRACE(_T("%08x         (%x:%02x :%s                             ), %f\n"), nf.i, nf.fi.sign, nf.fi.exp, toBin(nf.fi.mant, 23), nf.f);
		}
*/

		GSLocalMemory lm;

		for(int i = 0; i < 1024*1024; i++)
			((DWORD*)lm.GetVM())[i] = rand()*0x12345678;

/*
		GIFRegTEX0 TEX0;
		TEX0.TBP0 = 0;
		TEX0.TBW = 16;
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1

		__declspec(align(32)) static DWORD dst[1024*1024];

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				lm.unSwizzleTexture16(1024, 1024, (BYTE*)dst, 1024*4, TEX0, TEXA);
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
*/
/*
		__declspec(align(16)) static vertex_t v[100000];
		for(int i = 0; i < countof(v); i++)
		{
			v[i].u = 1.0f*rand() - RAND_MAX/2;
			v[i].v = 1.0f*rand() - RAND_MAX/2;
		}

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			uvmm_t uvmm;
			for(int i = 0; i < 1000; i++)
				UVMinMax_sse2(countof(v), v, &uvmm);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d, u %.2f %.2f, v %.2f %.2f"), diff, uvmm.umin, uvmm.umax, uvmm.vmin, uvmm.vmax);
			AfxMessageBox(str);
		}
*/
/*
		GIFRegTEX0 TEX0;
		GIFRegTEXCLUT TEXCLUT;
		TEX0.PSM = PSM_PSMT8;
		TEX0.CLD = 1;
		TEX0.CSA = 0;
		TEX0.CPSM = PSM_PSMCT32; // PSM_PSMCT16S
		TEX0.CBP = 0;
		TEX0.CSM = 0; // 0
		TEXCLUT.CBW = 1;
		TEXCLUT.COU = 0;
		TEXCLUT.COV = 0;

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 10000000; i++)
				lm.WriteCLUT(TEX0, TEXCLUT);

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
*/
	}
} sddscsd;

//

__forceinline static DWORD From24To32(DWORD c, BYTE TCC, GIFRegTEXA& TEXA)
{
	BYTE A = (!TEXA.AEM|(c&0xffffff)) ? TEXA.TA0 : 0;
	return (A<<24) | (c&0xffffff);
}

__forceinline static DWORD From16To32(WORD c, GIFRegTEXA& TEXA)
{
	BYTE A = (c&0x8000) ? TEXA.TA1 : (!TEXA.AEM|(c&0x7fff)) ? TEXA.TA0 : 0;
	return (A << 24) | ((c&0x7c00) << 9) | ((c&0x03e0) << 6) | ((c&0x001f) << 3);
}

//

#define ASSERT_BLOCK(r, w, h) \
	ASSERT((r).Width() >= w && (r).Height() >= w && !((r).left&(w-1)) && !((r).top&(h-1)) && !((r).right&(w-1)) && !((r).bottom&(h-1))); \

#define FOREACH_BLOCK(r, w, h) \
	ASSERT_BLOCK(r, w, h); \
	for(int y = (r).top; y < (r).bottom; y += (h), dst += dstpitch*(h)) \
		for(int x = (r).left; x < (r).right; x += (w)) \

#define FOREACH_BLOCK_OMP_START(r, w, h) \
	for(int y = (r).top; y < (r).bottom; y += (h)) \
	{ 	ASSERT_BLOCK(r, w, h); \
		BYTE* ompdst = dst + (y-(r).top)*dstpitch; \
		for(int x = (r).left; x < (r).right; x += (w)) \
		{ \

#define FOREACH_BLOCK_OMP_END }}

//

WORD GSLocalMemory::pageOffset32[32][32][64];
WORD GSLocalMemory::pageOffset32Z[32][32][64];
WORD GSLocalMemory::pageOffset16[32][64][64];
WORD GSLocalMemory::pageOffset16S[32][64][64];
WORD GSLocalMemory::pageOffset16Z[32][64][64];
WORD GSLocalMemory::pageOffset16SZ[32][64][64];
WORD GSLocalMemory::pageOffset8[32][64][128];
WORD GSLocalMemory::pageOffset4[32][128][128];

//

GSLocalMemory::GSLocalMemory()
{
	int len = 1024*1024*4*2; // *2 for safety...

	m_vm8 = (BYTE*)_aligned_malloc(len, 16);
	memset(m_vm8, 0, len);

	m_pCLUT = (WORD*)_aligned_malloc(256*2*sizeof(WORD)*2, 16);
	m_pCLUT32 = (DWORD*)_aligned_malloc(256*sizeof(DWORD), 16);
	m_pCLUT64 = (UINT64*)_aligned_malloc(256*sizeof(UINT64), 16);

	for(int bp = 0; bp < 32; bp++)
	{
		for(int y = 0; y < 32; y++) for(int x = 0; x < 64; x++)
		{
			pageOffset32[bp][y][x] = (WORD)pixelAddressOrg32(x, y, bp, 0);
			pageOffset32Z[bp][y][x] = (WORD)pixelAddressOrg32Z(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 64; x++) 
		{
			pageOffset16[bp][y][x] = (WORD)pixelAddressOrg16(x, y, bp, 0);
			pageOffset16S[bp][y][x] = (WORD)pixelAddressOrg16S(x, y, bp, 0);
			pageOffset16Z[bp][y][x] = (WORD)pixelAddressOrg16Z(x, y, bp, 0);
			pageOffset16SZ[bp][y][x] = (WORD)pixelAddressOrg16SZ(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset8[bp][y][x] = (WORD)pixelAddressOrg8(x, y, bp, 0);
		}

		for(int y = 0; y < 128; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset4[bp][y][x] = (WORD)pixelAddressOrg4(x, y, bp, 0);
		}
	}

	m_fCLUTMayBeDirty = true;
// 
/*
//	omp_set_num_threads(4);

	{
		for(int i = 0; i < 1024*1024; i++)
			((DWORD*)GetVM())[i] = rand()*0x12345678;

		GIFRegTEX0 TEX0;
		TEX0.TBP0 = 0;
		TEX0.TBW = 16;
		GIFRegTEXA TEXA;
		TEXA.AEM = 0; // 1

		__declspec(align(32)) static DWORD dst[1024*1024];

		for(int j = 0; j < 10; j++)
		{
			clock_t start = clock();

			for(int i = 0; i < 1000; i++)
			{
				unSwizzleTexture32(CRect(0,0,1024,1024), (BYTE*)dst, 1024*4, TEX0, TEXA);
			}

			clock_t diff = clock() - start;

			CString str;
			str.Format(_T("%d"), diff);
			AfxMessageBox(str);
		}
	}
*/
}

GSLocalMemory::~GSLocalMemory()
{
	_aligned_free(m_vm8);
	_aligned_free(m_pCLUT);
	_aligned_free(m_pCLUT32);
	_aligned_free(m_pCLUT64);	
}

////////////////////

CSize GSLocalMemory::GetBlockSize(DWORD PSM)
{
	CSize size;

	switch(PSM)
	{
	default:
	case PSM_PSMCT32: 
	case PSM_PSMCT24:
	case PSM_PSMT8H: 
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
	case PSM_PSMZ32:
	case PSM_PSMZ24:
		size.SetSize(8, 8); 
		break;
	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:
		size.SetSize(16, 8); 
		break;
	case PSM_PSMT8: 
		size.SetSize(16, 16);
		break;
	case PSM_PSMT4:
		size.SetSize(32, 16);
		break;
	}

	return size;
}

////////////////////

DWORD GSLocalMemory::pageAddress32(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 5) * bw + (x >> 6)) << 11; 
}

DWORD GSLocalMemory::pageAddress16(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * bw + (x >> 6)) << 12;
}

DWORD GSLocalMemory::pageAddress8(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7)) << 13; 
}

DWORD GSLocalMemory::pageAddress4(int x, int y, DWORD bp, DWORD bw)
{
	return ((bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7)) << 14;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPageAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pageAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pageAddress16; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pageAddress16; break;	
	case PSM_PSMT8: pa = &GSLocalMemory::pageAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pageAddress4; break;
	}

	return pa;
}

////////////////////

DWORD GSLocalMemory::blockAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD GSLocalMemory::blockAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	return (page + block) << 8;
}

DWORD GSLocalMemory::blockAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	return (page + block) << 9;
}

DWORD GSLocalMemory::blockAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	return (page + block) << 6;
}

DWORD GSLocalMemory::blockAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

DWORD GSLocalMemory::blockAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	return (page + block) << 7;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetBlockAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::blockAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::blockAddress16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::blockAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::blockAddress4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::blockAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::blockAddress32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::blockAddress32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::blockAddress16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::blockAddress16SZ; break;
	}
	
	return pa;
}

////////////////////

DWORD GSLocalMemory::pixelAddressOrg32(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + columnTable32[y & 7][x & 7];
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f);
	DWORD block = blockTable32[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + columnTable32[y & 7][x & 7];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16S(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + columnTable16[y & 7][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16S[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + columnTable16[y & 7][x & 15];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg8(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
//	DWORD block = (bp & 0x1f) + blockTable8[(y >> 4) & 3][(x >> 4) & 7];
//	DWORD word = (((page << 5) + block) << 8) + columnTable8[y & 15][x & 15];
	DWORD page = bp + ((y >> 1) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable8[(y >> 4) & 3][(x >> 4) & 7];
	DWORD word = ((page + block) << 8) + columnTable8[y & 15][x & 15];
//	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg4(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
//	DWORD block = (bp & 0x1f) + blockTable4[(y >> 4) & 7][(x >> 5) & 3];
//	DWORD word = (((page << 5) + block) << 9) + columnTable4[y & 15][x & 31];
	DWORD page = bp + ((y >> 2) & ~0x1f) * ((bw+1)>>1) + ((x >> 2) & ~0x1f); 
	DWORD block = blockTable4[(y >> 4) & 7][(x >> 5) & 3];
	DWORD word = ((page + block) << 9) + columnTable4[y & 15][x & 31];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg32Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
//	DWORD word = (((page << 5) + block) << 6) + ((y & 7) << 3) + (x & 7);
	DWORD page = bp + (y & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable32Z[(y >> 3) & 3][(x >> 3) & 7];
	DWORD word = ((page + block) << 6) + ((y & 7) << 3) + (x & 7);
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16Z(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16Z[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddressOrg16SZ(int x, int y, DWORD bp, DWORD bw)
{
//	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
//	DWORD block = (bp & 0x1f) + blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
//	DWORD word = (((page << 5) + block) << 7) + ((y & 7) << 4) + (x & 15);
	DWORD page = bp + ((y >> 1) & ~0x1f) * bw + ((x >> 1) & ~0x1f); 
	DWORD block = blockTable16SZ[(y >> 3) & 7][(x >> 4) & 3];
	DWORD word = ((page + block) << 7) + ((y & 7) << 4) + (x & 15);
	ASSERT(word < 1024*1024*2);
	return word;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPixelAddressOrg(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pixelAddressOrg16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pixelAddressOrg16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::pixelAddressOrg8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pixelAddressOrg4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pixelAddressOrg32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pixelAddressOrg32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pixelAddressOrg32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pixelAddressOrg16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pixelAddressOrg16SZ; break;
	}
	
	return pa;
}

////////////////////

DWORD GSLocalMemory::pixelAddress32(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddress16(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress16S(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16S[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress8(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * ((bw+1)>>1) + (x >> 7); 
	DWORD word = (page << 13) + pageOffset8[bp & 0x1f][y & 0x3f][x & 0x7f];
	ASSERT(word < 1024*1024*4);
	return word;
}

DWORD GSLocalMemory::pixelAddress4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 7) * ((bw+1)>>1) + (x >> 7);
	DWORD word = (page << 14) + pageOffset4[bp & 0x1f][y & 0x7f][x & 0x7f];
	ASSERT(word < 1024*1024*8);
	return word;
}

DWORD GSLocalMemory::pixelAddress32Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 5) * bw + (x >> 6); 
	DWORD word = (page << 11) + pageOffset32Z[bp & 0x1f][y & 0x1f][x & 0x3f];
	ASSERT(word < 1024*1024);
	return word;
}

DWORD GSLocalMemory::pixelAddress16Z(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16Z[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

DWORD GSLocalMemory::pixelAddress16SZ(int x, int y, DWORD bp, DWORD bw)
{
	DWORD page = (bp >> 5) + (y >> 6) * bw + (x >> 6); 
	DWORD word = (page << 12) + pageOffset16SZ[bp & 0x1f][y & 0x3f][x & 0x3f];
	ASSERT(word < 1024*1024*2);
	return word;
}

GSLocalMemory::pixelAddress GSLocalMemory::GetPixelAddress(DWORD psm)
{
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMCT24: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMCT16: pa = &GSLocalMemory::pixelAddress16; break;
	case PSM_PSMCT16S: pa = &GSLocalMemory::pixelAddress16S; break;
	case PSM_PSMT8: pa = &GSLocalMemory::pixelAddress8; break;
	case PSM_PSMT4: pa = &GSLocalMemory::pixelAddress4; break;
	case PSM_PSMT8H: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMT4HL: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMT4HH: pa = &GSLocalMemory::pixelAddress32; break;
	case PSM_PSMZ32: pa = &GSLocalMemory::pixelAddress32Z; break;
	case PSM_PSMZ24: pa = &GSLocalMemory::pixelAddress32Z; break;
	case PSM_PSMZ16: pa = &GSLocalMemory::pixelAddress16Z; break;
	case PSM_PSMZ16S: pa = &GSLocalMemory::pixelAddress16SZ; break;
	}
	
	return pa;
}

////////////////////

void GSLocalMemory::writePixel32(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16S(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress8(x, y, bp, bw);
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress32Z(x, y, bp, bw);
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16Z(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress16SZ(x, y, bp, bw);
	m_vm16[addr] = (WORD)c;
}

GSLocalMemory::writePixel GSLocalMemory::GetWritePixel(DWORD psm)
{
	writePixel wp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wp = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wp = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wp = &GSLocalMemory::writePixel16; break;
	case PSM_PSMCT16S: wp = &GSLocalMemory::writePixel16S; break;
	case PSM_PSMT8: wp = &GSLocalMemory::writePixel8; break;
	case PSM_PSMT4: wp = &GSLocalMemory::writePixel4; break;
	case PSM_PSMT8H: wp = &GSLocalMemory::writePixel8H; break;
	case PSM_PSMT4HL: wp = &GSLocalMemory::writePixel4HL; break;
	case PSM_PSMT4HH: wp = &GSLocalMemory::writePixel4HH; break;
	case PSM_PSMZ32: wp = &GSLocalMemory::writePixel32Z; break;
	case PSM_PSMZ24: wp = &GSLocalMemory::writePixel24Z; break;
	case PSM_PSMZ16: wp = &GSLocalMemory::writePixel16Z; break;
	case PSM_PSMZ16S: wp = &GSLocalMemory::writePixel16SZ; break;
	}
	
	return wp;
}

////////////////////

void GSLocalMemory::writeFrame16(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16(x, y, c, bp, bw);
}

void GSLocalMemory::writeFrame16S(int x, int y, DWORD c, DWORD bp, DWORD bw)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16S(x, y, c, bp, bw);
}

GSLocalMemory::writeFrame GSLocalMemory::GetWriteFrame(DWORD psm)
{
	writeFrame wf = NULL;

	switch(psm)
	{
	default: //ASSERT(0);
	case PSM_PSMCT32: wf = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wf = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wf = &GSLocalMemory::writeFrame16; break;
	case PSM_PSMCT16S: wf = &GSLocalMemory::writeFrame16S; break;
	}
	
	return wf;
}

////////////////////

DWORD GSLocalMemory::readPixel32(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16S(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16S(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm8[pixelAddress8(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel8H(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32(x, y, bp, bw)] >> 24;
}

DWORD GSLocalMemory::readPixel4(int x, int y, DWORD bp, DWORD bw)
{
	DWORD addr = pixelAddress4(x, y, bp, bw);
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(int x, int y, DWORD bp, DWORD bw)
{
	return (m_vm32[pixelAddress32(x, y, bp, bw)] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel24Z(int x, int y, DWORD bp, DWORD bw)
{
	return m_vm32[pixelAddress32Z(x, y, bp, bw)] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16Z(x, y, bp, bw)];
}

DWORD GSLocalMemory::readPixel16SZ(int x, int y, DWORD bp, DWORD bw)
{
	return (DWORD)m_vm16[pixelAddress16SZ(x, y, bp, bw)];
}

GSLocalMemory::readPixel GSLocalMemory::GetReadPixel(DWORD psm)
{
	readPixel rp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rp = &GSLocalMemory::readPixel32; break;
	case PSM_PSMCT24: rp = &GSLocalMemory::readPixel24; break;
	case PSM_PSMCT16: rp = &GSLocalMemory::readPixel16; break;
	case PSM_PSMCT16S: rp = &GSLocalMemory::readPixel16S; break;
	case PSM_PSMT8: rp = &GSLocalMemory::readPixel8; break;
	case PSM_PSMT4: rp = &GSLocalMemory::readPixel4; break;
	case PSM_PSMT8H: rp = &GSLocalMemory::readPixel8H; break;
	case PSM_PSMT4HL: rp = &GSLocalMemory::readPixel4HL; break;
	case PSM_PSMT4HH: rp = &GSLocalMemory::readPixel4HH; break;
	case PSM_PSMZ32: rp = &GSLocalMemory::readPixel32Z; break;
	case PSM_PSMZ24: rp = &GSLocalMemory::readPixel24Z; break;
	case PSM_PSMZ16: rp = &GSLocalMemory::readPixel16Z; break;
	case PSM_PSMZ16S: rp = &GSLocalMemory::readPixel16SZ; break;
	}
	
	return rp;
}

////////////////////

void GSLocalMemory::writePixel32(DWORD addr, DWORD c)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16S(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel8(DWORD addr, DWORD c)
{
	m_vm8[addr] = (BYTE)c;
}

void GSLocalMemory::writePixel8H(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0x00ffffff) | (c << 24);
}

void GSLocalMemory::writePixel4(DWORD addr, DWORD c)
{
	int shift = (addr&1) << 2; addr >>= 1;
	m_vm8[addr] = (BYTE)((m_vm8[addr] & (0xf0 >> shift)) | ((c & 0x0f) << shift));
}

void GSLocalMemory::writePixel4HL(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xf0ffffff) | ((c & 0x0f) << 24);
}

void GSLocalMemory::writePixel4HH(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0x0fffffff) | ((c & 0x0f) << 28);
}

void GSLocalMemory::writePixel32Z(DWORD addr, DWORD c)
{
	m_vm32[addr] = c;
}

void GSLocalMemory::writePixel24Z(DWORD addr, DWORD c)
{
	m_vm32[addr] = (m_vm32[addr] & 0xff000000) | (c & 0x00ffffff);
}

void GSLocalMemory::writePixel16Z(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

void GSLocalMemory::writePixel16SZ(DWORD addr, DWORD c)
{
	m_vm16[addr] = (WORD)c;
}

GSLocalMemory::writePixelAddr GSLocalMemory::GetWritePixelAddr(DWORD psm)
{
	writePixelAddr wp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: wp = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wp = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wp = &GSLocalMemory::writePixel16; break;
	case PSM_PSMCT16S: wp = &GSLocalMemory::writePixel16S; break;
	case PSM_PSMT8: wp = &GSLocalMemory::writePixel8; break;
	case PSM_PSMT4: wp = &GSLocalMemory::writePixel4; break;
	case PSM_PSMT8H: wp = &GSLocalMemory::writePixel8H; break;
	case PSM_PSMT4HL: wp = &GSLocalMemory::writePixel4HL; break;
	case PSM_PSMT4HH: wp = &GSLocalMemory::writePixel4HH; break;
	case PSM_PSMZ32: wp = &GSLocalMemory::writePixel32Z; break;
	case PSM_PSMZ24: wp = &GSLocalMemory::writePixel24Z; break;
	case PSM_PSMZ16: wp = &GSLocalMemory::writePixel16Z; break;
	case PSM_PSMZ16S: wp = &GSLocalMemory::writePixel16SZ; break;
	}
	
	return wp;
}

////////////////////

void GSLocalMemory::writeFrame16(DWORD addr, DWORD c)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16(addr, c);
}

void GSLocalMemory::writeFrame16S(DWORD addr, DWORD c)
{
	c = ((c>>16)&0x8000)|((c>>9)&0x7c00)|((c>>6)&0x03e0)|((c>>3)&0x001f);
	writePixel16S(addr, c);
}

GSLocalMemory::writeFrameAddr GSLocalMemory::GetWriteFrameAddr(DWORD psm)
{
	writeFrameAddr wf = NULL;

	switch(psm)
	{
	default: //ASSERT(0);
	case PSM_PSMCT32: wf = &GSLocalMemory::writePixel32; break;
	case PSM_PSMCT24: wf = &GSLocalMemory::writePixel24; break;
	case PSM_PSMCT16: wf = &GSLocalMemory::writeFrame16; break;
	case PSM_PSMCT16S: wf = &GSLocalMemory::writeFrame16S; break;
	}
	
	return wf;
}

////////////////////

DWORD GSLocalMemory::readPixel32(DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24(DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16S(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel8(DWORD addr)
{
	return (DWORD)m_vm8[addr];
}

DWORD GSLocalMemory::readPixel8H(DWORD addr)
{
	return m_vm32[addr] >> 24;
}

DWORD GSLocalMemory::readPixel4(DWORD addr)
{
	return (m_vm8[addr>>1] >> ((addr&1) << 2)) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HL(DWORD addr)
{
	return (m_vm32[addr] >> 24) & 0x0f;
}

DWORD GSLocalMemory::readPixel4HH(DWORD addr)
{
	return (m_vm32[addr] >> 28) & 0x0f;
}

DWORD GSLocalMemory::readPixel32Z(DWORD addr)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readPixel24Z(DWORD addr)
{
	return m_vm32[addr] & 0x00ffffff;
}

DWORD GSLocalMemory::readPixel16Z(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

DWORD GSLocalMemory::readPixel16SZ(DWORD addr)
{
	return (DWORD)m_vm16[addr];
}

GSLocalMemory::readPixelAddr GSLocalMemory::GetReadPixelAddr(DWORD psm)
{
	readPixelAddr rp = NULL;

	switch(psm)
	{
	default: ASSERT(0);
	case PSM_PSMCT32: rp = &GSLocalMemory::readPixel32; break;
	case PSM_PSMCT24: rp = &GSLocalMemory::readPixel24; break;
	case PSM_PSMCT16: rp = &GSLocalMemory::readPixel16; break;
	case PSM_PSMCT16S: rp = &GSLocalMemory::readPixel16S; break;
	case PSM_PSMT8: rp = &GSLocalMemory::readPixel8; break;
	case PSM_PSMT4: rp = &GSLocalMemory::readPixel4; break;
	case PSM_PSMT8H: rp = &GSLocalMemory::readPixel8H; break;
	case PSM_PSMT4HL: rp = &GSLocalMemory::readPixel4HL; break;
	case PSM_PSMT4HH: rp = &GSLocalMemory::readPixel4HH; break;
	case PSM_PSMZ32: rp = &GSLocalMemory::readPixel32Z; break;
	case PSM_PSMZ24: rp = &GSLocalMemory::readPixel24Z; break;
	case PSM_PSMZ16: rp = &GSLocalMemory::readPixel16Z; break;
	case PSM_PSMZ16S: rp = &GSLocalMemory::readPixel16SZ; break;
	}
	
	return rp;
}

////////////////////

bool GSLocalMemory::FillRect(CRect& r, DWORD c, DWORD psm, DWORD fbp, DWORD fbw)
{
	int w, h, bpp;

	writePixel wp = GetWritePixel(psm);
	pixelAddress pa = NULL;

	switch(psm)
	{
	default: ASSERT(0); 
	case PSM_PSMCT32: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMCT24: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMCT16: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMCT16S: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMZ32: w = 8; h = 8; bpp = 32; break;
	case PSM_PSMZ16: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMZ16S: w = 16; h = 8; bpp = 16; break;
	case PSM_PSMT8: w = 16; h = 16; bpp = 8; break;
	case PSM_PSMT4: w = 32; h = 16; bpp = 4; break;
	}

	pa = GetBlockAddress(psm);

	int dwords = w*h*bpp/8/4;
	int shift = 0;

	switch(bpp)
	{
	case 32: shift = 0; break;
	case 16: shift = 1; c = (c&0xffff)*0x00010001; break;
	case 8: shift = 2; c = (c&0xff)*0x01010101; break;
	case 4: shift = 3; c = (c&0xf)*0x11111111; break;
	}

	CRect clip((r.left+(w-1))&~(w-1), (r.top+(h-1))&~(h-1), r.right&~(w-1), r.bottom&~(h-1));

	for(int y = r.top; y < clip.top; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	for(int y = clip.top; y < clip.bottom; y += h)
	{
		for(int ys = y, ye = y + h; ys < ye; ys++)
		{
			for(int x = r.left; x < clip.left; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
			for(int x = clip.right; x < r.right; x++)
				(this->*wp)(x, ys, c, fbp, fbw);
		}
	}

	if(psm == PSM_PSMCT24)
	{
		c &= 0x00ffffff;
		for(int y = clip.top; y < clip.bottom; y += h)
		{
			for(int x = clip.left; x < clip.right; x += w)
			{
				DWORD* p = &m_vm32[(this->*pa)(x, y, fbp, fbw)];
				for(int size = dwords; size-- > 0; p++)
					*p = (*p&0xff000000)|c;
			}
		}
	}
	else
	{
		for(int y = clip.top; y < clip.bottom; y += h)
			for(int x = clip.left; x < clip.right; x += w)
				memsetd(&m_vm8[(this->*pa)(x, y, fbp, fbw) << 2 >> shift], c, dwords);
	}

	for(int y = clip.bottom; y < r.bottom; y++)
		for(int x = r.left; x < r.right; x++)
			(this->*wp)(x, y, c, fbp, fbw);

	return(true);
}

////////////////////

void GSLocalMemory::WriteCLUT(GIFRegTEX0 TEX0, GIFRegTEXCLUT TEXCLUT)
{
	switch(TEX0.CLD)
	{
	default:
	case 0: return;
	case 1: break;
	case 2: m_CBP[0] = TEX0.CBP; break;
	case 3: m_CBP[1] = TEX0.CBP; break;
	case 4: if(m_CBP[0] == TEX0.CBP) return; break;
	case 5: if(m_CBP[1] == TEX0.CBP) return; break;
	}

	{
		if(!m_fCLUTMayBeDirty && m_prevTEX0.i64 == TEX0.i64 && m_prevTEXCLUT.i64 == TEXCLUT.i64)
			return;

		m_prevTEX0 = TEX0;
		m_prevTEXCLUT = TEXCLUT;

		m_fCLUTMayBeDirty = false;
	}

	DWORD bp = TEX0.CBP;
	DWORD bw = TEX0.CSM == 0 ? 1 : TEXCLUT.CBW;

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	if(TEX0.CSM == 0)
	{
		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			WORD* vm = &m_vm16[TEX0.CPSM == PSM_PSMCT16 ? blockAddress16(0, 0, bp, bw) : blockAddress16S(0, 0, bp, bw)];

			if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
			{
				WriteCLUT_T16_I8_CSM1(vm, pCLUT);
			}
			else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
			{
				WriteCLUT_T16_I4_CSM1(vm, pCLUT);
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32)
		{
			DWORD* vm = &m_vm32[blockAddress32(0, 0, bp, bw)];

			if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
			{
				WriteCLUT_T32_I8_CSM1(vm, pCLUT);
			}
			else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
			{
				WriteCLUT_T32_I4_CSM1(vm, pCLUT);
			}
		}
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16); // this is the only allowed format for CSM2, but we implement all of them, just in case...

		readPixel rp = GetReadPixel(TEX0.CPSM);

		int nPaletteEntries = 0;

		if(TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)
		{
			nPaletteEntries = 256;
		}
		else if(TEX0.PSM == PSM_PSMT4HH || TEX0.PSM == PSM_PSMT4HL || TEX0.PSM == PSM_PSMT4)
		{
			nPaletteEntries = 16;
		}

		if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
		{
			for(int i = 0; i < nPaletteEntries; i++)
			{
				pCLUT[i] = (WORD)(this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
			}
		}
		else if(TEX0.CPSM == PSM_PSMCT32)
		{
			for(int i = 0; i < nPaletteEntries; i++)
			{
				DWORD dw = (this->*rp)((TEXCLUT.COU<<4) + i, TEXCLUT.COV, bp, bw);
				pCLUT[i] = (WORD)(dw & 0xffff);
				pCLUT[i+256] = (WORD)(dw >> 16);
			}
		}
	}
}

//

void GSLocalMemory::ReadCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32)
{
	ASSERT(pCLUT32);

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	if(TEX0.CPSM == PSM_PSMCT32)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T32_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T32_I4(pCLUT, pCLUT32);
			break;
		}
	}
	else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T16_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T16_I4(pCLUT, pCLUT32);
			break;
		}
	}
}

void GSLocalMemory::SetupCLUT(GIFRegTEX0 TEX0, GIFRegTEXA TEXA)
{
	ReadCLUT(TEX0, TEXA, m_pCLUT32);
}

//

void GSLocalMemory::ReadCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA, DWORD* pCLUT32)
{
	// FIXME: 16-bit palette (what if TEXA is changed after TEX0/2?)

	ASSERT(pCLUT32);

	WORD* pCLUT = m_pCLUT + (TEX0.CSA<<4);

	if(TEX0.CPSM == PSM_PSMCT32)
	{
		switch(TEX0.PSM)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
			ReadCLUT32_T32_I8(pCLUT, pCLUT32);
			break;
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadCLUT32_T32_I4(pCLUT, pCLUT32);
			break;
		}
	}
	else if(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S)
	{
		// TODO: avoid Expand16 with ps20+
		Expand16(pCLUT, pCLUT32, PaletteEntries(TEX0.PSM), &TEXA);
	}
}

void GSLocalMemory::SetupCLUT32(GIFRegTEX0 TEX0, GIFRegTEXA TEXA)
{
	ReadCLUT32(TEX0, TEXA, m_pCLUT32);

	// TMP
	switch(TEX0.PSM)
	{
	case PSM_PSMT4:
	case PSM_PSMT4HL:
	case PSM_PSMT4HH:
		for(int j = 0, k = 0; j < 16; j++)
			for(int i = 0; i < 16; i++, k++)
				m_pCLUT64[k] = ((UINT64)m_pCLUT32[j] << 32) | m_pCLUT32[i];
		break;
	}
}

void GSLocalMemory::CopyCLUT32(DWORD* pCLUT32, int nPaletteEntries)
{
	memcpy(pCLUT32, m_pCLUT32, sizeof(DWORD)*nPaletteEntries);
}

////////////////////

DWORD GSLocalMemory::readTexel32(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel24(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From24To32(m_vm32[pixelAddress32(x, y, TEX0.TBP0, TEX0.TBW)], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel16S(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[pixelAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], TEXA);
}

DWORD GSLocalMemory::readTexel8(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel8H(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HL(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW)];
}

DWORD GSLocalMemory::readTexel4HH(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW)];
}

GSLocalMemory::readTexel GSLocalMemory::GetReadTexel(DWORD psm)
{
	readTexel rt = NULL;

	switch(psm)
	{
	default: ;//ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel24; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16S; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8H; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HL; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HH; break;
	}
	
	return rt;
}

////////////////////

DWORD GSLocalMemory::readTexel32(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_vm32[addr];
}

DWORD GSLocalMemory::readTexel24(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From24To32(m_vm32[addr], TEX0.ai32[1]&4, TEXA);
}

DWORD GSLocalMemory::readTexel16(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel16S(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return From16To32(m_vm16[addr], TEXA);
}

DWORD GSLocalMemory::readTexel8(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8(addr)];
}

DWORD GSLocalMemory::readTexel8H(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel8H(addr)];
}

DWORD GSLocalMemory::readTexel4(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4(addr)];
}

DWORD GSLocalMemory::readTexel4HL(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HL(addr)];
}

DWORD GSLocalMemory::readTexel4HH(DWORD addr, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return m_pCLUT32[readPixel4HH(addr)];
}

GSLocalMemory::readTexelAddr GSLocalMemory::GetReadTexelAddr(DWORD psm)
{
	readTexelAddr rt = NULL;

	switch(psm)
	{
	default: //ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel24; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16S; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8H; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HL; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HH; break;
	}
	
	return rt;
}

////////////////////

static void SwizzleTextureStep(int& tx, int& ty, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
//	if(ty == TRXREG.RRH && tx == TRXPOS.DSAX) ASSERT(0);

	if(++tx == TRXREG.RRW)
	{
		tx = TRXPOS.DSAX;
		ty++;
	}
}

#define IsTopLeftAligned(dsax, tx, ty, bw, bh) \
	(((dsax) & ((bw)-1)) == 0 && ((tx) & ((bw)-1)) == 0 && (dsax) == (tx) && ((ty) & ((bh)-1)) == 0)

void GSLocalMemory::SwizzleTexture32(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*4;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 8 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int twa = tw & ~7;
			int tha = th & ~7;

			for(int y = ty; y < tha; )
			{
				for(int x = tx; x < twa; x += 8)
					SwizzleBlock32u((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel32(x, y, ((DWORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = tha;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 8)
				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*4, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture24(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*3;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)*3;
				DWORD* d = block;

				for(int j = 0, diff = srcpitch - 8*3; j < 8; j++, s += diff, d += 8)
					for(int i = 0; i < 8; i++, s += 3)
						d[i] = (s[2]<<16)|(s[1]<<8)|s[0];

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x00ffffff);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 8);

	if(!fTopLeftAligned || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int twa = tw & ~15;
			int tha = th & ~7;

			for(int y = ty; y < tha; )
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock16u((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16(x, y, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = tha;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture16S(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)*2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 8);

	if(!fTopLeftAligned || (tw & 15) || (th & 7) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 8)
		{
			len -= (th & ~7) * srcpitch;

			th += ty;

			int twa = tw & ~15;
			int tha = th & ~7;

			for(int y = ty; y < tha; )
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock16u((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

				for(int ye = y + 8; y < ye; y++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel16S(x, y, ((WORD*)src)[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = tha;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx)*2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 16, 16);

	if(!fTopLeftAligned || (tw & 15) || (th & 15) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 16 && th >= 16)
		{
			len -= (th & ~15) * srcpitch;

			th += ty;

			int twa = tw & ~15;
			int tha = th & ~15;

			for(int y = ty; y < tha; )
			{
				for(int x = tx; x < twa; x += 16)
					SwizzleBlock8u((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

				for(int ye = y + 16; y < ye; y++, src += srcpitch)
					for(int x = twa; x < tw; x++)
						writePixel8(x, y, src[x - tx], BITBLTBUF.DBP, BITBLTBUF.DBW);
			}

			ty = tha;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 16, src += srcpitch*16)
			for(int x = tx; x < tw; x += 16)
				SwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], src + (x - tx), srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture8H(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = TRXREG.RRW - TRXPOS.DSAX;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx);
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8; i++)
						d[i] = s[i] << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0xff000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 32, 16);

	if(!fTopLeftAligned || (tw & 31) || (th & 15) || (len % srcpitch))
	{
		if(fTopLeftAligned && tw >= 32 && th >= 16)
		{
			len -= (th & ~15) * srcpitch;

			th += ty;

			int twa = tw & ~31;
			int tha = th & ~15;

			for(int y = ty; y < tha; )
			{
				for(int x = tx; x < twa; x += 32)
					SwizzleBlock4u((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

				for(int ye = y + 16; y < ye; y++, src += srcpitch)
				{
					BYTE* s = src + (twa - tx)/2;

					for(int x = twa; x < tw; x += 2, s++)
					{
						writePixel4(x, y, *s&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
						writePixel4(x+1, y, *s>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
					}
				}
			}

			ty = tha;
		}

		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 16, src += srcpitch*16)
			for(int x = tx; x < tw; x += 32)
				SwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)>>1], src + (x - tx)/2, srcpitch);

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HL(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 24, 
						d[i*2+1] = (s[i]&0xf0) << 20;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTexture4HH(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int tw = TRXREG.RRW, srcpitch = (TRXREG.RRW - TRXPOS.DSAX)/2;
	int th = len / srcpitch;

	bool fTopLeftAligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!fTopLeftAligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		SwizzleTextureX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		__declspec(align(16)) DWORD block[8*8];

		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch*8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				BYTE* s = src + (x - tx)/2;
				DWORD* d = block;

				for(int j = 0; j < 8; j++, s += srcpitch, d += 8)
					for(int i = 0; i < 8/2; i++)
						d[i*2] = (s[i]&0x0f) << 28, 
						d[i*2+1] = (s[i]&0xf0) << 24;

				SwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, BITBLTBUF.DBP, BITBLTBUF.DBW)], (BYTE*)block, sizeof(block)/8, 0x0f000000);
			}
		}

		ty = th;
	}
}

void GSLocalMemory::SwizzleTextureX(int& tx, int& ty, BYTE* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(len <= 0) return;

	BYTE* pb = (BYTE*)src;
	WORD* pw = (WORD*)src;
	DWORD* pd = (DWORD*)src;

	// if(ty >= (int)TRXREG.RRH) {ASSERT(0); return;}

	switch(BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16S(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel8H(tx, ty, *pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HL(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HL(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb++)
			writePixel4HH(tx, ty, *pb&0xf, BITBLTBUF.DBP, BITBLTBUF.DBW),
			writePixel4HH(tx+1, ty, *pb>>4, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pd++)
			writePixel32Z(tx, ty, *pd, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pb+=3)
			writePixel24Z(tx, ty, *(DWORD*)pb, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16Z(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; SwizzleTextureStep(tx, ty, TRXPOS, TRXREG), pw++)
			writePixel16SZ(tx, ty, *pw, BITBLTBUF.DBP, BITBLTBUF.DBW);
		break;
	}
}

GSLocalMemory::SwizzleTexture GSLocalMemory::GetSwizzleTexture(DWORD psm)
{
	SwizzleTexture st = NULL;

	switch(psm)
	{
	default:
	case PSM_PSMCT32: st = &GSLocalMemory::SwizzleTexture32; break;
	case PSM_PSMCT24: st = &GSLocalMemory::SwizzleTexture24; break;
	case PSM_PSMCT16: st = &GSLocalMemory::SwizzleTexture16; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::SwizzleTexture16S; break;
	case PSM_PSMT8: st = &GSLocalMemory::SwizzleTexture8; break;
	case PSM_PSMT8H: st = &GSLocalMemory::SwizzleTexture8H; break;
	case PSM_PSMT4: st = &GSLocalMemory::SwizzleTexture4; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::SwizzleTexture4HL; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::SwizzleTexture4HH; break;
	//case PSM_PSMZ32: st = &GSLocalMemory::SwizzleTexture32Z; break;
	//case PSM_PSMZ24: st = &GSLocalMemory::SwizzleTexture24Z; break;
	//case PSM_PSMZ16: st = &GSLocalMemory::SwizzleTexture16Z; break;
	//case PSM_PSMZ16S: st = &GSLocalMemory::SwizzleTexture16SZ; break;
	}

	return st;
}

///////////////////

void GSLocalMemory::unSwizzleTexture32(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ompdst + (x-r.left)*4, dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left)*4, dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture24(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock24(block, (DWORD*)ompdst + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		__declspec(align(16)) DWORD block[8*8];
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
		ExpandBlock24(block, (DWORD*)dst + (x-r.left), dstpitch, &TEXA);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture16(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 8)
		{
			__declspec(align(16)) WORD block[16*8];
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)ompdst + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 8)
	{
		__declspec(align(16)) WORD block[16*8];
		unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
		ExpandBlock16(block, (DWORD*)dst + (x-r.left), dstpitch, &TEXA);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture16S(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 8)
		{
			__declspec(align(16)) WORD block[16*8];
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
			ExpandBlock16(block, (DWORD*)ompdst + (x-r.left), dstpitch, &TEXA);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 8)
	{
		__declspec(align(16)) WORD block[16*8];
		unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);
		ExpandBlock16(block, (DWORD*)dst + (x-r.left), dstpitch, &TEXA);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture8(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 16)
		{
			__declspec(align(16)) BYTE block[16*16];
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 16, d += dstpitch)
				for(int i = 0; i < 16; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i]];
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 16)
	{
		__declspec(align(16)) BYTE block[16*16];
		unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/16);

        BYTE* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 16; j++, s += 16, d += dstpitch)
			for(int i = 0; i < 16; i++)
				((DWORD*)d)[i] = m_pCLUT32[s[i]];
	}
#endif
}

void GSLocalMemory::unSwizzleTexture8H(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i] >> 24];
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		__declspec(align(16)) DWORD block[8*8];
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

        DWORD* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)d)[i] = m_pCLUT32[s[i] >> 24];
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 32, 16)
		{
/*
			__declspec(align(16)) BYTE block[(32/2)*16];
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
				for(int i = 0; i < 32/2; i++)
					((DWORD*)d)[i*2] = m_pCLUT32[s[i]&0x0f],
					((DWORD*)d)[i*2+1] = m_pCLUT32[s[i]>>4];
*/
/*
			__declspec(align(16)) BYTE block[32*16];
			unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 32, d += dstpitch)
				for(int i = 0; i < 32; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i]];
*/
			__declspec(align(16)) BYTE block[(32/2)*16];
			unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

			BYTE* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
				for(int i = 0; i < 32/2; i++)
					((UINT64*)d)[i] = m_pCLUT64[s[i]];

		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 32, 16)
	{
/*
		__declspec(align(16)) BYTE block[(32/2)*16];
		unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

        BYTE* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
			for(int i = 0; i < 32/2; i++)
				((DWORD*)d)[i*2] = m_pCLUT32[s[i]&0x0f],
				((DWORD*)d)[i*2+1] = m_pCLUT32[s[i]>>4];
*/
/*
		__declspec(align(16)) BYTE block[32*16];
		unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

        BYTE* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 16; j++, s += 32, d += dstpitch)
			for(int i = 0; i < 32; i++)
				((DWORD*)d)[i] = m_pCLUT32[s[i]];
*/
		__declspec(align(16)) BYTE block[(32/2)*16];
		unSwizzleBlock4((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], (BYTE*)block, sizeof(block)/16);

        BYTE* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 16; j++, s += 32/2, d += dstpitch)
			for(int i = 0; i < 32/2; i++)
				((UINT64*)d)[i] = m_pCLUT64[s[i]];
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4HL(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[(s[i] >> 24)&0x0f];
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		__declspec(align(16)) DWORD block[8*8];
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

        DWORD* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)d)[i] = m_pCLUT32[(s[i] >> 24)&0x0f];
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4HH(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			__declspec(align(16)) DWORD block[8*8];
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

			DWORD* s = block;
			BYTE* d = ompdst + (x-r.left)*4;

			for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
				for(int i = 0; i < 8; i++)
					((DWORD*)d)[i] = m_pCLUT32[s[i] >> 28];
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		__declspec(align(16)) DWORD block[8*8];
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], (BYTE*)block, sizeof(block)/8);

        DWORD* s = block;
		BYTE* d = dst + (x-r.left)*4;

		for(int j = 0; j < 8; j++, s += 8, d += dstpitch)
			for(int i = 0; i < 8; i++)
				((DWORD*)d)[i] = m_pCLUT32[s[i] >> 28];
	}
#endif
}

GSLocalMemory::unSwizzleTexture GSLocalMemory::GetUnSwizzleTexture(DWORD psm)
{
	unSwizzleTexture st = NULL;

	switch(psm)
	{
	default:
	case PSM_PSMCT32: st = &GSLocalMemory::unSwizzleTexture32; break;
	case PSM_PSMCT24: st = &GSLocalMemory::unSwizzleTexture24; break;
	case PSM_PSMCT16: st = &GSLocalMemory::unSwizzleTexture16; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::unSwizzleTexture16S; break;
	case PSM_PSMT8: st = &GSLocalMemory::unSwizzleTexture8; break;
	case PSM_PSMT8H: st = &GSLocalMemory::unSwizzleTexture8H; break;
	case PSM_PSMT4: st = &GSLocalMemory::unSwizzleTexture4; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::unSwizzleTexture4HL; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::unSwizzleTexture4HH; break;
	}

	return st;
}

///////////////////

void GSLocalMemory::ReadTexture(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	CSize bs = GetBlockSize(TEX0.PSM);

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, GetReadTexel(TEX0.PSM));
	}
	else
	{
		unSwizzleTexture st = GetUnSwizzleTexture(TEX0.PSM);
		(this->*st)(r, dst, dstpitch, TEX0, TEXA);
	}
}

////////////////////

DWORD GSLocalMemory::readTexel32P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel32(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel16P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel16(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel16SP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel16S(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel8(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel8HP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel8H(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4P(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HLP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4HL(x, y, TEX0.TBP0, TEX0.TBW);
}

DWORD GSLocalMemory::readTexel4HHP(int x, int y, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
	return readPixel4HH(x, y, TEX0.TBP0, TEX0.TBW);
}

GSLocalMemory::readTexelP GSLocalMemory::GetReadTexelP(DWORD psm)
{
	readTexelP rt = NULL;

	switch(psm)
	{
	default: ;//ASSERT(0);
	case PSM_PSMCT32: rt = &GSLocalMemory::readTexel32P; break;
	case PSM_PSMCT24: rt = &GSLocalMemory::readTexel32P; break;
	case PSM_PSMCT16: rt = &GSLocalMemory::readTexel16P; break;
	case PSM_PSMCT16S: rt = &GSLocalMemory::readTexel16SP; break;
	case PSM_PSMT8: rt = &GSLocalMemory::readTexel8P; break;
	case PSM_PSMT4: rt = &GSLocalMemory::readTexel4P; break;
	case PSM_PSMT8H: rt = &GSLocalMemory::readTexel8HP; break;
	case PSM_PSMT4HL: rt = &GSLocalMemory::readTexel4HLP; break;
	case PSM_PSMT4HH: rt = &GSLocalMemory::readTexel4HHP; break;
	}
	
	return rt;
}

///////////////////

void GSLocalMemory::unSwizzleTexture32P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], ompdst + (x-r.left)*4, dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		unSwizzleBlock32((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left)*4, dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture16P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 8)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], ompdst + (x-r.left)*2, dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 8)
	{
		unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left)*2, dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture16SP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 8)
		{
			unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left)*2, dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 8)
	{
		unSwizzleBlock16((BYTE*)&m_vm16[blockAddress16S(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left)*2, dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture8P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 16, 16)
		{
			unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 16, 16)
	{
		unSwizzleBlock8((BYTE*)&m_vm8[blockAddress8(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture8HP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			unSwizzleBlock8HP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		unSwizzleBlock8HP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4P(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 32, 16)
		{
			unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], dst + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 32, 16)
	{
		unSwizzleBlock4P((BYTE*)&m_vm8[blockAddress4(x, y, TEX0.TBP0, TEX0.TBW)>>1], dst + (x-r.left), dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4HLP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			unSwizzleBlock4HLP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		unSwizzleBlock4HLP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
	}
#endif
}

void GSLocalMemory::unSwizzleTexture4HHP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA)
{
#ifdef _OPENMP
	#pragma omp parallel
	{
		#pragma omp for
		FOREACH_BLOCK_OMP_START(r, 8, 8)
		{
			unSwizzleBlock4HHP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
		}
		FOREACH_BLOCK_OMP_END
	}
#else
	FOREACH_BLOCK(r, 8, 8)
	{
		unSwizzleBlock4HHP((BYTE*)&m_vm32[blockAddress32(x, y, TEX0.TBP0, TEX0.TBW)], dst + (x-r.left), dstpitch);
	}
#endif
}

GSLocalMemory::unSwizzleTextureP GSLocalMemory::GetUnSwizzleTextureP(DWORD psm)
{
	unSwizzleTextureP st = NULL;

	switch(psm)
	{
	default:
	case PSM_PSMCT32: st = &GSLocalMemory::unSwizzleTexture32P; break;
	case PSM_PSMCT24: st = &GSLocalMemory::unSwizzleTexture32P; break;
	case PSM_PSMCT16: st = &GSLocalMemory::unSwizzleTexture16P; break;
	case PSM_PSMCT16S: st = &GSLocalMemory::unSwizzleTexture16SP; break;
	case PSM_PSMT8: st = &GSLocalMemory::unSwizzleTexture8P; break;
	case PSM_PSMT8H: st = &GSLocalMemory::unSwizzleTexture8HP; break;
	case PSM_PSMT4: st = &GSLocalMemory::unSwizzleTexture4P; break;
	case PSM_PSMT4HL: st = &GSLocalMemory::unSwizzleTexture4HLP; break;
	case PSM_PSMT4HH: st = &GSLocalMemory::unSwizzleTexture4HHP; break;
	}

	return st;
}

///////////////////

void GSLocalMemory::ReadTextureP(const CRect& r, BYTE* dst, int dstpitch, GIFRegTEX0& TEX0, GIFRegTEXA& TEXA, GIFRegCLAMP& CLAMP)
{
	CSize bs = GetBlockSize(TEX0.PSM);

	if(r.Width() < bs.cx || r.Height() < bs.cy 
	|| (r.left & (bs.cx-1)) || (r.top & (bs.cy-1)) 
	|| (r.right & (bs.cx-1)) || (r.bottom & (bs.cy-1)) 
	|| (CLAMP.WMS&2) || (CLAMP.WMT&2))
	{
		switch(TEX0.PSM)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTexture<DWORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, GetReadTexelP(TEX0.PSM));
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTexture<WORD>(r, dst, dstpitch, TEX0, TEXA, CLAMP, GetReadTexelP(TEX0.PSM));
			break;
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			ReadTexture<BYTE>(r, dst, dstpitch, TEX0, TEXA, CLAMP, GetReadTexelP(TEX0.PSM));
			break;
		}
	}
	else
	{
		unSwizzleTextureP st = GetUnSwizzleTextureP(TEX0.PSM);
		(this->*st)(r, dst, dstpitch, TEX0, TEXA);
	}
}

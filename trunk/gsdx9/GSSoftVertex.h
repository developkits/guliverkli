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

__declspec(align(16)) union GSSoftVertexFP
{
	struct
	{
		float r, g, b, a;
		float x, y, z, q;
		float u, v, fog, reserved;
	};

	struct {float f[12];};
	struct {__m128 xmm[3];};

	void operator += (GSSoftVertexFP& v)
	{
#ifdef USE_SIMD
		xmm[0] = _mm_add_ps(xmm[0], v.xmm[0]);
		xmm[1] = _mm_add_ps(xmm[1], v.xmm[1]);
		xmm[2] = _mm_add_ps(xmm[2], v.xmm[2]);
#else
		for(int i = 0; i < countof(f); i++) f[i] += v.f[i];
#endif
	}

	friend GSSoftVertexFP operator + (GSSoftVertexFP& v1, GSSoftVertexFP& v2);
	friend GSSoftVertexFP operator - (GSSoftVertexFP& v1, GSSoftVertexFP& v2);
	friend GSSoftVertexFP operator * (GSSoftVertexFP& v1, float f);
	friend GSSoftVertexFP operator / (GSSoftVertexFP& v1, float f);

	int GetX() {return (int)x;}
	int GetY() {return (int)y;}
	DWORD GetZ() {return (DWORD)(z * UINT_MAX);}
	BYTE GetFog() {return (BYTE)z;}
	void GetColor(void* pRGBA)
	{
#ifdef USE_SIMD
		*(__m128i*)pRGBA = _mm_cvttps_epi32(xmm[0]);
#else
		int* p = (int*)pRGBA;
		for(int i = 0; i < 4; i++) *p++ = (int)f[i];
#endif
	}
};

inline GSSoftVertexFP operator + (GSSoftVertexFP& v1, GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
#ifdef USE_SIMD
	v0.xmm[0] = _mm_add_ps(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_add_ps(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_add_ps(v1.xmm[2], v2.xmm[2]);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] + v2.f[i];
#endif
	return v0;
}

inline GSSoftVertexFP operator - (GSSoftVertexFP& v1, GSSoftVertexFP& v2)
{
	GSSoftVertexFP v0;
#ifdef USE_SIMD
	v0.xmm[0] = _mm_sub_ps(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_sub_ps(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_sub_ps(v1.xmm[2], v2.xmm[2]);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] - v2.f[i];
#endif
	return v0;
}

inline GSSoftVertexFP operator * (GSSoftVertexFP& v1, float f)
{
	GSSoftVertexFP v0;
#ifdef USE_SIMD
	__m128 f128 = _mm_set_ps1(f);
	v0.xmm[0] = _mm_mul_ps(v1.xmm[0], f128);
	v0.xmm[1] = _mm_mul_ps(v1.xmm[1], f128);
	v0.xmm[2] = _mm_mul_ps(v1.xmm[2], f128);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] * f;
#endif
	return v0;
}

inline GSSoftVertexFP operator / (GSSoftVertexFP& v1, float f)
{
	GSSoftVertexFP v0;
#ifdef USE_SIMD
	__m128 f128 = _mm_set_ps1(f);
	v0.xmm[0] = _mm_div_ps(v1.xmm[0], f128);
	v0.xmm[1] = _mm_div_ps(v1.xmm[1], f128);
	v0.xmm[2] = _mm_div_ps(v1.xmm[2], f128);
#else
	for(int i = 0; i < countof(v0.f); i++) v0.f[i] = v1.f[i] / f;
#endif
	return v0;
}

__declspec(align(16)) union GSSoftVertexFX
{
	typedef signed int s32;
	typedef unsigned int u32;
	typedef signed __int64 s64;
	typedef unsigned __int64 u64;

	struct
	{
		s32 r, g, b, a;
		s32 x, y, fog, reserved;
		s64 z, q;
		s64 u, v;
	};

	struct {s32 dw[16];};
	struct {s64 qw[8];};
	struct {__m128i xmm[4];};

	void operator += (GSSoftVertexFX& v)
	{
#ifdef USE_SIMD
		xmm[0] = _mm_add_epi32(xmm[0], v.xmm[0]);
		xmm[1] = _mm_add_epi32(xmm[1], v.xmm[1]);
		xmm[2] = _mm_add_epi64(xmm[2], v.xmm[2]);
		xmm[3] = _mm_add_epi64(xmm[3], v.xmm[3]);
#else
		for(int i = 0; i < 8; i++) dw[i] += v.dw[i];
		for(int i = 4; i < 8; i++) qw[i] += v.qw[i];
#endif
	}

	friend GSSoftVertexFX operator + (GSSoftVertexFX& v1, GSSoftVertexFX& v2);
	friend GSSoftVertexFX operator - (GSSoftVertexFX& v1, GSSoftVertexFX& v2);
	friend GSSoftVertexFX operator * (GSSoftVertexFX& v1, s32 f);
	friend GSSoftVertexFX operator / (GSSoftVertexFX& v1, s32 f);

	int GetX() {return (int)((x+0x8000) >> 16);}
	int GetY() {return (int)((y+0x8000) >> 16);}
	DWORD GetZ() {return (DWORD)(z >> 32);}
	BYTE GetFog() {return (BYTE)(fog >> 16);}
	void GetColor(void* pRGBA)
	{
#ifdef USE_SIMD
		*(__m128i*)pRGBA = _mm_srai_epi32(xmm[0], 16);
#else
		int* p = (int*)pRGBA;
		for(int i = 0; i < 4; i++) *p++ = dw[i] >> 16;
#endif
	}
};

inline GSSoftVertexFX operator + (GSSoftVertexFX& v1, GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
#ifdef USE_SIMD
	v0.xmm[0] = _mm_add_epi32(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_add_epi32(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_add_epi64(v1.xmm[2], v2.xmm[2]);
	v0.xmm[3] = _mm_add_epi64(v1.xmm[3], v2.xmm[3]);
#else
	for(int i = 0; i < 8; i++) v0.dw[i] = v1.dw[i] + v2.dw[i];
	for(int i = 4; i < 8; i++) v0.qw[i] = v1.qw[i] + v2.qw[i];
#endif
	return v0;
}

inline GSSoftVertexFX operator - (GSSoftVertexFX& v1, GSSoftVertexFX& v2)
{
	GSSoftVertexFX v0;
#ifdef USE_SIMD
	v0.xmm[0] = _mm_sub_epi32(v1.xmm[0], v2.xmm[0]);
	v0.xmm[1] = _mm_sub_epi32(v1.xmm[1], v2.xmm[1]);
	v0.xmm[2] = _mm_sub_epi64(v1.xmm[2], v2.xmm[2]);
	v0.xmm[3] = _mm_sub_epi64(v1.xmm[3], v2.xmm[3]);
#else
	for(int i = 0; i < 8; i++) v0.dw[i] = v1.dw[i] - v2.dw[i];
	for(int i = 4; i < 8; i++) v0.qw[i] = v1.qw[i] - v2.qw[i];
#endif
	return v0;
}

inline GSSoftVertexFX operator * (GSSoftVertexFX& v1, GSSoftVertexFX::s32 f)
{
	GSSoftVertexFX v0;
	float f2 = (float)f / 65536.0f;
	for(int i = 0; i < 8; i++) v0.dw[i] = (GSSoftVertexFX::s32)((float)v1.dw[i] * f2);
	for(int i = 4; i < 8; i++) v0.qw[i] = (GSSoftVertexFX::s64)((float)v1.qw[i] * f2);
	return v0;
}

inline GSSoftVertexFX operator / (GSSoftVertexFX& v1, GSSoftVertexFX::s32 f)
{
	GSSoftVertexFX v0;
	float f2 = 65536.0f / f;
	for(int i = 0; i < 8; i++) v0.dw[i] = (GSSoftVertexFX::s32)((float)v1.dw[i] * f2);
	for(int i = 4; i < 8; i++) v0.qw[i] = (GSSoftVertexFX::s64)((float)v1.qw[i] * f2);
	return v0;
}

////////

#ifdef USE_SIMD

inline void SaturateColor(__m128i& c)
{
	__asm
	{
		pxor		xmm0, xmm0
		mov			esi, c
		movaps		xmm1, [esi]
		packssdw	xmm1, xmm0
		packuswb	xmm1, xmm0
		punpcklbw	xmm1, xmm0
		punpcklwd	xmm1, xmm0
		movaps		[esi], xmm1
	}
}

inline void MaskColor(__m128i& c)
{
	c = _mm_and_si128(c, _mm_set1_epi32(0xff));
}

inline void PackColor(__m128i& c)
{
	c = _mm_packus_epi16(_mm_packs_epi32(c, c), c);
}

inline void UnpackColor(__m128i& c)
{
	__m128i zero = _mm_set_epi32(0, 0, 0, 0);
	c = _mm_unpacklo_epi16(_mm_unpacklo_epi8(c, zero), zero);
}

#else

inline void SaturateColor(int* c)
{
	__asm
	{
		mov esi, c

		xor eax, eax
		mov edx, 0xff

		mov ecx, [esi]
		cmp ecx, eax
		cmovl ecx, eax
		cmp ecx, edx
		cmovg ecx, edx
		mov [esi], ecx

		mov ecx, [esi+4]
		cmp ecx, eax
		cmovl ecx, eax
		cmp ecx, edx
		cmovg ecx, edx
		mov [esi+4], ecx

		mov ecx, [esi+8]
		cmp ecx, eax
		cmovl ecx, eax
		cmp ecx, edx
		cmovg ecx, edx
		mov [esi+8], ecx

		mov ecx, [esi+12]
		cmp ecx, eax
		cmovl ecx, eax
		cmp ecx, edx
		cmovg ecx, edx
		mov [esi+12], ecx
	}
}

#endif
/* 
 *	Copyright (C) 2003 Gabest
 *	http://www.gabest.org
 *
 *  Mpeg2DecFilter.ax is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  Mpeg2DecFilter.ax is distributed in the hope that it will be useful,
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

#include <atlcoll.h>
#include <afxtempl.h>
#include "IMpeg2DecFilter.h"
#include "..\..\..\decss\DeCSSInputPin.h"

class CSubpicInputPin;
class CClosedCaptionOutputPin;
class CMpeg2Dec;

[uuid("39F498AF-1A09-4275-B193-673B0BA3D478")]
class CMpeg2DecFilter : public CTransformFilter, public IMpeg2DecFilter
{
	CSubpicInputPin* m_pSubpicInput;
	CClosedCaptionOutputPin* m_pClosedCaptionOutput;
	CAutoPtr<CMpeg2Dec> m_dec;
	REFERENCE_TIME m_AvgTimePerFrame;
	CCritSec m_csReceive;
	bool m_fWaitForKeyFrame;
	bool m_fFilm;
	struct framebuf 
	{
		int w, h, pitch;
		BYTE* buf[6];
		REFERENCE_TIME rtStart, rtStop;
		DWORD flags;
        framebuf()
		{
			w = h = pitch = 0;
			memset(&buf, 0, sizeof(buf));
			rtStart = rtStop = 0;
			flags = 0;
		}
        ~framebuf() {free();}
		void alloc(int w, int h, int pitch)
		{
			this->w = w; this->h = h; this->pitch = pitch;
			buf[0] = (BYTE*)_aligned_malloc(pitch*h, 16); buf[3] = (BYTE*)_aligned_malloc(pitch*h, 16);
			buf[1] = (BYTE*)_aligned_malloc(pitch*h/4, 16); buf[4] = (BYTE*)_aligned_malloc(pitch*h/4, 16);
			buf[2] = (BYTE*)_aligned_malloc(pitch*h/4, 16); buf[5] = (BYTE*)_aligned_malloc(pitch*h/4, 16);
		}
		void free() {for(int i = 0; i < 6; i++) {_aligned_free(buf[i]); buf[i] = NULL;}}
	} m_fb;

	void Copy(BYTE* pOut, BYTE** ppIn, DWORD w, DWORD h, DWORD pitchIn);
	void ResetMpeg2Decoder();
	HRESULT ReconnectOutput(int w, int h);

	DWORD m_win, m_hin, m_arxin, m_aryin;

	AM_SimpleRateChange m_rate;

public:
	CMpeg2DecFilter(LPUNKNOWN lpunk, HRESULT* phr);
	virtual ~CMpeg2DecFilter();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT Deliver(bool fRepeatLast);
	HRESULT CheckOutputMediaType(const CMediaType& mtOut);

	int GetPinCount();
	CBasePin* GetPin(int n);

    HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    HRESULT Receive(IMediaSample* pIn);

	HRESULT CheckConnect(PIN_DIRECTION dir, IPin* pPin);
    HRESULT CheckInputType(const CMediaType* mtIn);
    HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);

	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt);

	HRESULT StartStreaming();
	HRESULT StopStreaming();

	HRESULT AlterQuality(Quality q);

protected:
	CCritSec m_csProps;
	ditype m_di;
	double m_bright, m_cont, m_hue, m_sat;
	BYTE m_YTbl[256], m_UTbl[256*256], m_VTbl[256*256];
	bool m_fForcedSubs;
	bool m_fPlanarYUV;

	static void CalcBrCont(BYTE* YTbl, double bright, double cont);
	static void CalcHueSat(BYTE* UTbl, BYTE* VTbl, double hue, double sat);
	void ApplyBrContHueSat(BYTE* srcy, BYTE* srcu, BYTE* srcv, int w, int h, int pitch);
	
public:
	// IMpeg2DecFilter

	STDMETHODIMP SetDeinterlaceMethod(ditype di);
	STDMETHODIMP_(ditype) GetDeinterlaceMethod();

	STDMETHODIMP SetBrightness(double bright);
	STDMETHODIMP SetContrast(double cont);
	STDMETHODIMP SetHue(double hue);
	STDMETHODIMP SetSaturation(double sat);
	STDMETHODIMP_(double) GetBrightness();
	STDMETHODIMP_(double) GetContrast();
	STDMETHODIMP_(double) GetHue();
	STDMETHODIMP_(double) GetSaturation();

	STDMETHODIMP EnableForcedSubtitles(bool fEnable);
	STDMETHODIMP_(bool) IsForcedSubtitlesEnabled();

	STDMETHODIMP EnablePlanarYUV(bool fEnable);
	STDMETHODIMP_(bool) IsPlanarYUVEnabled();
};

class CMpeg2DecInputPin : public CDeCSSInputPin
{
	LONG m_CorrectTS;

public:
    CMpeg2DecInputPin(CTransformFilter* pFilter, HRESULT* phr, LPWSTR pName);

	CCritSec m_csRateLock;
	AM_SimpleRateChange m_ratechange;

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* pBytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CSubpicInputPin : public CMpeg2DecInputPin
{
	CCritSec m_csReceive;

	AM_PROPERTY_COMPOSIT_ON m_spon;
	AM_DVD_YUV m_sppal[16];
	CAutoPtr<AM_PROPERTY_SPHLI> m_sphli; // temp
	struct sp_t
	{
		REFERENCE_TIME rtStart, rtStop; 
		CArray<BYTE> pData;
		CAutoPtr<AM_PROPERTY_SPHLI> sphli; // hli
		bool fForced;
	};
	CAutoPtrList<sp_t> m_sps;

	bool DecodeSubpic(sp_t* sp, AM_PROPERTY_SPHLI& sphli, DWORD& offset1, DWORD& offset2);
	void RenderSubpic(sp_t* sp, BYTE** p, int w, int h, AM_PROPERTY_SPHLI* sphli_hli = NULL);

protected:
	HRESULT Transform(IMediaSample* pSample);

public:
	CSubpicInputPin(CTransformFilter* pFilter, HRESULT* phr);

	bool HasAnythingToRender(REFERENCE_TIME rt);
	void RenderSubpics(REFERENCE_TIME rt, BYTE** p, int w, int h);

    HRESULT CheckMediaType(const CMediaType* mtIn);
	HRESULT SetMediaType(const CMediaType* mtIn);

	// we shouldn't pass these to the filter from this pin
	STDMETHODIMP EndOfStream() {return S_OK;}
    STDMETHODIMP BeginFlush() {return S_OK;}
    STDMETHODIMP EndFlush();
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {return S_OK;}

	// IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Id, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Id, ULONG* pTypeSupport);
};

class CMpeg2DecOutputPin : public CTransformOutputPin
{
public:
	CMpeg2DecOutputPin(CTransformFilter* pTransformFilter, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* mtOut);
};

class CClosedCaptionOutputPin : public CBaseOutputPin
{
public:
	CClosedCaptionOutputPin(CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);

    HRESULT CheckMediaType(const CMediaType* mtOut);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);

	CMediaType& CurrentMediaType() {return m_mt;}
};


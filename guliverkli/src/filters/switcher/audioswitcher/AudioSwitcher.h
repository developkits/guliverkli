// Copyright 2003 Gabest.
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

#include <afxtempl.h>
#include <atlbase.h>
#include <atlcoll.h>

class CStreamSwitcherFilter;

class CStreamSwitcherPassThru : public IMediaSeeking, public CMediaPosition
{
protected:
	CStreamSwitcherFilter* m_pFilter;

public:
    CStreamSwitcherPassThru(LPUNKNOWN, HRESULT* phr, CStreamSwitcherFilter* pFilter);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // IMediaSeeking methods
    STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
    STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
    STDMETHODIMP SetTimeFormat(const GUID* pFormat);
    STDMETHODIMP GetTimeFormat(GUID* pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
    STDMETHODIMP IsFormatSupported(const GUID* pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
    STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
    STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD CurrentFlags, LONGLONG* pStop, DWORD StopFlags);
    STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
    STDMETHODIMP GetStopPosition(LONGLONG* pStop);
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double* pdRate);
    STDMETHODIMP GetDuration(LONGLONG* pDuration);
    STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
    STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);

    // IMediaPosition properties
    STDMETHODIMP get_Duration(REFTIME* plength);
    STDMETHODIMP put_CurrentPosition(REFTIME llTime);
    STDMETHODIMP get_StopTime(REFTIME* pllTime);
    STDMETHODIMP put_StopTime(REFTIME llTime);
    STDMETHODIMP get_PrerollTime(REFTIME* pllTime);
    STDMETHODIMP put_PrerollTime(REFTIME llTime);
    STDMETHODIMP get_Rate(double* pdRate);
    STDMETHODIMP put_Rate(double dRate);
    STDMETHODIMP get_CurrentPosition(REFTIME* pllTime);
    STDMETHODIMP CanSeekForward(LONG* pCanSeekForward);
    STDMETHODIMP CanSeekBackward(LONG* pCanSeekBackward);
};

class CStreamSwitcherInputPin;

class CStreamSwitcherAllocator : public CMemAllocator
{
protected:
    CStreamSwitcherInputPin* m_pPin;

    CMediaType m_mt;
	bool m_fMediaTypeChanged;

public:
	CStreamSwitcherAllocator(CStreamSwitcherInputPin* pPin, HRESULT* phr);
#ifdef DEBUG
	~CStreamSwitcherAllocator();
#endif

	STDMETHODIMP_(ULONG) NonDelegatingAddRef();
	STDMETHODIMP_(ULONG) NonDelegatingRelease();

	STDMETHODIMP GetBuffer(
		IMediaSample** ppBuffer, 
		REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, 
		DWORD dwFlags);

	void NotifyMediaType(const CMediaType& mt);
};

class CStreamSwitcherInputPin : public CBaseInputPin, public IPinConnection
{
	friend class CStreamSwitcherAllocator;

	CStreamSwitcherAllocator m_Allocator;

	BOOL m_bSampleSkipped;
	BOOL m_bQualityChanged;
	BOOL m_bUsingOwnAllocator;

	CAMEvent m_evBlock;
	bool m_fCanBlock;
	HRESULT Active();
	HRESULT Inactive();

	HRESULT QueryAcceptDownstream(const AM_MEDIA_TYPE* pmt);

	bool IsSelected();

	HRESULT InitializeOutputSample(IMediaSample* pInSample, IMediaSample** ppOutSample);

	HANDLE m_hNotifyEvent;

public:
    CStreamSwitcherInputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr, LPCWSTR pName);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    CMediaType& CurrentMediaType() {return m_mt;}
	IMemAllocator* CurrentAllocator() {return m_pAllocator;}

	bool IsUsingOwnAllocator() {return m_bUsingOwnAllocator == TRUE;}

	void Block(bool fBlock);

	CCritSec m_csReceive;

	// pure virtual
	HRESULT CheckMediaType(const CMediaType* pmt);

	// virtual
	HRESULT CheckConnect(IPin* pPin);
	HRESULT CompleteConnect(IPin* pReceivePin);

	// IPin
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetAllocator(IMemAllocator** ppAllocator);
	STDMETHODIMP NotifyAllocator(IMemAllocator* pAllocator, BOOL bReadOnly);
	STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();
	STDMETHODIMP EndOfStream();

	// IMemInputPin
    STDMETHODIMP Receive(IMediaSample* pSample);
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	//
	
	STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);
	STDMETHODIMP IsEndPin();
	STDMETHODIMP DynamicDisconnect();
};

class CStreamSwitcherOutputPin : public CBaseOutputPin, public IStreamBuilder
{
	CComPtr<IUnknown> m_pStreamSwitcherPassThru;
	CComPtr<IPinConnection> m_pPinConnection;

	HRESULT QueryAcceptUpstream(const AM_MEDIA_TYPE* pmt);

public:
    CStreamSwitcherOutputPin(CStreamSwitcherFilter* pFilter, HRESULT* phr);

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    CMediaType& CurrentMediaType() {return m_mt;}
	IMemAllocator* CurrentAllocator() {return m_pAllocator;}
	IPinConnection* CurrentPinConnection() {return m_pPinConnection;}

	// pure virtual
	HRESULT	DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties);

	// virtual
	HRESULT CheckConnect(IPin* pPin);
	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin* pReceivePin);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT GetMediaType(int iPosition, CMediaType* pmt);

	// IPin
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* pmt);

	// IQualityControl
	STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

	// IStreamBuilder
	STDMETHODIMP Render(IPin* ppinOut, IGraphBuilder* pGraph);
	STDMETHODIMP Backout(IPin* ppinOut, IGraphBuilder* pGraph);
};

class CStreamSwitcherFilter : public CBaseFilter, public IAMStreamSelect
{
	friend class CStreamSwitcherInputPin;
	friend class CStreamSwitcherOutputPin;
	friend class CStreamSwitcherPassThru;

	CList<CStreamSwitcherInputPin*> m_pInputs;
	CStreamSwitcherInputPin* m_pInput;
	CStreamSwitcherOutputPin* m_pOutput;

	CCritSec m_csState, m_csPins;

	HRESULT CompleteConnect(PIN_DIRECTION dir, CBasePin* pPin, IPin* pReceivePin);

protected:
	void SelectInput(CStreamSwitcherInputPin* pInput);

public:
	CStreamSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr, const CLSID& clsid);
	virtual ~CStreamSwitcherFilter();

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	int GetPinCount();
	CBasePin* GetPin(int n);

	int GetConnectedInputPinCount();
	CStreamSwitcherInputPin* GetConnectedInputPin(int n);
	CStreamSwitcherInputPin* GetInputPin();
	CStreamSwitcherOutputPin* GetOutputPin();

	bool m_fResetOutputMediaType;
	void ResetOutputMediaType() {m_fResetOutputMediaType = true;}

	// override these
	virtual HRESULT CheckMediaType(const CMediaType* pmt) = 0;
	virtual HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	virtual CMediaType CreateNewOutputMediaType(CMediaType mt, REFERENCE_TIME rtLen);
	virtual void OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut) {}

	// IAMStreamSelect
    STDMETHODIMP Count(DWORD* pcStreams);
    STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);
    STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
};

[uuid("CEDB2890-53AE-4231-91A3-B0AAFCD1DBDE")]
interface IAudioSwitcherFilter : public IUnknown
{
	STDMETHOD(GetInputSpeakerConfig) (DWORD* pdwChannelMask) = 0;
    STDMETHOD(GetSpeakerConfig) (bool* pfCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18]) = 0;
    STDMETHOD(SetSpeakerConfig) (bool fCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18]) = 0;
    STDMETHOD_(int, GetNumberOfInputChannels) () = 0;
	STDMETHOD_(bool, IsDownSamplingTo441Enabled) () = 0;
	STDMETHOD(EnableDownSamplingTo441) (bool fEnable) = 0;
	STDMETHOD_(REFERENCE_TIME, GetAudioTimeShift) () = 0;
	STDMETHOD(SetAudioTimeShift) (REFERENCE_TIME rtAudioTimeShift) = 0;
};

class AudioStreamResampler;

[uuid("18C16B08-6497-420e-AD14-22D21C2CEAB7")]
class CAudioSwitcherFilter : public CStreamSwitcherFilter, public IAudioSwitcherFilter
{
	typedef struct {DWORD Speaker, Channel;} ChMap;
	CArray<ChMap> m_chs[18];

	bool m_fCustomChannelMapping;
	DWORD m_pSpeakerToChannelMap[18][18];
	bool m_fDownSampleTo441;
	REFERENCE_TIME m_rtAudioTimeShift;
	CAutoPtrArray<AudioStreamResampler> m_pResamplers;

	REFERENCE_TIME m_rtNextStart, m_rtNextStop;

public:
	CAudioSwitcherFilter(LPUNKNOWN lpunk, HRESULT* phr);

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	HRESULT CheckMediaType(const CMediaType* pmt);
	HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut);
	CMediaType CreateNewOutputMediaType(CMediaType mt, REFERENCE_TIME rtLen);
	void OnNewOutputMediaType(const CMediaType& mtIn, const CMediaType& mtOut);

	// IAudioSwitcherFilter
	STDMETHODIMP GetInputSpeakerConfig(DWORD* pdwChannelMask);
    STDMETHODIMP GetSpeakerConfig(bool* pfCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18]);
    STDMETHODIMP SetSpeakerConfig(bool fCustomChannelMapping, DWORD pSpeakerToChannelMap[18][18]);
    STDMETHODIMP_(int) GetNumberOfInputChannels();
	STDMETHODIMP_(bool) IsDownSamplingTo441Enabled();
	STDMETHODIMP EnableDownSamplingTo441(bool fEnable);
	STDMETHODIMP_(REFERENCE_TIME) GetAudioTimeShift();
	STDMETHODIMP SetAudioTimeShift(REFERENCE_TIME rtAudioTimeShift);
};

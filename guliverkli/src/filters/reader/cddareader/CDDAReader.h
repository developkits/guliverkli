#pragma once

#include <atlbase.h>
#include <devioctl.h>
#include <ntddcdrm.h>
#include <qnetwork.h>
#include "..\asyncreader\asyncio.h"
#include "..\asyncreader\asyncrdr.h"

typedef struct {UINT chunkID; long chunkSize;} ChunkHeader;

#define RIFFID 'FFIR' 
#define WAVEID 'EVAW' 
typedef struct {ChunkHeader hdr; UINT WAVE;} RIFFChunk;

#define FormatID ' tmf' 
typedef struct {ChunkHeader hdr; PCMWAVEFORMAT pcm;} FormatChunk;

#define DataID 'atad'
typedef struct {ChunkHeader hdr;} DataChunk;

typedef struct {RIFFChunk riff; FormatChunk frm; DataChunk data;} WAVEChunck;

class CCDDAStream : public CAsyncStream
{
private:
    CCritSec m_csLock;

	LONGLONG m_llPosition, m_llLength;

	HANDLE m_hDrive;
	CDROM_TOC m_TOC;
	UINT m_nFirstSector, m_nStartSector, m_nStopSector;

	WAVEChunck m_header;

public:
	CCDDAStream();
	virtual ~CCDDAStream();

	CString m_discTitle, m_trackTitle;
	CString m_discArtist, m_trackArtist;

	bool Load(const WCHAR* fnw);

	// overrides
    HRESULT SetPointer(LONGLONG llPos);
    HRESULT Read(PBYTE pbBuffer, DWORD dwBytesToRead, BOOL bAlign, LPDWORD pdwBytesRead);
    LONGLONG Size(LONGLONG* pSizeAvailable);
    DWORD Alignment();
    void Lock();
	void Unlock();
};

[uuid("54A35221-2C8D-4a31-A5DF-6D809847E393")]
class CCDDAReader 
	: public CAsyncReader
	, public IFileSourceFilter
	, public IAMMediaContent
{
    CCDDAStream m_stream;
	CStringW m_fn;

public:
    CCDDAReader(IUnknown* pUnk, HRESULT* phr);
	~CCDDAReader();

#ifdef REGISTER_FILTER
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
#endif

	DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IFileSourceFilter

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	// IAMMediaContent

    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    STDMETHODIMP get_AuthorName(BSTR* pbstrAuthorName);
    STDMETHODIMP get_Title(BSTR* pbstrTitle);
    STDMETHODIMP get_Rating(BSTR* pbstrRating);
    STDMETHODIMP get_Description(BSTR* pbstrDescription);
    STDMETHODIMP get_Copyright(BSTR* pbstrCopyright);
    STDMETHODIMP get_BaseURL(BSTR* pbstrBaseURL);
    STDMETHODIMP get_LogoURL(BSTR* pbstrLogoURL);
    STDMETHODIMP get_LogoIconURL(BSTR* pbstrLogoURL);
    STDMETHODIMP get_WatermarkURL(BSTR* pbstrWatermarkURL);
    STDMETHODIMP get_MoreInfoURL(BSTR* pbstrMoreInfoURL);
    STDMETHODIMP get_MoreInfoBannerImage(BSTR* pbstrMoreInfoBannerImage);
    STDMETHODIMP get_MoreInfoBannerURL(BSTR* pbstrMoreInfoBannerURL);
    STDMETHODIMP get_MoreInfoText(BSTR* pbstrMoreInfoText);
};

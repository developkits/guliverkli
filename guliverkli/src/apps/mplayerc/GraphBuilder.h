#pragma once

#include <atlcoll.h>

#define LMERIT(m) ((ULONGLONG(m))<<16)
#define LMERIT_DO_USE LMERIT(MERIT_DO_NOT_USE+1)
#define LMERIT_UNLIKELY (LMERIT(MERIT_UNLIKELY))
#define LMERIT_NORMAL (LMERIT(MERIT_NORMAL))
#define LMERIT_PREFERRED (LMERIT(MERIT_PREFERRED))

class CGraphFilter
{
protected:
	CStringW m_name;
	struct {union {ULONGLONG val; struct {ULONGLONG low:16, mid:32, high:16;};};} m_merit;
	CLSID m_clsid;
	CList<GUID> m_guids;

public:
	CGraphFilter(CStringW name = L"", ULONGLONG merit = LMERIT_DO_USE);
	virtual ~CGraphFilter() {}

	CStringW GetName() {return(m_name);}
	ULONGLONG GetMerit() {return(m_merit.val);}
	DWORD GetDWORDMerit() {return(m_merit.mid);}
	CLSID GetCLSID() {return(m_clsid);}
	void GetGUIDs(CList<GUID>& guids) {guids.RemoveAll(); guids.AddTail(&m_guids);}
	void SetGUIDs(CList<GUID>& guids) {m_guids.RemoveAll(); m_guids.AddTail(&guids);}
	bool IsExactMatch(CArray<GUID>& guids);
	bool IsCompatible(CArray<GUID>& guids);

	virtual HRESULT Create(IBaseFilter** ppBF, IUnknown** ppUnk) = 0;
};

class CGraphRegFilter : public CGraphFilter
{
protected:
	CStringW m_dispname;
	CComPtr<IMoniker> m_pMoniker;

	void ExtractFilterData(BYTE* p, UINT len);

public:
	CGraphRegFilter(IMoniker* pMoniker, ULONGLONG merit = LMERIT_DO_USE);
	CGraphRegFilter(CStringW m_dispname, ULONGLONG merit = LMERIT_DO_USE);
	CGraphRegFilter(const CLSID& clsid, ULONGLONG merit = LMERIT_DO_USE);

	CStringW GetDispName() {return(m_dispname);}
	CComPtr<IMoniker> GetMoniker() {return(m_pMoniker);}

	HRESULT Create(IBaseFilter** ppBF, IUnknown** ppUnk);
};

class CGraphCustomFilter : public CGraphFilter
{
public:
	CGraphCustomFilter(const CLSID& clsid, CList<GUID>& guids, CStringW name = L"", ULONGLONG merit = LMERIT_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, IUnknown** ppUnk);
};

class CGraphFileFilter : public CGraphCustomFilter
{
protected:
	CString m_path;
	HINSTANCE m_hInst;

public:
	CGraphFileFilter(const CLSID& clsid, CList<GUID>& guids, CString path, CStringW name = L"", ULONGLONG merit = LMERIT_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, IUnknown** ppUnk);
};

class CGraphRendererFilter : public CGraphFilter
{
protected:
	CLSID m_clsid;
	HWND m_hWnd;

public:
	CGraphRendererFilter(const CLSID& clsid, HWND hWnd, CStringW name = L"", ULONGLONG merit = LMERIT_DO_USE);

	HRESULT Create(IBaseFilter** ppBF, IUnknown** ppUnk);
};

class CGraphBuilder
{
public:
	typedef struct {int nStream; CString filter, pin; CList<CMediaType> mts; CStringList path;} DeadEnd;

protected:
	HWND m_hWnd;
	CComPtr<IGraphBuilder> m_pGB;
	CComPtr<IFilterMapper2> m_pFM;
	CInterfaceList<IUnknown, &IID_IUnknown> m_pUnks;
	CAutoPtrList<CGraphFilter> m_pMoreFilters;
	ULONGLONG m_VRMerit, m_ARMerit;

	UINT m_nTotalStreams, m_nCurrentStream;
	CList<CGraphFilter*> m_chain;

	void ExtractMediaTypes(IPin* pPin, CArray<GUID>& guids);
	void ExtractMediaTypes(IPin* pPin, CList<CMediaType>& mts);

	void SaveFilters(CInterfaceList<IBaseFilter>& bfl);
	void RestoreFilters(CInterfaceList<IBaseFilter>& bfl);

	HRESULT SafeAddFilter(IBaseFilter* pBF, LPCWSTR pName);

	CAutoPtrArray<DeadEnd> m_DeadEnds;
/*
	CStringList m_log;
	void LOG(LPCTSTR fmt, ...);
*/
	HRESULT ConnectDirect(IPin* pPin, IBaseFilter* pBF, const AM_MEDIA_TYPE* pmt = NULL);

public:
	CGraphBuilder(IGraphBuilder* pGB, HWND hWnd);
	virtual ~CGraphBuilder();

	void Reset();

	void AddFilter(CGraphFilter* pFilter) {if(pFilter) {CAutoPtr<CGraphFilter> f(pFilter); m_pMoreFilters.AddTail(f);}}
	void RemoveFilters() {m_pMoreFilters.RemoveAll();}

	HRESULT Render(LPCTSTR fn);
	HRESULT Render(IBaseFilter* pBF);
	HRESULT Render(IPin* pPin);

	HRESULT FindInterface(REFIID iid, void** ppv);

	int GetStreamCount() {return m_nTotalStreams;}
	DeadEnd* GetDeadEnd(int i) {return (i >= 0 && i < (int)m_DeadEnds.GetCount()) ? (DeadEnd*)m_DeadEnds[i] : NULL;}
};

class CGraphBuilderFile : public CGraphBuilder
{
public:
	CGraphBuilderFile(IGraphBuilder* pGB, HWND hWnd);
};

class CGraphBuilderDVD : public CGraphBuilderFile
{
public:
	CGraphBuilderDVD(IGraphBuilder* pGB, HWND hWnd);

	HRESULT Render(CString fn, CString& path);
};

class CGraphBuilderCapture : public CGraphBuilderFile
{
public:
	CGraphBuilderCapture(IGraphBuilder* pGB, HWND hWnd);
};
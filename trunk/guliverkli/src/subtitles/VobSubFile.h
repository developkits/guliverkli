#pragma once

#include <afxtempl.h>
#include "VobSubImage.h"
#include "..\SubPic\ISubPic.h"

#define VOBSUBIDXVER 7

extern CString FindLangFromId(WORD id);

class CVobSubSettings
{
public:
	CSize m_size;
	int m_x, m_y;
	CPoint m_org;
	int m_scale_x, m_scale_y;	// % (don't set it to unsigned because as a side effect it will mess up negative coordinates in GetDestrect())
	int m_alpha;				// %
	int m_fSmooth;				// 0: OFF, 1: ON, 2: OLD (means no filtering at all)
	int m_fadein, m_fadeout;	// ms
	bool m_fAlign;
	int m_alignhor, m_alignver; // 0: left/top, 1: center, 2: right/bottom
	unsigned int m_toff;				// ms
	bool m_fOnlyShowForcedSubs;
	bool m_fCustomPal;
	int m_tridx;
	RGBQUAD m_orgpal[16], m_cuspal[4];

	CVobSubImage m_img;

	CVobSubSettings() {InitSettings();}
	void InitSettings();

	bool GetCustomPal(RGBQUAD* cuspal, int& tridx);
    void SetCustomPal(RGBQUAD* cuspal, int tridx);

	void GetDestrect(CRect& r); // destrect of m_img, considering the current alignment mode
	void GetDestrect(CRect& r, int w, int h); // this will scale it to the frame size of (w, h)

	void SetAlignment(bool fAlign, int x, int y, int hor, int ver);
};

[uuid("998D4C9A-460F-4de6-BDCD-35AB24F94ADF")]
class CVobSubFile : public CVobSubSettings, public ISubStream, public ISubPicProviderImpl
{
protected:
	CString m_title;

	void TrimExtension(CString& fn);
	bool ReadIdx(CString fn, int& ver), ReadSub(CString fn), ReadRar(CString fn), ReadIfo(CString fn);
	bool WriteIdx(CString fn), WriteSub(CString fn);

	CMemFile m_sub;

	BYTE* GetPacket(int idx, int& packetsize, int& datasize, int iLang = -1);
	bool GetFrame(int idx, int iLang = -1);
	bool GetFrameByTimeStamp(__int64 time);
	int GetFrameIdxByTimeStamp(__int64 time);

	bool SaveVobSub(CString fn);
	bool SaveWinSubMux(CString fn);
	bool SaveScenarist(CString fn);
	bool SaveMaestro(CString fn);

public:
	typedef struct
	{
		__int64 filepos;
		__int64 start, stop;
		bool fForced;
		char vobid, cellid;
		__int64 celltimestamp;
		bool fValid;
	} SubPos;

	typedef struct
	{
		int id;
		CString name, alt;
		CArray<SubPos> subpos;
	} SubLang;

	int m_iLang;
	SubLang m_langs[32];

public:
	CVobSubFile(CCritSec* pLock);
	virtual ~CVobSubFile();

	bool Copy(CVobSubFile& vsf);

	typedef enum {None,VobSub,WinSubMux,Scenarist,Maestro} SubFormat;

	bool Open(CString fn);
	bool Save(CString fn, SubFormat sf = VobSub);
	void Close();

	CString GetTitle() {return(m_title);}

	DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISubPicProvider
	STDMETHODIMP_(POSITION) GetStartPosition(REFERENCE_TIME rt, double fps);
	STDMETHODIMP_(POSITION) GetNext(POSITION pos);
	STDMETHODIMP_(REFERENCE_TIME) GetStart(POSITION pos, double fps);
	STDMETHODIMP_(REFERENCE_TIME) GetStop(POSITION pos, double fps);
	STDMETHODIMP_(bool) IsAnimated(POSITION pos);
	STDMETHODIMP Render(SubPicDesc& spd, REFERENCE_TIME rt, double fps, RECT& bbox);

	// IPersist
	STDMETHODIMP GetClassID(CLSID* pClassID);

	// ISubStream
	STDMETHODIMP_(int) GetStreamCount();
	STDMETHODIMP GetStreamInfo(int i, WCHAR** ppName, LCID* pLCID);
	STDMETHODIMP_(int) GetStream();
	STDMETHODIMP SetStream(int iStream);
};
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

#include <atlcoll.h>

class CBaseSplitterFile
{
	CComPtr<IAsyncReader> m_pAsyncReader;
	CAutoVectorPtr<BYTE> m_pCache;
	__int64 m_cachepos, m_cachelen, m_cachetotal;

	bool m_fStreaming;
	__int64 m_pos, m_len;

protected:
	UINT64 m_bitbuff;
	int m_bitlen;

	enum {DEFAULT_CACHELEN = 2048};

public:
	CBaseSplitterFile(IAsyncReader* pReader, HRESULT& hr, int cachelen = DEFAULT_CACHELEN);
	virtual ~CBaseSplitterFile() {}

	bool SetCacheSize(int cachelen = DEFAULT_CACHELEN);

	__int64 GetPos();
	__int64 GetLength();
	virtual void Seek(__int64 pos);
	virtual HRESULT Read(BYTE* pData, __int64 len);

	UINT64 BitRead(int nBits, bool fPeek = false);
	void BitByteAlign(), BitFlush();
	HRESULT ByteRead(BYTE* pData, __int64 len);

	bool IsStreaming() {return m_fStreaming;}
	HRESULT HasMoreData(__int64 len = 1, DWORD ms = 1);
};

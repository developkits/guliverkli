/* 
 *	Copyright (C) 2003-2006 Gabest
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
#include "fakefiltermapper2.h"
#include "MacrovisionKicker.h"
#include "..\..\..\include\detours\detours.h"
#include "..\..\DSUtil\DSUtil.h"

#include <initguid.h>
#include <qedit.h>

DETOUR_TRAMPOLINE(HRESULT WINAPI Real_CoCreateInstance(IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter, IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID FAR* ppv), CoCreateInstance);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegCloseKey(HKEY a0), RegCloseKey);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegFlushKey(HKEY a0), RegFlushKey);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegCreateKeyA(HKEY a0, LPCSTR a1, PHKEY a2), RegCreateKeyA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegCreateKeyW(HKEY a0, LPCWSTR a1, PHKEY a2), RegCreateKeyW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegCreateKeyExA(HKEY a0, LPCSTR a1, DWORD a2, LPSTR a3, DWORD a4, REGSAM a5, LPSECURITY_ATTRIBUTES a6, PHKEY a7, LPDWORD a8), RegCreateKeyExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegCreateKeyExW(HKEY a0, LPCWSTR a1, DWORD a2, LPWSTR a3, DWORD a4, REGSAM a5, LPSECURITY_ATTRIBUTES a6, PHKEY a7, LPDWORD a8), RegCreateKeyExW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegDeleteKeyA(HKEY a0, LPCSTR a1), RegDeleteKeyA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegDeleteKeyW(HKEY a0, LPCWSTR a1), RegDeleteKeyW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegDeleteValueA(HKEY a0, LPCSTR a1), RegDeleteValueA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegDeleteValueW(HKEY a0, LPCWSTR a1), RegDeleteValueW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegEnumKeyExA(HKEY a0, DWORD a1, LPSTR a2, LPDWORD a3, LPDWORD a4, LPSTR a5, LPDWORD a6, struct _FILETIME* a7), RegEnumKeyExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegEnumKeyExW(HKEY a0, DWORD a1, LPWSTR a2, LPDWORD a3, LPDWORD a4, LPWSTR a5, LPDWORD a6, struct _FILETIME* a7), RegEnumKeyExW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegEnumValueA(HKEY a0, DWORD a1, LPSTR a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPBYTE a6, LPDWORD a7), RegEnumValueA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegEnumValueW(HKEY a0, DWORD a1, LPWSTR a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPBYTE a6, LPDWORD a7), RegEnumValueW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegOpenKeyA(HKEY a0, LPCSTR a1, PHKEY a2), RegOpenKeyA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegOpenKeyW(HKEY a0, LPCWSTR a1, PHKEY a2), RegOpenKeyW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegOpenKeyExA(HKEY a0, LPCSTR a1, DWORD a2, REGSAM a3, PHKEY a4), RegOpenKeyExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegOpenKeyExW(HKEY a0, LPCWSTR a1, DWORD a2, REGSAM a3, PHKEY a4), RegOpenKeyExW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryInfoKeyA(HKEY a0, LPSTR a1, LPDWORD a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPDWORD a6, LPDWORD a7, LPDWORD a8, LPDWORD a9, LPDWORD a10, struct _FILETIME* a11), RegQueryInfoKeyA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryInfoKeyW(HKEY a0, LPWSTR a1, LPDWORD a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPDWORD a6, LPDWORD a7, LPDWORD a8, LPDWORD a9, LPDWORD a10, struct _FILETIME* a11), RegQueryInfoKeyW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryValueA(HKEY a0, LPCSTR a1, LPSTR a2, PLONG a3), RegQueryValueA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryValueW(HKEY a0, LPCWSTR a1, LPWSTR a2, PLONG a3), RegQueryValueW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryValueExA(HKEY a0, LPCSTR a1, LPDWORD a2, LPDWORD a3, LPBYTE a4, LPDWORD a5), RegQueryValueExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegQueryValueExW(HKEY a0, LPCWSTR a1, LPDWORD a2, LPDWORD a3, LPBYTE a4, LPDWORD a5), RegQueryValueExW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegSetValueA(HKEY a0, LPCSTR a1, DWORD a2, LPCSTR a3, DWORD a4), RegSetValueA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegSetValueW(HKEY a0, LPCWSTR a1, DWORD a2, LPCWSTR a3, DWORD a4), RegSetValueW);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegSetValueExA(HKEY a0, LPCSTR a1, DWORD a2, DWORD a3, BYTE* a4, DWORD a5), RegSetValueExA);
DETOUR_TRAMPOLINE(LONG WINAPI Real_RegSetValueExW(HKEY a0, LPCWSTR a1, DWORD a2, DWORD a3, BYTE* a4, DWORD a5), RegSetValueExW);

HRESULT WINAPI Mine_CoCreateInstance(IN REFCLSID rclsid, IN LPUNKNOWN pUnkOuter,
                    IN DWORD dwClsContext, IN REFIID riid, OUT LPVOID FAR* ppv)
{
	if(CFilterMapper2::m_pFilterMapper2)
	{
		CheckPointer(ppv, E_POINTER);

		if(rclsid == CLSID_FilterMapper)
		{
			ASSERT(0);
			return REGDB_E_CLASSNOTREG; // sorry...
		}

		if(rclsid == CLSID_FilterMapper2)
		{
			if(pUnkOuter) return CLASS_E_NOAGGREGATION;

			if(riid == __uuidof(IUnknown))
			{
				CFilterMapper2::m_pFilterMapper2->AddRef();
				*ppv = (IUnknown*)CFilterMapper2::m_pFilterMapper2;
				return S_OK;
			}
			else if(riid == __uuidof(IFilterMapper2))
			{
				CFilterMapper2::m_pFilterMapper2->AddRef();
				*ppv = (IFilterMapper2*)CFilterMapper2::m_pFilterMapper2;
				return S_OK;
			}
			else
			{
				return E_NOINTERFACE;
			}
		}
	}
/*	else
	{
		if(rclsid == CLSID_FilterMapper2)
		{
			CFilterMapper2* pFM2 = new CFilterMapper2(true, false, pUnkOuter);
			CComPtr<IUnknown> pUnk = (IUnknown*)pFM2;
			return pUnk->QueryInterface(riid, ppv);
		}
	}
*/
	if(!pUnkOuter)
	if(rclsid == CLSID_VideoMixingRenderer || rclsid == CLSID_VideoMixingRenderer9
	|| rclsid == CLSID_VideoRenderer || rclsid == CLSID_VideoRendererDefault
	|| rclsid == CLSID_OverlayMixer)// || rclsid == CLSID_OverlayMixer2 - where is this declared?)
	{
		CMacrovisionKicker* pMK = new CMacrovisionKicker(NAME("CMacrovisionKicker"), NULL);
		CComPtr<IUnknown> pUnk = (IUnknown*)(INonDelegatingUnknown*)pMK;
		CComPtr<IUnknown> pInner;
		HRESULT hr;
		if(SUCCEEDED(hr = Real_CoCreateInstance(rclsid, pUnk, dwClsContext, __uuidof(IUnknown), (void**)&pInner)))
		{
			pMK->SetInner(pInner);
			return pUnk->QueryInterface(riid, ppv);
		}
	}

	return Real_CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

#define FAKEHKEY (HKEY)0x12345678

LONG WINAPI Mine_RegCloseKey(HKEY a0)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_SUCCESS;}
	return Real_RegCloseKey(a0);
}
LONG WINAPI Mine_RegFlushKey(HKEY a0)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_SUCCESS;}
	return Real_RegFlushKey(a0);
}
LONG WINAPI Mine_RegCreateKeyA(HKEY a0, LPCSTR a1, PHKEY a2)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a2 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegCreateKeyA(a0, a1, a2);
}
LONG WINAPI Mine_RegCreateKeyW(HKEY a0, LPCWSTR a1, PHKEY a2)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a2 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegCreateKeyW(a0, a1, a2);
}
LONG WINAPI Mine_RegCreateKeyExA(HKEY a0, LPCSTR a1, DWORD a2, LPSTR a3, DWORD a4, REGSAM a5, LPSECURITY_ATTRIBUTES a6, PHKEY a7, LPDWORD a8)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a7 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegCreateKeyExA(a0, a1, a2, a3, a4, a5, a6, a7, a8);
}
LONG WINAPI Mine_RegCreateKeyExW(HKEY a0, LPCWSTR a1, DWORD a2, LPWSTR a3, DWORD a4, REGSAM a5, LPSECURITY_ATTRIBUTES a6, PHKEY a7, LPDWORD a8)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a7 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegCreateKeyExW(a0, a1, a2, a3, a4, a5, a6, a7, a8);
}
LONG WINAPI Mine_RegDeleteKeyA(HKEY a0, LPCSTR a1)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
	return Real_RegDeleteKeyA(a0, a1);
}
LONG WINAPI Mine_RegDeleteKeyW(HKEY a0, LPCWSTR a1)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
	return Real_RegDeleteKeyW(a0, a1);
}
LONG WINAPI Mine_RegDeleteValueA(HKEY a0, LPCSTR a1)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegDeleteValueA(a0, a1);
}
LONG WINAPI Mine_RegDeleteValueW(HKEY a0, LPCWSTR a1)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegDeleteValueW(a0, a1);
}
LONG WINAPI Mine_RegEnumKeyExA(HKEY a0, DWORD a1, LPSTR a2, LPDWORD a3, LPDWORD a4, LPSTR a5, LPDWORD a6, struct _FILETIME* a7)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_NO_MORE_ITEMS;}
    return Real_RegEnumKeyExA(a0, a1, a2, a3, a4, a5, a6, a7);
}
LONG WINAPI Mine_RegEnumKeyExW(HKEY a0, DWORD a1, LPWSTR a2, LPDWORD a3, LPDWORD a4, LPWSTR a5, LPDWORD a6, struct _FILETIME* a7)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_NO_MORE_ITEMS;}
    return Real_RegEnumKeyExW(a0, a1, a2, a3, a4, a5, a6, a7);
}
LONG WINAPI Mine_RegEnumValueA(HKEY a0, DWORD a1, LPSTR a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPBYTE a6, LPDWORD a7)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_NO_MORE_ITEMS;}
    return Real_RegEnumValueA(a0, a1, a2, a3, a4, a5, a6, a7);
}
LONG WINAPI Mine_RegEnumValueW(HKEY a0, DWORD a1, LPWSTR a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPBYTE a6, LPDWORD a7)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_NO_MORE_ITEMS;}
    return Real_RegEnumValueW(a0, a1, a2, a3, a4, a5, a6, a7);
}
LONG WINAPI Mine_RegOpenKeyA(HKEY a0, LPCSTR a1, PHKEY a2)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a2 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegOpenKeyA(a0, a1, a2);
}
LONG WINAPI Mine_RegOpenKeyW(HKEY a0, LPCWSTR a1, PHKEY a2)
{
	if(CFilterMapper2::m_pFilterMapper2) {*a2 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegOpenKeyW(a0, a1, a2);
}
LONG WINAPI Mine_RegOpenKeyExA(HKEY a0, LPCSTR a1, DWORD a2, REGSAM a3, PHKEY a4)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a3&(KEY_SET_VALUE|KEY_CREATE_SUB_KEY))) {*a4 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegOpenKeyExA(a0, a1, a2, a3, a4);
}
LONG WINAPI Mine_RegOpenKeyExW(HKEY a0, LPCWSTR a1, DWORD a2, REGSAM a3, PHKEY a4)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a3&(KEY_SET_VALUE|KEY_CREATE_SUB_KEY))) {*a4 = FAKEHKEY; return ERROR_SUCCESS;}
	return Real_RegOpenKeyExW(a0, a1, a2, a3, a4);
}
LONG WINAPI Mine_RegQueryInfoKeyA(HKEY a0, LPSTR a1, LPDWORD a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPDWORD a6, LPDWORD a7, LPDWORD a8, LPDWORD a9, LPDWORD a10, struct _FILETIME* a11)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_INVALID_HANDLE;}
	return Real_RegQueryInfoKeyA(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}
LONG WINAPI Mine_RegQueryInfoKeyW(HKEY a0, LPWSTR a1, LPDWORD a2, LPDWORD a3, LPDWORD a4, LPDWORD a5, LPDWORD a6, LPDWORD a7, LPDWORD a8, LPDWORD a9, LPDWORD a10, struct _FILETIME* a11)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {return ERROR_INVALID_HANDLE;}
	return Real_RegQueryInfoKeyW(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
}
LONG WINAPI Mine_RegQueryValueA(HKEY a0, LPCSTR a1, LPSTR a2, PLONG a3)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {*a3 = 0; return ERROR_SUCCESS;}
    return Real_RegQueryValueA(a0, a1, a2, a3);
}
LONG WINAPI Mine_RegQueryValueW(HKEY a0, LPCWSTR a1, LPWSTR a2, PLONG a3)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {*a3 = 0; return ERROR_SUCCESS;}
    return Real_RegQueryValueW(a0, a1, a2, a3);
}
LONG WINAPI Mine_RegQueryValueExA(HKEY a0, LPCSTR a1, LPDWORD a2, LPDWORD a3, LPBYTE a4, LPDWORD a5)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {*a5 = 0; return ERROR_SUCCESS;}
    return Real_RegQueryValueExA(a0, a1, a2, a3, a4, a5);
}
LONG WINAPI Mine_RegQueryValueExW(HKEY a0, LPCWSTR a1, LPDWORD a2, LPDWORD a3, LPBYTE a4, LPDWORD a5)
{
	if(CFilterMapper2::m_pFilterMapper2 && a0 == FAKEHKEY) {*a5 = 0; return ERROR_SUCCESS;}
    return Real_RegQueryValueExW(a0, a1, a2, a3, a4, a5);
}
LONG WINAPI Mine_RegSetValueA(HKEY a0, LPCSTR a1, DWORD a2, LPCSTR a3, DWORD a4)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegSetValueA(a0, a1, a2, a3, a4);
}
LONG WINAPI Mine_RegSetValueW(HKEY a0, LPCWSTR a1, DWORD a2, LPCWSTR a3, DWORD a4)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegSetValueW(a0, a1, a2, a3, a4);
}
LONG WINAPI Mine_RegSetValueExA(HKEY a0, LPCSTR a1, DWORD a2, DWORD a3, BYTE* a4, DWORD a5)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegSetValueExA(a0, a1, a2, a3, a4, a5);
}
LONG WINAPI Mine_RegSetValueExW(HKEY a0, LPCWSTR a1, DWORD a2, DWORD a3, BYTE* a4, DWORD a5)
{
	if(CFilterMapper2::m_pFilterMapper2 && (a0 == FAKEHKEY || (int)a0 < 0)) {return ERROR_SUCCESS;}
    return Real_RegSetValueExW(a0, a1, a2, a3, a4, a5);
}

//
// CFilterMapper2
//

IFilterMapper2* CFilterMapper2::m_pFilterMapper2 = NULL;

bool CFilterMapper2::fInitialized = false;

void CFilterMapper2::Init()
{
	if(!fInitialized)
	{
		DetourFunctionWithTrampoline((PBYTE)Real_CoCreateInstance, (PBYTE)Mine_CoCreateInstance);
		DetourFunctionWithTrampoline((PBYTE)Real_RegCloseKey, (PBYTE)Mine_RegCloseKey);
		DetourFunctionWithTrampoline((PBYTE)Real_RegFlushKey, (PBYTE)Mine_RegFlushKey);
		DetourFunctionWithTrampoline((PBYTE)Real_RegCreateKeyA, (PBYTE)Mine_RegCreateKeyA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegCreateKeyW, (PBYTE)Mine_RegCreateKeyW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegCreateKeyExA, (PBYTE)Mine_RegCreateKeyExA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegCreateKeyExW, (PBYTE)Mine_RegCreateKeyExW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegDeleteKeyA, (PBYTE)Mine_RegDeleteKeyA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegDeleteKeyW, (PBYTE)Mine_RegDeleteKeyW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegDeleteValueA, (PBYTE)Mine_RegDeleteValueA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegDeleteValueW, (PBYTE)Mine_RegDeleteValueW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegEnumKeyExA, (PBYTE)Mine_RegEnumKeyExA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegEnumKeyExW, (PBYTE)Mine_RegEnumKeyExW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegEnumValueA, (PBYTE)Mine_RegEnumValueA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegEnumValueW, (PBYTE)Mine_RegEnumValueW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegOpenKeyA, (PBYTE)Mine_RegOpenKeyA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegOpenKeyW, (PBYTE)Mine_RegOpenKeyW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegOpenKeyExA, (PBYTE)Mine_RegOpenKeyExA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegOpenKeyExW, (PBYTE)Mine_RegOpenKeyExW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryInfoKeyA, (PBYTE)Mine_RegQueryInfoKeyA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryInfoKeyW, (PBYTE)Mine_RegQueryInfoKeyW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryValueA, (PBYTE)Mine_RegQueryValueA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryValueW, (PBYTE)Mine_RegQueryValueW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryValueExA, (PBYTE)Mine_RegQueryValueExA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegQueryValueExW, (PBYTE)Mine_RegQueryValueExW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegSetValueA, (PBYTE)Mine_RegSetValueA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegSetValueW, (PBYTE)Mine_RegSetValueW);
		DetourFunctionWithTrampoline((PBYTE)Real_RegSetValueExA, (PBYTE)Mine_RegSetValueExA);
		DetourFunctionWithTrampoline((PBYTE)Real_RegSetValueExW, (PBYTE)Mine_RegSetValueExW);

		fInitialized = true;
	}
}

CFilterMapper2::CFilterMapper2(bool fRefCounted, bool fAllowUnreg, LPUNKNOWN pUnkOuter)
	: CUnknown(NAME("CFilterMapper2"), pUnkOuter)
	, m_fRefCounted(fRefCounted), m_fAllowUnreg(fAllowUnreg)
{
	m_cRef = fRefCounted ? 0 : 1;

	Init();

	HRESULT hr = Real_CoCreateInstance(
		CLSID_FilterMapper2, (IUnknown*)(INonDelegatingUnknown*)this, CLSCTX_ALL, 
		__uuidof(IUnknown), (void**)&m_pFM2);
	if(FAILED(hr) || !m_pFM2)
	{
		ASSERT(0);
		return;
	}
}

CFilterMapper2::~CFilterMapper2()
{
	POSITION pos = m_filters.GetHeadPosition();
	while(pos) delete m_filters.GetNext(pos);
}

STDMETHODIMP CFilterMapper2::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	if(riid == __uuidof(IFilterMapper2))
		return GetInterface((IFilterMapper2*)this, ppv);

	HRESULT hr = m_pFM2 ? m_pFM2->QueryInterface(riid, ppv) : E_NOINTERFACE;

	return 
		SUCCEEDED(hr) ? hr :
		__super::NonDelegatingQueryInterface(riid, ppv);
}

void CFilterMapper2::Register(CString path)
{
	if(HMODULE h = LoadLibrary(path))
	{
		typedef HRESULT (__stdcall * PDllRegisterServer)();
		if(PDllRegisterServer p = (PDllRegisterServer)GetProcAddress(h, "DllRegisterServer"))
		{
			ASSERT(CFilterMapper2::m_pFilterMapper2 == NULL);

			CFilterMapper2::m_pFilterMapper2 = this;
			m_path = path;
			p();
			m_path.Empty();
			CFilterMapper2::m_pFilterMapper2 = NULL;
		}

		FreeLibrary(h);
	}
}

// IFilterMapper2

STDMETHODIMP CFilterMapper2::CreateCategory(REFCLSID clsidCategory, DWORD dwCategoryMerit, LPCWSTR Description)
{
	if(!m_path.IsEmpty())
	{
		return S_OK;
	}
	else if(CComQIPtr<IFilterMapper2> pFM2 = m_pFM2)
	{
		return pFM2->CreateCategory(clsidCategory, dwCategoryMerit, Description);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CFilterMapper2::UnregisterFilter(const CLSID* pclsidCategory, const OLECHAR* szInstance, REFCLSID Filter)
{
	if(!m_path.IsEmpty())
	{
		return S_OK;
	}
	else if(CComQIPtr<IFilterMapper2> pFM2 = m_pFM2)
	{
		return m_fAllowUnreg 
			? pFM2->UnregisterFilter(pclsidCategory, szInstance, Filter) 
			: S_OK;
	}

	return E_NOTIMPL;
}
	
STDMETHODIMP CFilterMapper2::RegisterFilter(REFCLSID clsidFilter, LPCWSTR Name, IMoniker** ppMoniker, const CLSID* pclsidCategory, const OLECHAR* szInstance, const REGFILTER2* prf2)
{
	if(!m_path.IsEmpty())
	{
		if(FilterOverride* f = new FilterOverride)
		{
			f->fDisabled = false;
			f->type = FilterOverride::EXTERNAL;
			f->path = m_path;
			f->name = CStringW(Name);
			f->clsid = clsidFilter;
			f->iLoadType = FilterOverride::MERIT;
			f->dwMerit = prf2->dwMerit;

			if(prf2->dwVersion == 1)
			{
				for(ULONG i = 0; i < prf2->cPins; i++)
				{
					const REGFILTERPINS& rgPin = prf2->rgPins[i];
					if(rgPin.bOutput) continue;

					for(UINT i = 0; i < rgPin.nMediaTypes; i++)
					{
						if(!rgPin.lpMediaType[i].clsMajorType || !rgPin.lpMediaType[i].clsMinorType) break;
						f->guids.AddTail(*rgPin.lpMediaType[i].clsMajorType);
						f->guids.AddTail(*rgPin.lpMediaType[i].clsMinorType);
					}
				}
			}
			else if(prf2->dwVersion == 2)
			{
				for(ULONG i = 0; i < prf2->cPins2; i++)
				{
					const REGFILTERPINS2& rgPin = prf2->rgPins2[i];
					if(rgPin.dwFlags&REG_PINFLAG_B_OUTPUT) continue;

					for(UINT i = 0; i < rgPin.nMediaTypes; i++)
					{
						if(!rgPin.lpMediaType[i].clsMajorType || !rgPin.lpMediaType[i].clsMinorType) break;
						f->guids.AddTail(*rgPin.lpMediaType[i].clsMajorType);
						f->guids.AddTail(*rgPin.lpMediaType[i].clsMinorType);
					}
				}
			}

			f->backup.AddTailList(&f->guids);

			m_filters.AddTail(f);
		}

		return S_OK;
	}
	else if(CComQIPtr<IFilterMapper2> pFM2 = m_pFM2)
	{
		return pFM2->RegisterFilter(clsidFilter, Name, ppMoniker, pclsidCategory, szInstance, prf2);
	}

	return E_NOTIMPL;
}

STDMETHODIMP CFilterMapper2::EnumMatchingFilters(IEnumMoniker** ppEnum, DWORD dwFlags, BOOL bExactMatch, DWORD dwMerit, 
	BOOL bInputNeeded, DWORD cInputTypes, const GUID* pInputTypes, const REGPINMEDIUM* pMedIn, const CLSID* pPinCategoryIn, BOOL bRender, 
	BOOL bOutputNeeded, DWORD cOutputTypes, const GUID* pOutputTypes, const REGPINMEDIUM* pMedOut, const CLSID* pPinCategoryOut)
{
	if(CComQIPtr<IFilterMapper2> pFM2 = m_pFM2)
	{
		pFM2->EnumMatchingFilters(ppEnum, dwFlags, bExactMatch, dwMerit, 
			bInputNeeded, cInputTypes, pInputTypes, pMedIn, pPinCategoryIn, bRender, 
			bOutputNeeded, cOutputTypes, pOutputTypes, pMedOut, pPinCategoryOut);
	}

	return E_NOTIMPL;
}

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#pragma warning(disable: 4996)

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0510		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0400	// Change this to the appropriate value to target Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
/*
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
*/
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxmt.h>
#include <atlbase.h>
#include <atlcoll.h>
#include <atlpath.h>
#include <ddraw.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "../GSdx/GSRendererHW.h"
#include "../GSdx/GSRendererSW.h"
#include "../GSdx/GSRendererNull.h"

#define countof(a) (sizeof(a)/sizeof(a[0]))

#define EXPORT_C extern "C" __declspec(dllexport) void __stdcall
#define EXPORT_C_(type) extern "C" __declspec(dllexport) type __stdcall

#pragma warning(disable : 4995 4324)

#define D3DCOLORWRITEENABLE_RGB (D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE)
#define D3DCOLORWRITEENABLE_RGBA (D3DCOLORWRITEENABLE_RGB|D3DCOLORWRITEENABLE_ALPHA)

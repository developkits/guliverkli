/* 
 *	Copyright (C) 2003 Gabest
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
 *  Note: This interface was defined for the matroska container format 
 *  originally, but can be implemented for other formats as well.
 *
 */

#pragma once

// 30323449-0000-0010-8000-00AA00389B71  'I420' == MEDIASUBTYPE_I420
DEFINE_GUID(MEDIASUBTYPE_I420,
0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#define WAVE_FORMAT_DOLBY_AC3 0x2000
// {00002000-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_WAVE_DOLBY_AC3, 
WAVE_FORMAT_DOLBY_AC3, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#define WAVE_FORMAT_DVD_DTS 0x2001
// {00002001-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_WAVE_DTS, 
WAVE_FORMAT_DVD_DTS, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// Be compatible with 3ivx
#define WAVE_FORMAT_AAC 0x00FF
// {000000FF-0000-0010-8000-00AA00389B71}
DEFINE_GUID(MEDIASUBTYPE_AAC,
WAVE_FORMAT_AAC, 0x000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#define WAVE_FORMAT_MP3 0x0055
// 00000055-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_MP3,
0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);


//
// RealMedia
//


// {57428EC6-C2B2-44a2-AA9C-28F0B6A5C48E}
DEFINE_GUID(MEDIASUBTYPE_RealMedia, 
0x57428ec6, 0xc2b2, 0x44a2, 0xaa, 0x9c, 0x28, 0xf0, 0xb6, 0xa5, 0xc4, 0x8e);

// 30315652-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_RV10,
0x30315652, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 30325652-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_RV20,
0x30325652, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 30335652-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_RV30,
0x30335652, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 30345652-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_RV40,
0x30345652, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 345f3431-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_14_4,
0x345f3431, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 385f3832-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_28_8,
0x385f3832, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 43525441-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_ATRC,
0x43525441, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 4b4f4f43-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_COOK,
0x4b4f4f43, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 54454e44-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_DNET,
0x54454e44, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// 52504953-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_SIPR,
0x52504953, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

enum 
{
	WAVE_FORMAT_14_4 = 0x2002,
	WAVE_FORMAT_28_8 = 0x2003,
	WAVE_FORMAT_ATRC = 0x0270, //WAVE_FORMAT_SONY_SCX,
	WAVE_FORMAT_COOK = 0x2004,
	WAVE_FORMAT_DNET = 0x2005,
	WAVE_FORMAT_SIPR = 0x0130, //WAVE_FORMAT_SIPROLAB_ACEPLNET,
};

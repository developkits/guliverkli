#include "stdafx.h"
#include "BaseSource.h"
#include "..\..\..\DSUtil\DSUtil.h"

//
// CBaseSource
//

//
// CBaseStream
//

CBaseStream::CBaseStream(TCHAR* name, CSource* pParent, HRESULT* phr) 
	: CSourceStream(name, phr, pParent, L"Output")
	, CSourceSeeking(name, (IPin*)this, phr, &m_cSharedState)
	, m_bDiscontinuity(FALSE), m_bFlushing(FALSE)
{
	CAutoLock cAutoLock(&m_cSharedState);

	m_AvgTimePerFrame = 0;
	m_rtDuration = 0;
	m_rtStop = m_rtDuration;
}

CBaseStream::~CBaseStream()
{
	CAutoLock cAutoLock(&m_cSharedState);
}

STDMETHODIMP CBaseStream::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    CheckPointer(ppv, E_POINTER);

	return (riid == IID_IMediaSeeking) ? CSourceSeeking::NonDelegatingQueryInterface(riid, ppv)
		: CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

void CBaseStream::UpdateFromSeek()
{
	if(ThreadExists())
	{
		// next time around the loop, the worker thread will
		// pick up the position change.
		// We need to flush all the existing data - we must do that here
		// as our thread will probably be blocked in GetBuffer otherwise
	    
		m_bFlushing = TRUE;

		DeliverBeginFlush();
		// make sure we have stopped pushing
		Stop();
		// complete the flush
		DeliverEndFlush();

        m_bFlushing = FALSE;

		// restart
		Run();
	}
}

HRESULT CBaseStream::SetRate(double dRate)
{
	if(dRate <= 0)
		return E_INVALIDARG;

	{
		CAutoLock lock(CSourceSeeking::m_pLock);
		m_dRateSeeking = dRate;
	}

	UpdateFromSeek();

	return S_OK;
}

HRESULT CBaseStream::OnThreadStartPlay()
{
    m_bDiscontinuity = TRUE;
    return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}

HRESULT CBaseStream::ChangeStart()
{
    {
        CAutoLock lock(CSourceSeeking::m_pLock);
		m_rtSampleTime = 0;
		m_rtPosition = m_rtStart;
    }

    UpdateFromSeek();

    return S_OK;
}

HRESULT CBaseStream::ChangeStop()
{
    {
        CAutoLock lock(CSourceSeeking::m_pLock);
        if(m_rtPosition < m_rtStop)
			return S_OK;
    }

    // We're already past the new stop time -- better flush the graph.
    UpdateFromSeek();

    return S_OK;
}

HRESULT CBaseStream::OnThreadCreate()
{
    CAutoLock cAutoLockShared(&m_cSharedState);

    m_rtSampleTime = 0;
    m_rtPosition = m_rtStart;

    return CSourceStream::OnThreadCreate();
}

HRESULT CBaseStream::FillBuffer(IMediaSample* pSample)
{
	HRESULT hr;

	{
		CAutoLock cAutoLockShared(&m_cSharedState);

        if(m_rtPosition >= m_rtStop)
			return S_FALSE;

		BYTE* pOut = NULL;
		if(FAILED(hr = pSample->GetPointer(&pOut)) || !pOut)
			return S_FALSE;

		int nFrame = m_rtPosition / m_AvgTimePerFrame; // (int)(1.0 * m_rtPosition / m_AvgTimePerFrame + 0.5);

		long len = pSample->GetSize();

		hr = FillBuffer(pSample, nFrame, pOut, len);
		if(hr != S_OK) return hr;

		pSample->SetActualDataLength(len);

		REFERENCE_TIME rtStart, rtStop;
        // The sample times are modified by the current rate.
        rtStart = static_cast<REFERENCE_TIME>(m_rtSampleTime / m_dRateSeeking);
        rtStop  = rtStart + static_cast<int>(m_AvgTimePerFrame / m_dRateSeeking);
		pSample->SetTime(&rtStart, &rtStop);

        m_rtSampleTime += m_AvgTimePerFrame;
        m_rtPosition += m_AvgTimePerFrame;
	}

	pSample->SetSyncPoint(TRUE);

	if(m_bDiscontinuity) 
    {
		pSample->SetDiscontinuity(TRUE);
		m_bDiscontinuity = FALSE;
	}

	return S_OK;
}

STDMETHODIMP CBaseStream::Notify(IBaseFilter* pSender, Quality q)
{
	return E_NOTIMPL;
}
#include "stdafx.h"
#include <assert.h>
#include "EventBase.h"

CEventBase::CEventBase(void)
{
    m_pBase = NULL;
    m_pThread = NULL;
    m_nIoState = 0;
}

CEventBase::~CEventBase(void)
{
    assert(NULL == m_pBase);
    assert(NULL == m_pThread);
    assert(0 == m_nIoState);
}

bool CEventBase::Init(void)
{
    bool bInit = false;
    if (NULL == m_pBase)
    {
        struct event_config *pCfg = event_config_new();
        //event_config_require_features(cfg, EV_FEATURE_EARLY_CLOSE);
        m_pBase = event_base_new_with_config(pCfg);
        event_config_free(pCfg);
        m_nIoState = 1;
        bInit = true;
    }
    return bInit;
}

void CEventBase::Uninit(void)
{
    if (NULL != m_pBase)
    {
        event_base_free(m_pBase);
        m_pBase = NULL;
    }
    m_nIoState = 0;
}

int CEventBase::Start(void)
{
    int nStart = 2;
    if (NULL == m_pThread)
    {
        m_pThread = new std::thread(WorkShell, this);
        if (NULL != m_pThread)
        {
            nStart = 0;
            m_nIoState = 2;
        }
    }
    else
        nStart = 1;
    return nStart;
}

void CEventBase::Stop(void)
{
    if (NULL != m_pThread)
    {
        if (NULL != m_pBase)
            event_base_loopbreak(m_pBase);
        m_pThread->join();
        delete m_pThread;
        m_pThread = NULL;
    }
    m_nIoState = 4;
}

void CEventBase::NotifyStop(void)
{
    m_nIoState = 3;
}

void CEventBase::WorkShell(void *pContext)
{
    CEventBase *pEvent = (CEventBase *)pContext;
    pEvent->WorkFunc();
}

void CEventBase::WorkFunc(void)
{
    event_base_loop(m_pBase, EVLOOP_NO_EXIT_ON_EMPTY);
}

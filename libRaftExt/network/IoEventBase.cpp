#include "stdafx.h"
#include <assert.h>
#include "IoEventBase.h"
#include "EventSession.h"
#include "SequenceID.h"

CIoEventBase::CIoEventBase(CSequenceID *pSessionSeqID)
{
    m_pSessionSeqID = pSessionSeqID;
}

CIoEventBase::~CIoEventBase(void)
{
}

void CIoEventBase::Uninit(void)
{
    for (auto itSession = m_mapSession.begin(); itSession != m_mapSession.end(); itSession++)
    {
        CEventSession *pSession = itSession->second;
        struct bufferevent *pBufferEvent = pSession->GetBufferEvent();
        if(NULL != pBufferEvent)
            bufferevent_free(pBufferEvent);
        delete pSession;
    }
    m_mapSession.clear();
    CEventBase::Uninit();
}

CEventSession * CIoEventBase::Connect(const std::string &strHost, int nPort,uint32_t nSessionID)
{
    bool bConnect = false;
    struct event_base *pBase = GetBase();
    struct bufferevent *pBev = bufferevent_socket_new(pBase, -1,BEV_OPT_CLOSE_ON_FREE| BEV_OPT_THREADSAFE);
    CEventSession *pSession = CreateClientSession(pBev, strHost, nPort, nSessionID);
    if (NULL != pSession)
    {
        if (0 == pSession->Connect(strHost, nPort))
            bConnect = true;
        else
        {
            DestroySession(pSession);
            pSession = NULL;
        }
    }
    return pSession;
}

CEventSession *CIoEventBase::CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID)
{
    CEventSession *pSession = NULL;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    if (m_mapSession.find(nSessionID) == m_mapSession.end())
    {
        pSession = new CEventSession(this, pBufferEvent, strHost, nPort, nSessionID);
        m_mapSession[nSessionID] = pSession;
    }
    return pSession;
}

CEventSession *CIoEventBase::CreateServiceSession(struct bufferevent *pBufferEvent)
{
    CEventSession *pSession = NULL;
    uint32_t nSessionID;
    bool bCreate = m_pSessionSeqID->AllocSeqID(nSessionID);
    if (bCreate)
    {
        std::lock_guard<std::mutex> guardSession(m_mutexSession);
        assert(m_mapSession.find(nSessionID) == m_mapSession.end());
        pSession = new CEventSession(this, pBufferEvent, nSessionID);
        m_mapSession[nSessionID] = pSession;
    }
    return pSession;
}

bool CIoEventBase::DestroySession(CEventSession *pSession)
{
    bool bDestroy = false;
    uint32_t nSessionID = pSession->GetSessionID();
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    auto itFound = m_mapSession.find(nSessionID);
    if (itFound != m_mapSession.end())
    {
        struct bufferevent *pBufferEvent = pSession->GetBufferEvent();
        bufferevent_free(pBufferEvent);
        m_mapSession.erase(itFound);
        if (!pSession->IsClient())
            m_pSessionSeqID->FreeSeqID(nSessionID);
        delete pSession;
        bDestroy = true;
    }
    return bDestroy;
}

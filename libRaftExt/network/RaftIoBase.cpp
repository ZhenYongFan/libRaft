#include "stdafx.h"
#include "RaftIoBase.h"
#include "RaftSession.h"
#include "RaftQueue.h"
#include "rpc.pb.h"
#include "SequenceID.h"

CRaftIoBase::CRaftIoBase(CSequenceID *pSessionSeqID)
    :CIoEventBase(pSessionSeqID)
{
    m_pMsgQueue = NULL;
}

CRaftIoBase::~CRaftIoBase()
{
}

void CRaftIoBase::SetMsgQueue(CRaftQueue *pQueue)
{
    m_pMsgQueue = pQueue;
}

bool CRaftIoBase::TrySendRaftMsg(uint32_t nSessionID, google::protobuf::MessageLite * pMsg)
{
    bool bSent = false;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    if (m_mapSession.find(nSessionID) != m_mapSession.end())
    {
        CRaftSession *pSession = dynamic_cast<CRaftSession *>(m_mapSession[nSessionID]);
        if (pSession->IsConnected())
        {
            std::string strMsg = pMsg->SerializeAsString();
            if(0 == pSession->SendMsg(strMsg))
                bSent = true;
        }
    }
    return bSent;
}

CEventSession *CRaftIoBase::CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID)
{
    CRaftSession *pSession = NULL;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    if (m_mapSession.find(nSessionID) == m_mapSession.end())
    {
        pSession = new CRaftSession(this, pBufferEvent, strHost, nPort, nSessionID);
        pSession->SetMsgQueue(m_pMsgQueue);
        m_mapSession[nSessionID] = pSession;
    }
    return pSession;
}

CEventSession *CRaftIoBase::CreateServiceSession(struct bufferevent *pBufferEvent)
{
    CRaftSession *pSession = NULL;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    uint32_t nSessionID;
    if (m_pSessionSeqID->AllocSeqID(nSessionID))
    {
        if (m_mapSession.find(nSessionID) == m_mapSession.end())
        {
            pSession = new CRaftSession(this, pBufferEvent, nSessionID);
            pSession->SetMsgQueue(m_pMsgQueue);
            m_mapSession[nSessionID] = pSession;
        }
    }
    return pSession;
}

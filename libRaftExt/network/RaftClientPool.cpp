#include "stdafx.h"
#include "raft.pb.h"
#include "rpc.pb.h"
using namespace raftserverpb;

#include <signal.h>
#include <assert.h>
#include "RaftClientPool.h"
#include "RaftAdminClient.h"
#include "RaftQueue.h"
#include "RaftClientSession.h"
#include "Log4CxxLogger.h"


#include "RaftDef.h"

CRaftClientPool::CRaftClientPool()
{
    m_pSignalEvent = NULL;
    m_pTimerEvent = NULL;
    m_nTryTicks = 10;
    m_nActiveNode = 0;
    m_nSubSessionSeq = 0;
    m_pMsgQueue = NULL;
}

CRaftClientPool::~CRaftClientPool()
{
}

void CRaftClientPool::SetLogger(CLogger *pLogger)
{
    m_pLogger = pLogger;
}

bool CRaftClientPool::Init(void)
{
    bool bInit = false;
    if (CRaftIoBase::Init())
    {
        bInit = (InitSignal() && InitTimer() && InitSubSession());
        if (!bInit)
            Uninit();
    }
    return bInit;
}

void CRaftClientPool::Uninit(void)
{
    if (NULL != m_pSignalEvent && NULL != GetBase())
    {
        event_del(m_pSignalEvent);
        event_free(m_pSignalEvent);
        m_pSignalEvent = NULL;
    }
    if (NULL != m_pTimerEvent && NULL != GetBase())
    {
        event_del(m_pTimerEvent);
        event_free(m_pTimerEvent);
        m_pTimerEvent = NULL;
    }
    m_semSubSessions.Destroy();
    for (auto iter = m_listSem.begin(); iter != m_listSem.end();iter ++)
    {
        CRaftSemaphore *pSem = *iter;
        delete pSem;
    }
    m_listSem.clear();
    CRaftIoBase::Uninit();
}

bool CRaftClientPool::InitSubSession(void)
{
    bool bInit = false;
    m_nMaxSubSessions = 256;
    if (m_semSubSessions.Create(m_nMaxSubSessions))
    {
        for (int nIndex = 0; nIndex < m_nMaxSubSessions; nIndex ++)
        {
            CRaftSemaphore *pSem = new CRaftSemaphore();
            pSem->Create(0);
            m_listSem.push_back(pSem);
        }
        bInit = true;
    }
    return bInit;
}

bool CRaftClientPool::InitSignal(void)
{
    bool bInit = false;
    struct event *pSignalEvent = evsignal_new(GetBase(), SIGINT, SignalShell, this);
    if (NULL != pSignalEvent)
    {
        if (0 == event_add(pSignalEvent, NULL))
        {
            m_pSignalEvent = pSignalEvent;
            bInit = true;
        }
        else
            event_free(pSignalEvent);
    }
    return bInit;
}

bool CRaftClientPool::InitTimer(void)
{
    bool bInit = false;

    event *pEvent = evtimer_new(GetBase(), TimerShell, this);
    if (NULL != pEvent)
    {
        struct timeval tv;
        tv.tv_sec = m_cfgSelf.m_nMsPerTick / 1000;
        tv.tv_usec = (m_cfgSelf.m_nMsPerTick % 1000) * 1000;
        if (0 == evtimer_add(pEvent, &tv))
        {
            m_pTimerEvent = pEvent;
            bInit = true;
        }
        else
            event_free(pEvent);
    }
    return bInit;
}

int CRaftClientPool::Start(void)
{
    int nStart = CRaftIoBase::Start();
    if (0 == nStart)
    {
        for (size_t nNode = 0; nNode < m_cfgSelf.m_aNodes.size(); nNode++)
        {
            const CRaftInfo &nodeInfo = m_cfgSelf.m_aNodes[nNode];
            CRaftClientSession * pSession = (CRaftClientSession*)Connect(nodeInfo.m_strHost, nodeInfo.m_nPort, nodeInfo.m_nNodeId);
            if (NULL != pSession)
            {
                std::lock_guard<std::mutex> guardSession(m_mutexSession);
                pSession->SetPool(this);
                m_mapNodes[nodeInfo.m_nNodeId] = pSession;
            }
            else
            {
                nStart = 2;
                break;
            }
        }
    }
    else
    {
        CRaftIoBase::NotifyStop();
        CRaftIoBase::Stop();
        nStart = 1;
    }
    return nStart;
}

void CRaftClientPool::NotifyStop(void)
{
    CRaftIoBase::NotifyStop();
    if(NULL != m_pMsgQueue)
        m_pMsgQueue->Push(NULL);
    //唤醒所有的客户端
    std::lock_guard<std::mutex> guardSession(m_mutexSubSession);
    for (auto iterSession: m_mapSubSession)
    {
        CRaftSemaphore *pSemWake = iterSession.second;
        pSemWake->Post(1);
    }
}

void CRaftClientPool::Stop(void)
{
    CRaftIoBase::Stop();
}

void CRaftClientPool::TimerShell(evutil_socket_t tTimer, short nEvents, void *pData)
{
    CRaftClientPool *pClient = (CRaftClientPool *)pData;
    pClient->TimerFunc(tTimer, nEvents);
}

void CRaftClientPool::TimerFunc(evutil_socket_t tSignal, short nEvents)
{
    m_nTryTicks--;
    if (0 == m_nTryTicks)
    {
        m_nTryTicks = 10;
        std::lock_guard<std::mutex> guardSession(m_mutexSession);
        for (auto iter = m_mapNodes.begin(); iter != m_mapNodes.end(); ++iter)
        {
            CRaftSession *pSession = (CRaftSession*)iter->second;
            if (!pSession->IsConnected())
            {
                int nConnect = pSession->RetryConnect();
                m_pLogger->Infof(__FILE__, __LINE__, "Reconnect %d %d", iter->first, nConnect);
            }
        }
    }

    struct timeval tv;
    tv.tv_sec = m_cfgSelf.m_nMsPerTick / 1000;
    tv.tv_usec = (m_cfgSelf.m_nMsPerTick % 1000) * 1000;
    evtimer_add(m_pTimerEvent, &tv);
}

void CRaftClientPool::SignalShell(evutil_socket_t tSignal, short nEvents, void *pData)
{
    CRaftClientPool *pServer = (CRaftClientPool *)pData;
    pServer->SignalFunc(tSignal, nEvents);
}

void CRaftClientPool::SignalFunc(evutil_socket_t tSignal, short nEvents)
{
    struct event_base *pBase = GetBase();
    NotifyStop();
    Stop();
}

uint32_t CRaftClientPool::RegClient(CRaftClient *pClient)
{
    uint32_t nClientID = 0;
    if (WAIT_OBJECT_0 == m_semSubSessions.Wait(1000 * 60))
    {
        std::lock_guard<std::mutex> guardSession(m_mutexSubSession);
        if (m_nSubSessionSeq < 0x7FFFFFFF)
            m_nSubSessionSeq++;
        else
            m_nSubSessionSeq = 1;
        CRaftSemaphore *pSem = m_listSem.back();
        m_listSem.pop_back();
        nClientID = m_nSubSessionSeq;
        m_mapSubSession[nClientID] = pSem;
    }
    return nClientID;
}

bool CRaftClientPool::UnRegClient(CRaftClient *pClient)
{
    bool bUnReg = false;
    std::lock_guard<std::mutex> guardSession(m_mutexSubSession);
    uint32_t nClientSeq = pClient->GetSubSessionID();
    auto iterClient = m_mapSubSession.find(nClientSeq);
    if (iterClient != m_mapSubSession.end())
    {
        m_mapSubSession.erase(iterClient);
        m_listSem.push_front(iterClient->second);
        bUnReg = true;

        auto iterResponse = m_mapResponses.find(nClientSeq);
        if (iterResponse != m_mapResponses.end())
        {
            delete iterResponse->second;
            m_mapResponses.erase(iterResponse);
        }
    }
    return bUnReg;
}

CEventSession *CRaftClientPool::GetActiveSession(void)
{
    CEventSession *pSession = NULL;
    int nActiveNode = m_nActiveNode;
    auto iterNode = m_mapSession.begin();
    for (int nTry = 0; nTry < m_nActiveNode; nTry++)
        iterNode++;
    size_t nMaxTry = m_cfgSelf.m_aNodes.size();
    for (size_t nTry = 0; nTry < nMaxTry && iterNode != m_mapSession.end(); nTry++)
    {
        pSession = iterNode->second;
        if (CEventSession::eConnected != pSession->GetState())
        {
            pSession = NULL;
            iterNode++;
            if (iterNode == m_mapSession.end())
                iterNode = m_mapSession.begin();
            m_nActiveNode++;
            if (m_nActiveNode == m_cfgSelf.m_aNodes.size())
                m_nActiveNode = 0;
        }
        else
            break;
    }
    return pSession;
}

raftserverpb::ResponseOp * CRaftClientPool::SendReq(raftserverpb::RequestOp *pRequest, uint32_t dwTimeoutMs)
{
    raftserverpb::ResponseOp *pResponse = NULL;
    CRaftSession *pSession = (CRaftSession*)GetActiveSession();
    if (NULL != pSession)
    {
        CRaftSemaphore *pSemWake = NULL;
        uint32_t nSubSessionID = pRequest->subsessionid();
        auto iterSession = m_mapSubSession.find(nSubSessionID);
        if (iterSession != m_mapSubSession.end())
            pSemWake = iterSession->second;
        else
        {
            pSemWake = m_listSem.back();
            m_listSem.pop_back();
            m_mapSubSession[nSubSessionID] = pSemWake;
        }

        std::string strRequest = pRequest->SerializeAsString();
        int nSend = pSession->SendMsg(strRequest);
        if (0 == nSend)
        {
            uint32_t nWait = pSemWake->Wait(dwTimeoutMs);
            if (nWait == WAIT_OBJECT_0)
            {
                auto iter = m_mapResponses.find(nSubSessionID);
                if (iter != m_mapResponses.end())
                {
                    pResponse = iter->second;
                    m_mapResponses.erase(iter);
                }
            }
        }
    }
    return pResponse;
}

bool CRaftClientPool::Wake(raftserverpb::ResponseOp *pResponse)
{
    bool bWake = false;
    uint32_t nSessionID = pResponse->subsessionid();
    std::lock_guard<std::mutex> guardSession(m_mutexSubSession);
    auto iterSession = m_mapSubSession.find(nSessionID);
    if (iterSession  != m_mapSubSession.end())
    {
        CRaftSemaphore *pSemWake = iterSession->second;
        assert(m_mapResponses.find(nSessionID) == m_mapResponses.end());
        m_mapResponses[nSessionID] = pResponse;
        pSemWake->Post(1);
        bWake = true;
    }
    return bWake;
}

CEventSession *CRaftClientPool::CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID)
{
    CRaftClientSession *pSession = new CRaftClientSession(this, pBufferEvent, strHost, nPort);
    pSession->SetPool(this);
    pSession->SetSessionID(nSessionID);
    pSession->SetMsgQueue(m_pMsgQueue);
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    m_mapSession[nSessionID] = pSession;
    return pSession;
}

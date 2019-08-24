#include "stdafx.h"
#include <signal.h>
#include "RaftServer.h"
#include <event2/event.h>
#include <event2/thread.h>
#include "RaftFrame.h"
#include "RaftSession.h"
#include "RaftIoBase.h"
#include "Raft.h"
#include "RaftUtil.h"
#include "RaftQueue.h"
#include "protobuffer/RaftProtoBufferSerializer.h"
#include "Log4CxxLogger.h"
#include "KvService.h"
#include "RequestOp.h"

CRaftServer::CRaftServer()
{
    m_pSignalEvent = NULL;
    m_pTimerEvent = NULL;
    m_nTryTicks = 10;
    m_pRaftFrame = NULL;
}

CRaftServer::~CRaftServer()
{
}

bool CRaftServer::Init(CRaftFrame *pRaftFrame)
{
    bool bInit = false;
    CRaftConfig * pConfig = pRaftFrame->GetRaftConfig();
    const CRaftInfo &infoRaft = pConfig->GetSelf();
    AddListenCfg(infoRaft.m_strHost, infoRaft.m_nPort);
    AddListenCfg(infoRaft.m_strHost, infoRaft.m_nPeerPort);
    if (CListenEventBase::Init())
    {
        bInit = (InitSignal() && InitTimer());
        if (bInit)
            m_pRaftFrame = pRaftFrame;
        else
            Uninit();
    }
    return bInit;
}

void CRaftServer::Uninit(void)
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
    CListenEventBase::Uninit();
}

bool CRaftServer::InitSignal(void)
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

bool CRaftServer::InitTimer(void)
{
    bool bInit = false;
    
    event *pEvent = evtimer_new(GetBase(), TimerShell,this);
    if (NULL != pEvent)
    {
        struct timeval tv;
        CRaftConfig * pConfig = m_pRaftFrame->GetRaftConfig();
        tv.tv_sec = pConfig->m_nMsPerTick /1000;
        tv.tv_usec = (pConfig->m_nMsPerTick % 1000) * 1000;
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

int CRaftServer::Start(void)
{
    int nStart = CListenEventBase::Start();
    if (0 == nStart)
    {
        if (NULL == m_pThreadMsg)
            m_pThreadMsg = new std::thread(SendMsgShell, this);
        if (NULL == m_pThreadIo)
            m_pThreadIo = new std::thread(KvIoShell, this);
        if (NULL != m_pThreadMsg && NULL != m_pThreadIo)
        {
            nStart = 0;
            CRaftConfig * pConfig = m_pRaftFrame->GetRaftConfig();
            for (size_t nNode = 0; nNode < pConfig->m_aNodes.size(); nNode++)
            {
                const CRaftInfo &nodeInfo = pConfig->m_aNodes[nNode];
                if (pConfig->m_nRaftID != nodeInfo.m_nNodeId)
                {
                    CEventSession * pSession = m_pIoBase->Connect(nodeInfo.m_strHost, nodeInfo.m_nPeerPort,nodeInfo.m_nNodeId);
                    if (NULL != pSession)
                    {
                        std::lock_guard<std::mutex> guardSession(m_mutexSession);
                        m_mapNodes[nodeInfo.m_nNodeId] = pSession;
                    }
                }
            }
        }
        else
        {
            CListenEventBase::NotifyStop();
            CListenEventBase::Stop();
            nStart = 1;
        }
    }
    return nStart;
}

void CRaftServer::NotifyStop(void)
{
    CListenEventBase::NotifyStop();
    m_pRaftFrame->GetMsgQueue()->Push(NULL);
    m_pRaftFrame->GetIoQueue()->Push(NULL);
}

void CRaftServer::Stop(void)
{
    if (NULL != m_pThreadIo)
    {
        m_pThreadIo->join();
        delete m_pThreadIo;
        m_pThreadIo = NULL;
    }
    if (NULL != m_pThreadMsg)
    {
        m_pThreadMsg->join();
        delete m_pThreadMsg;
        m_pThreadMsg = NULL;
    }
    CListenEventBase::Stop();
}

CIoEventBase* CRaftServer::CreateIoBase(void)
{
    CRaftIoBase *pIoBase = new CRaftIoBase(&m_seqIDs);
    pIoBase->SetMsgQueue(m_pRaftFrame->GetMsgQueue());
    pIoBase->SetSerializer(m_pRaftFrame->GetSerializer());
    return pIoBase;
}

void CRaftServer::DestroyIoBase(CIoEventBase* pIoBase)
{
    CListenEventBase::DestroyIoBase(pIoBase);
}

void CRaftServer::TimerShell(evutil_socket_t tTimer, short nEvents, void *pData)
{
    CRaftServer *pServer = (CRaftServer *)pData;
    pServer->TimerFunc(tTimer, nEvents);
}

void CRaftServer::TimerFunc(evutil_socket_t tSignal, short nEvents)
{
    CRaftConfig *pConfig = m_pRaftFrame->GetRaftConfig();
    CRaft * pRaft = m_pRaftFrame->GetRaft();
    if (NULL != pRaft)
        pRaft->OnTick();
    m_nTryTicks --;
    if (0 == m_nTryTicks)
    {
        m_nTryTicks = 10;
        std::lock_guard<std::mutex> guardSession(m_mutexSession);
        for (auto iter = m_mapNodes.begin(); iter != m_mapNodes.end(); ++iter)
        {
            CRaftSession *pSession = (CRaftSession*)iter->second;
            if (pConfig->m_nRaftID != iter->first)
            {
                if (!pSession->IsConnected())
                {
                    int nConnect = pSession->RetryConnect();
                    m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "Reconnect %d %d", iter->first, nConnect);
                }
            }
        }
    }

    struct timeval tv;
    tv.tv_sec = pConfig->m_nMsPerTick / 1000;
    tv.tv_usec = (pConfig->m_nMsPerTick % 1000) * 1000;
    evtimer_add(m_pTimerEvent, &tv);
}

void CRaftServer::SignalShell(evutil_socket_t tSignal, short nEvents, void *pData)
{
    CRaftServer *pServer = (CRaftServer *)pData;
    pServer->SignalFunc(tSignal, nEvents);
}

void CRaftServer::SignalFunc(evutil_socket_t tSignal, short nEvents)
{
    struct event_base *pBase = GetBase();
    NotifyStop();
    event_base_loopbreak(pBase);
}

uint32_t CRaftServer::CreateSessionID(void)
{
    return 0;
}

void CRaftServer::ListenEventFunc(struct evconnlistener *pListener, evutil_socket_t fdSocket,
    struct sockaddr *pSockAddr, int nSockLen)
{
    CRaftSession *pSession = static_cast<CRaftSession *>(CreateEvent(m_pIoBase, pListener, fdSocket, pSockAddr, nSockLen));
    if (NULL != pSession)
    {
        std::lock_guard<std::mutex> guardSession(m_mutexSession);

        /*m_mapNodes[m_nCurClientID] = pSession;*/
//        pSession->SetNodeID(m_nCurClientID);
        if (pListener == m_apListener[0])
            pSession->SetTypeID(1); //表明是从客户端来的Session

        if (pSockAddr->sa_family == AF_INET)
        {
            struct sockaddr_in *pSockAddrV4 = (struct sockaddr_in*)pSockAddr;
            int nPort = ntohs(pSockAddrV4->sin_port);
            struct in_addr in = pSockAddrV4->sin_addr;
            char strIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &in, strIP, sizeof(strIP));
            printf("Session Info: IP\t%s\tPort\t%d\tType\t%d\n", strIP, nPort, pSession->GetTypeID());
        }
        else if (pSockAddr->sa_family == AF_INET6)
        {

        }
    }
    else
    {
        //出现异常，在合适的时机，由主线程停止子线程
    }
}

void CRaftServer::KvIoShell(void *pContext)
{
    CRaftServer *pRaftServer = (CRaftServer*)pContext;
    pRaftServer->KvIoFunc();
}

void CRaftServer::KvIoFunc(void)
{
    CRaftQueue * pIoQueue = m_pRaftFrame->GetIoQueue();
    if (NULL != pIoQueue)
    {
        int nErrorNo = CRaftErrNo::eOK;
        CLogOperation *pLogOperation = static_cast<CLogOperation*> (pIoQueue->Pop());
        while ((NULL != pLogOperation) && (2 == m_nIoState) && CRaftErrNo::Success(nErrorNo))
        {
            nErrorNo = DoLogopt(pLogOperation);
            delete pLogOperation;
            if (CRaftErrNo::Success(nErrorNo))
                pLogOperation = static_cast<CLogOperation*> (pIoQueue->Pop());
        }
        DiscardIoTask();
    }
}

int CRaftServer::DoLogopt(CLogOperation * pLogOperation)
{
    int nErrorNo = CRaftErrNo::eOK;
    CRaftProtoBufferSerializer *pSerializer = dynamic_cast<CRaftProtoBufferSerializer *>(m_pRaftFrame->GetSerializer());
    CRaftLog * pRaftLog = m_pRaftFrame->GetRaftLog();
    CKvService *pKvService = m_pRaftFrame->GetKvService();
    if ((0 == pLogOperation->m_nType || 1 == pLogOperation->m_nType) 
        && pLogOperation->m_pOperation != NULL)
    {
        CReadState *pIoReady = pLogOperation->m_pOperation;
        CRequestOp *pRequest = new CRequestOp();
        pSerializer->ParseRequestOp(*pRequest, pIoReady->m_strRequestCtx);
        uint32_t nClientID = pRequest->clientid();
        uint32_t nSubSessionID = pRequest->subsessionid();
        CRequestOp::RequestCase typeRequest = pRequest->request_case();

        CResponseOp *pResponse = new CResponseOp();
        pResponse->set_subsessionid(nSubSessionID);
        if (typeRequest == CRequestOp::kRequestRange)
        {
            const CRangeRequest &rangeRequest = pRequest->request_range();
            const string strKey = rangeRequest.key();
            string strValue;
            int nGet = pKvService->Get(strKey, strValue);
            CRangeResponse* pRangeResonse = pResponse->mutable_response_range();
            pResponse->set_errorno(nGet);
            if (0 == nGet)
            {
                CKeyValue* pKeyValue = pRangeResonse->add_kvs();
                pKeyValue->set_key(strKey);
                pKeyValue->set_value(strValue);
            }
        }
        else if (typeRequest == CRequestOp::kRequestPut)
        {
            const CPutRequest &requestPut = pRequest->request_put();
            int nPut = pKvService->Put(requestPut.key(), requestPut.value());
            if (0 == nPut)
            {
                pRaftLog->AppliedTo(pIoReady->m_u64Index);
                m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "put opt apply log to %llu node", pIoReady->m_u64Index);
            }
            pResponse->set_errorno(nPut);
            CPutResponse *pPutResponse = pResponse->mutable_response_put();
        }
        else if (typeRequest == CRequestOp::kRequestDeleteRange)
        {
            const CDeleteRangeRequest &requestDel = pRequest->request_delete_range();
            int nDelete = pKvService->Delete(requestDel.key());
            if (0 == nDelete)
            {
                m_pRaftFrame->GetRaftLog()->AppliedTo(pIoReady->m_u64Index);
                m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "delete opt apply log to %llu node", pIoReady->m_u64Index);
            }
            pResponse->set_errorno(nDelete);
            CDeleteRangeResponse* pDelResponse = pResponse->mutable_response_delete_range();
        }
        if (NULL != pResponse)
        {
            std::string strMsg;
            CRaftProtoBufferSerializer *pSerializer = dynamic_cast<CRaftProtoBufferSerializer*>(m_pRaftFrame->GetSerializer());
            pSerializer->SerializeResponseOp(*pResponse, strMsg);
            bool bSent = TrySendRaftMsg(nClientID, strMsg);
            if (bSent)
                m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "send io message to %d node", nClientID);
            else
                m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "discard message when not found node");
            delete pResponse;
        }
    }
    else if (2 == pLogOperation->m_nType && pLogOperation->m_u64ApplyTo > 0)
    {
        uint64_t u64MaxSize = 1024 * 1024;
        EntryVec entries;
        uint64_t u64Apply = pRaftLog->GetApplied() + 1;
        for (; u64Apply <= pLogOperation->m_u64ApplyTo; u64Apply++)
        {
            nErrorNo = pRaftLog->GetSliceEntries(u64Apply, u64Apply + 1, u64MaxSize, entries);
            if (CRaftErrNo::Success(nErrorNo))
            {
                const std::string &strRequestCtx = entries[0].data();
                if (!strRequestCtx.empty())
                {
                    CRequestOp *pRequest = new CRequestOp();
                    pSerializer->ParseRequestOp(*pRequest, strRequestCtx);
                    CRequestOp::RequestCase typeRequest = pRequest->request_case();
                    if (typeRequest == CRequestOp::kRequestPut)
                    {
                        const CPutRequest &requestPut = pRequest->request_put();
                        int nPut = pKvService->Put(requestPut.key(), requestPut.value());
                    }
                    else if (typeRequest == CRequestOp::kRequestDeleteRange)
                    {
                        const CDeleteRangeRequest &requestDel = pRequest->request_delete_range();
                        int nDelete = pKvService->Delete(requestDel.key());
                    }
                    delete pRequest;
                }
            }
            else
                break;
        }
        if (CRaftErrNo::Success(nErrorNo))
        {
            pRaftLog->AppliedTo(pLogOperation->m_u64ApplyTo);
            m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "apply log to %llu node", pLogOperation->m_u64ApplyTo);
        }
    }
    return nErrorNo;
}

void CRaftServer::SendMsgShell(void *pContext)
{
    CRaftServer *pRaftServer = (CRaftServer*)pContext;
    pRaftServer->SendMsgFunc();
}

void CRaftServer::SendMsgFunc(void)
{
    CRaftQueue * pMsgQueue = m_pRaftFrame->GetMsgQueue();
    CRaftConfig *pConfig = m_pRaftFrame->GetRaftConfig();
    CRaft *pRaft = m_pRaftFrame->GetRaft();
    if (NULL != pMsgQueue)
    {
        CMessage *pMsg = static_cast<CMessage*> (pMsgQueue->Pop());
        while ((NULL != pMsg) && (2 == m_nIoState))
        {
            uint32_t uRaftID = pMsg->to();
            if (pConfig->m_nRaftID == uRaftID || 0 == uRaftID)
                pRaft->Step(*pMsg);
            else
            {
                std::string strMsg;
                m_pRaftFrame->GetSerializer()->SerializeMessage(*pMsg, strMsg);
                bool bSent = TrySendRaftMsg(uRaftID, strMsg);
                if (bSent)
                    m_pRaftFrame->GetLogger()->Debugf(__FILE__, __LINE__, "send message :From %d  to %d type %s",
                        pMsg->from(), pMsg->to(), CRaftUtil::MsgType2String(pMsg->type()));
                else
                    m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "discard message when not found node");
            }
            delete pMsg;
            pMsg = static_cast<CMessage *>(pMsgQueue->Pop());
        }
        DiscardMessages();
    }
}

bool CRaftServer::TrySendRaftMsg(uint32_t uRaftID, std::string &strMsg)
{
    bool bSent = false;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    if (m_mapNodes.find(uRaftID) != m_mapNodes.end())
    {
        CRaftSession *pSession = (CRaftSession*)m_mapNodes[uRaftID];
        if (pSession->IsConnected())
        {
            int nSend = pSession->SendMsg(strMsg);
            bSent = true;
        }
    }
    else
    {
        CRaftIoBase *pIoBase = dynamic_cast<CRaftIoBase *>(m_pIoBase);
        int nSend = pIoBase->TrySendRaftMsg(uRaftID, strMsg);
        bSent = (0 == nSend);
    }
    return bSent;
}

void CRaftServer::DiscardIoTask(void)
{
    int nMsgCount = 0;
    CRaftQueue * pIoQueue = m_pRaftFrame->GetIoQueue();
    CReadState* pMsg = static_cast<CReadState*> (pIoQueue->Pop(0));
    while (NULL != pMsg)
    {
        nMsgCount++;
        delete pMsg;
        pMsg = static_cast<CReadState*> (pIoQueue->Pop(0));
    }
    m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "discard %d io task when stopping", nMsgCount);
}

void CRaftServer::DiscardMessages(void)
{
    CRaftQueue *pMsgQueue = m_pRaftFrame->GetMsgQueue();
    int nMsgCount = 0;
    CMessage *pMsg = static_cast<CMessage *>(pMsgQueue->Pop(0));
    while (NULL != pMsg)
    {
        nMsgCount++;
        delete pMsg;
        pMsg = static_cast<CMessage *>(pMsgQueue->Pop(0));
    }
    m_pRaftFrame->GetLogger()->Infof(__FILE__, __LINE__, "discard %d messages when stopping", nMsgCount);               
}

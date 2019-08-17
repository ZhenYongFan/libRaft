#include "stdafx.h"
#include <signal.h>
#include "RaftServer.h"
#include <event2/event.h>
#include <event2/thread.h>

#include "RaftSession.h"
#include "RaftIoBase.h"
#include "Raft.h"
#include "RaftUtil.h"
#include "RaftQueue.h"
#include "Log4CxxLogger.h"
#include "KvService.h"

#include "rpc.pb.h"
using namespace raftserverpb;

CRaftServer::CRaftServer()
{
    m_pSignalEvent = NULL;
    m_pTimerEvent = NULL;
    m_pKvService = NULL;
    m_nTryTicks = 10;
}

CRaftServer::~CRaftServer()
{
}

void CRaftServer::SetRaftNode(CRaft *pRaftNode)
{
    m_pRaftNode = pRaftNode;
}

void CRaftServer::SetMsgQueue(CRaftQueue *pQueue)
{
    m_pMsgQueue = pQueue;
}

void CRaftServer::SetIoQueue(CRaftQueue *pQueue)
{
    m_pIoQueue = pQueue;
}

void CRaftServer::SetLogger(CLogger *pLogger)
{
    m_pLogger = pLogger;
}

void CRaftServer::SetKvService(CKvService *pKvService)
{
    m_pKvService = pKvService;
}

void CRaftServer::SetRaftLog(CRaftLog *pRaftLog)
{
    m_pRaftLog = pRaftLog;
}

bool CRaftServer::Init(void)
{
    bool bInit = false;
    const CRaftInfo &infoRaft = m_cfgSelf.GetSelf();
    AddListenCfg(infoRaft.m_strHost, infoRaft.m_nPort);
    AddListenCfg(infoRaft.m_strHost, infoRaft.m_nPeerPort);
    if (CListenEventBase::Init())
    {
        bInit = (InitSignal() && InitTimer());
        if (!bInit)
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
        tv.tv_sec = m_cfgSelf.m_nMsPerTick /1000;
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
            for (size_t nNode = 0; nNode < m_cfgSelf.m_aNodes.size(); nNode++)
            {
                const CRaftInfo &nodeInfo = m_cfgSelf.m_aNodes[nNode];
                if (m_cfgSelf.m_nRaftID != nodeInfo.m_nNodeId)
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
    m_pMsgQueue->Push(NULL);
    m_pIoQueue->Push(NULL);
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
    CRaftIoBase *pIoBase = new CRaftIoBase();
    pIoBase->SetSessionRange(1000, 11000);
    pIoBase->SetMsgQueue(m_pMsgQueue);
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
    if (NULL != m_pRaftNode)
        m_pRaftNode->OnTick();
    m_nTryTicks --;
    if (0 == m_nTryTicks)
    {
        m_nTryTicks = 10;
        std::lock_guard<std::mutex> guardSession(m_mutexSession);
        for (auto iter = m_mapNodes.begin(); iter != m_mapNodes.end(); ++iter)
        {
            CRaftSession *pSession = (CRaftSession*)iter->second;
            if (m_cfgSelf.m_nRaftID != iter->first)
            {
                if (!pSession->IsConnected())
                {
                    int nConnect = pSession->RetryConnect();
                    m_pLogger->Infof(__FILE__, __LINE__, "Reconnect %d %d", iter->first, nConnect);
                }
            }
        }
    }

    struct timeval tv;
    tv.tv_sec = m_cfgSelf.m_nMsPerTick / 1000;
    tv.tv_usec = (m_cfgSelf.m_nMsPerTick % 1000) * 1000;
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
    if (NULL != m_pIoQueue)
    {
        int nErrorNo = OK;
        CLogOperation *pLogOperation = static_cast<CLogOperation*> (m_pIoQueue->Pop());
        while ((NULL != pLogOperation) && (2 == m_nIoState) && SUCCESS(nErrorNo))
        {
            nErrorNo = DoLogopt(pLogOperation);
            delete pLogOperation;
            if (SUCCESS(nErrorNo))
                pLogOperation = static_cast<CLogOperation*> (m_pIoQueue->Pop());
        }
        DiscardIoTask();
    }
}

int CRaftServer::DoLogopt(CLogOperation * pLogOperation)
{
    int nErrorNo = OK;
    if ((0 == pLogOperation->m_nType || 1 == pLogOperation->m_nType) && pLogOperation->m_pOperation != NULL)
    {
        CReadState *pIoReady = pLogOperation->m_pOperation;
        RequestOp *pRequest = new RequestOp();
        pRequest->ParseFromString(pIoReady->m_strRequestCtx);
        uint32_t nClientID = pRequest->clientid();
        uint32_t nSubSessionID = pRequest->subsessionid();
        RequestOp::RequestCase typeRequest = pRequest->request_case();

        ResponseOp *pResponse = new ResponseOp();
        pResponse->set_subsessionid(nSubSessionID);
        if (typeRequest == RequestOp::kRequestRange)
        {
            RangeRequest * pRangeRequest = pRequest->mutable_request_range();
            string strKey = pRangeRequest->key();
            string strValue;
            int nGet = m_pKvService->Get(strKey, strValue);
            RangeResponse* pRangeResonse = pResponse->mutable_response_range();
            pResponse->set_errorno(nGet);
            if (0 == nGet)
            {
                raftserverpb::KeyValue* pKeyValue = pRangeResonse->add_kvs();
                pKeyValue->set_key(strKey);
                pKeyValue->set_value(strValue);
            }
        }
        else if (typeRequest == RequestOp::kRequestPut)
        {
            const PutRequest &requestPut = pRequest->request_put();
            int nPut = m_pKvService->Put(requestPut.key(), requestPut.value());
            if (0 == nPut)
            {
                m_pRaftLog->AppliedTo(pIoReady->m_u64Index);
                m_pLogger->Infof(__FILE__, __LINE__, "put opt apply log to %llu node", pIoReady->m_u64Index);
            }
            pResponse->set_errorno(nPut);
            PutResponse *pPutResponse = pResponse->mutable_response_put();
        }
        else if (typeRequest == RequestOp::kRequestDeleteRange)
        {
            const DeleteRangeRequest &requestDel = pRequest->request_delete_range();
            int nDelete = m_pKvService->Delete(requestDel.key());
            if (0 == nDelete)
            {
                m_pRaftLog->AppliedTo(pIoReady->m_u64Index);
                m_pLogger->Infof(__FILE__, __LINE__, "delete opt apply log to %llu node", pIoReady->m_u64Index);
            }
            pResponse->set_errorno(nDelete);
            DeleteRangeResponse* pDelResponse = pResponse->mutable_response_delete_range();
        }
        if (NULL != pResponse)
        {
            bool bSent = TrySendRaftMsg(nClientID, pResponse);
            if (bSent)
                m_pLogger->Infof(__FILE__, __LINE__, "send io message to %d node", nClientID);
            else
                m_pLogger->Infof(__FILE__, __LINE__, "discard message when not found node");
            delete pResponse;
        }
    }
    else if (2 == pLogOperation->m_nType && pLogOperation->m_u64ApplyTo > 0)
    {
        uint64_t u64MaxSize = 1024 * 1024;
        EntryVec entries;
        uint64_t u64Apply = m_pRaftLog->GetApplied() + 1;
        for (; u64Apply <= pLogOperation->m_u64ApplyTo; u64Apply++)
        {
            nErrorNo = m_pRaftLog->GetSliceEntries(u64Apply, u64Apply + 1, u64MaxSize, entries);
            if (SUCCESS(nErrorNo))
            {
                const std::string &strRequestCtx = entries[0].data();
                if (!strRequestCtx.empty())
                {
                    RequestOp *pRequest = new RequestOp();
                    pRequest->ParseFromString(strRequestCtx);
                    RequestOp::RequestCase typeRequest = pRequest->request_case();
                    if (typeRequest == RequestOp::kRequestPut)
                    {
                        const PutRequest &requestPut = pRequest->request_put();
                        int nPut = m_pKvService->Put(requestPut.key(), requestPut.value());
                    }
                    else if (typeRequest == RequestOp::kRequestDeleteRange)
                    {
                        const DeleteRangeRequest &requestDel = pRequest->request_delete_range();
                        int nDelete = m_pKvService->Delete(requestDel.key());
                    }
                    delete pRequest;
                }
            }
            else
                break;
        }
        if (SUCCESS(nErrorNo))
        {
            m_pRaftLog->AppliedTo(pLogOperation->m_u64ApplyTo);
            m_pLogger->Infof(__FILE__, __LINE__, "apply log to %llu node", pLogOperation->m_u64ApplyTo);
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
    if (NULL != m_pMsgQueue)
    {
        Message *pMsg = static_cast<Message*> (m_pMsgQueue->Pop());
        while ((NULL != pMsg) && (2 == m_nIoState))
        {
            uint32_t uRaftID = pMsg->to();
            if (m_cfgSelf.m_nRaftID == uRaftID || 0 == uRaftID)
                m_pRaftNode->Step(*pMsg);
            else
            {
                bool bSent = TrySendRaftMsg(uRaftID,pMsg);
                if (bSent)
                    m_pLogger->Debugf(__FILE__, __LINE__, "send message :From %d  to %d type %s",
                        pMsg->from(), pMsg->to(), CRaftUtil::MsgType2String(pMsg->type()));
                else
                    m_pLogger->Infof(__FILE__, __LINE__, "discard message when not found node");
            }
            delete pMsg;
            pMsg = static_cast<Message *>(m_pMsgQueue->Pop());
        }
        DiscardMessages();
    }
}

bool CRaftServer::TrySendRaftMsg(uint32_t uRaftID, google::protobuf::MessageLite * pMsg)
{
    bool bSent = false;
    std::lock_guard<std::mutex> guardSession(m_mutexSession);
    if (m_mapNodes.find(uRaftID) != m_mapNodes.end())
    {
        CRaftSession *pSession = (CRaftSession*)m_mapNodes[uRaftID];
        if (pSession->IsConnected())
        {
            std::string strMsg = pMsg->SerializeAsString();
            int nSend = pSession->SendMsg(strMsg);
            bSent = true;
        }
    }
    else
    {
        CRaftIoBase *pIoBase = dynamic_cast<CRaftIoBase *>(m_pIoBase);
        bSent = pIoBase->TrySendRaftMsg(uRaftID, pMsg);
    }
    return bSent;
}

void CRaftServer::DiscardIoTask(void)
{
    int nMsgCount = 0;
    CReadState* pMsg = static_cast<CReadState*> (m_pIoQueue->Pop(0));
    while (NULL != pMsg)
    {
        nMsgCount++;
        delete pMsg;
        pMsg = static_cast<CReadState*> (m_pIoQueue->Pop(0));
    }
    m_pLogger->Infof(__FILE__, __LINE__, "discard %d io task when stopping", nMsgCount);
}

void CRaftServer::DiscardMessages(void)
{
    int nMsgCount = 0;
    google::protobuf::MessageLite *pMsg = static_cast<google::protobuf::MessageLite *>(m_pMsgQueue->Pop(0));
    while (NULL != pMsg)
    {
        nMsgCount++;
        delete pMsg;
        pMsg = static_cast<google::protobuf::MessageLite *>(m_pMsgQueue->Pop(0));
    }
    m_pLogger->Infof(__FILE__, __LINE__, "discard %d messages when stopping", nMsgCount);               
}

#include "stdafx.h"
#include "raft.pb.h"
using namespace raftpb;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TestRaftFrame.h"
#include <RaftUtil.h>
#include "NullLogger.h"
extern CNullLogger kDefaultLogger;

CRaftConfig* newTestConfig(uint32_t id, const vector<uint32_t>& peers, int election, int hb)
{
    CRaftConfig *pConfig = new CRaftConfig();
    pConfig->m_nRaftID = int(id);
    pConfig->m_nTicksElection = election;
    pConfig->m_nTicksHeartbeat = hb;
    pConfig->m_nMaxMsgSize = noLimit;
    pConfig->m_nMaxInfilght = 256;
    pConfig->m_optionReadOnly = ReadOnlySafe;
    pConfig->m_bCheckQuorum = false;
    pConfig->m_bPreVote = false;
    for (auto peer : peers)
    {
        CRaftInfo info;
        info.m_nNodeId = int(peer);
        info.m_nPeerPort = 8818;
        info.m_nPort = 8819;
        pConfig->m_aNodes.push_back(info);
    }
    return pConfig;
}


CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents)
{
    std::string strErrMsg;
    CRaftFrame *pFrame = new CRaftFrame();
    if (!pFrame->Init(id, peers, election, hb, &kDefaultLogger, ents,NULL, strErrMsg))
    {
        delete pFrame;
        pFrame = NULL;
    }
    return pFrame;
}

CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents, ConfigFun funCfg)
{
    std::string strErrMsg;
    CRaftFrame *pFrame = new CRaftFrame();
    if (!pFrame->Init(id, peers, election, hb, &kDefaultLogger, ents, funCfg, strErrMsg))
    {
        delete pFrame;
        pFrame = NULL;
    }
    return pFrame;
}

CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, HardState &hs, ConfigFun funCfg)
{
    std::string strErrMsg;
    CRaftFrame *pFrame = new CRaftFrame();
    if (!pFrame->Init(id, peers, election, hb, &kDefaultLogger, hs, funCfg, strErrMsg))
    {
        delete pFrame;
        pFrame = NULL;
    }
    return pFrame;
}

CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb)
{
    std::string strErrMsg;
    CRaftFrame *pFrame = new CRaftFrame();
    EntryVec ents;
    if (!pFrame->Init(id, peers, election, hb, &kDefaultLogger, ents,NULL, strErrMsg))
    {
        delete pFrame;
        pFrame = NULL;
    }
    return pFrame;
}

CRaftFrame::CRaftFrame(void)
{
    m_pMsgQueue = NULL;
    m_pIoQueue = NULL;
    m_pStorage = NULL;
    m_pRaftLog = NULL;
    m_pRaftNode = NULL;
}

CRaftFrame::~CRaftFrame(void)
{
   // Uninit();
}

bool CRaftFrame::Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, EntryVec &ents, ConfigFun funCfg, std::string &strErrMsg)
{
    bool bInit = false;
    m_pConfig = newTestConfig(id, peers, election, hb);
    if (funCfg != NULL)
        funCfg(m_pConfig);
    m_pMsgQueue = new CRaftQueue();
    if (m_pMsgQueue->Init(1024, strErrMsg))
    {
        m_pIoQueue = new CRaftQueue();
        if (m_pIoQueue->Init(1024, strErrMsg))
        {
            m_pStorage = new CRaftMemStorage(pLogger);
            m_pStorage->Append(ents);
            m_pRaftLog = newLog(m_pStorage, pLogger);
            m_pRaftNode = new CRaft(m_pConfig, m_pRaftLog, m_pMsgQueue, m_pIoQueue, pLogger);
            if (m_pRaftNode->Init(strErrMsg))
                bInit = true;
        }
    }
    return bInit;
}

bool CRaftFrame::Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, HardState &hs, ConfigFun funCfg, std::string &strErrMsg)
{
    bool bInit = false;
    m_pConfig = newTestConfig(id, peers, election, hb);
    if (funCfg != NULL)
        funCfg(m_pConfig);
    m_pMsgQueue = new CRaftQueue();
    if (m_pMsgQueue->Init(1024, strErrMsg))
    {
        m_pIoQueue = new CRaftQueue();
        if (m_pIoQueue->Init(1024, strErrMsg))
        {
            m_pStorage = new CRaftMemStorage(pLogger);
            m_pStorage->SetHardState(hs);
            m_pRaftLog = newLog(m_pStorage, pLogger);
            m_pRaftNode = new CRaft(m_pConfig, m_pRaftLog, m_pMsgQueue, m_pIoQueue, pLogger);
            if (m_pRaftNode->Init(strErrMsg))
                bInit = true;
        }
    }
    return bInit;
}

void CRaftFrame::Uninit(void)
{
    FreeMessages();
    if (NULL != m_pRaftNode)
    {
        m_pRaftNode->Uninit();
        delete m_pRaftNode;
        m_pRaftNode = NULL;
    }
    if (NULL != m_pRaftLog)
    {
        delete m_pRaftLog;
        m_pRaftLog = NULL;
    }
    if (NULL != m_pStorage)
    {
        delete m_pStorage;
        m_pStorage = NULL;
    }
    if (NULL != m_pIoQueue)
    {
        delete m_pIoQueue;
        m_pIoQueue = NULL;
    }
    if (NULL != m_pMsgQueue)
    {
        delete m_pMsgQueue;
        m_pMsgQueue = NULL;
    }
    if (NULL != m_pConfig)
    {
        delete m_pConfig;
        m_pConfig = NULL;
    }
}

void CRaftFrame::ReadMessages(vector<Message*> &msgs)
{
    FreeMessages(msgs);
    void *pMsg = m_pMsgQueue->Pop(0);
    while (NULL != pMsg)
    {
        msgs.push_back(static_cast<Message*>(pMsg));
        pMsg = m_pMsgQueue->Pop(0);
    }
}

void CRaftFrame::FreeMessages(vector<Message*> &msgs)
{
    for (auto pMsg : msgs)
        delete pMsg;
    msgs.clear();
}

void CRaftFrame::FreeMessages(void)
{
    if (NULL != m_pMsgQueue)
    {
        void *pMsg = m_pMsgQueue->Pop(0);
        while (NULL != pMsg)
        {
            delete (static_cast<Message*>(pMsg));
            pMsg = m_pMsgQueue->Pop(0);
        }
    }
    if (NULL != m_pIoQueue)
    {
        void *pMsg = m_pIoQueue->Pop(0);
        while (NULL != pMsg)
        {
            CLogOperation *pLogOperation = static_cast<CLogOperation*> (pMsg);
            delete pLogOperation;
            pMsg = m_pIoQueue->Pop(0);
        }
    }
}

void idsBySize(int size, vector<uint32_t>* ids)
{
    int i = 0;
    for (i = 0; i < size; ++i)
    {
        ids->push_back(1 + i);
    }
}
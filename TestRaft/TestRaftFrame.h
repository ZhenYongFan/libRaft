#pragma once
#include <Raft.h>
#include <RaftConfig.h>
#include <RaftQueue.h>
#include <RaftMemLog.h>
#include <RaftMemStorage.h>

class CRaftFrame
{
public:
    CRaftFrame(void);
    
    virtual ~CRaftFrame(void);
    
    bool Init(uint64_t id, const vector<uint64_t>& peers, int election, int hb, CLogger *pLogger, std::string &strErrMsg);
    
    void Uninit(void);

    void ReadMessages(vector<Message*> &msgs);

    void FreeMessages(vector<Message*> &msgs);

    void FreeMessages(void);
public:
    CRaftConfig *m_pConfig;
    CRaftQueue *m_pMsgQueue;
    CRaftQueue *m_pIoQueue;
    CRaftMemStorage *m_pStorage;
    CRaftLog *m_pRaftLog;
    CRaft *m_pRaftNode;
};

CRaftFrame* newTestRaft(uint64_t id, const vector<uint64_t>& peers, int election, int hb);
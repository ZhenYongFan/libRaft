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
    
    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger,EntryVec &ents,std::string &strErrMsg);
    
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
CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb);

CRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents);
void idsBySize(int size, vector<uint32_t>* ids);
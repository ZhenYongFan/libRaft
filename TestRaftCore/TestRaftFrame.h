#pragma once
#include <Raft.h>
#include <RaftConfig.h>
#include <RaftQueue.h>
#include <RaftMemLog.h>
#include <RaftMemStorage.h>
typedef void(*ConfigFun)(CRaftConfig*);

class CTestRaftFrame
{
public:
    CTestRaftFrame(void);
    
    virtual ~CTestRaftFrame(void);
    
    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, CSnapshot &ss, std::string &strErrMsg);

    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger,EntryVec &ents,ConfigFun funCfg,std::string &strErrMsg);
    
    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, EntryVec &ents, uint64_t u64Term, ConfigFun funCfg, std::string &strErrMsg);

    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, EntryVec &ents, uint64_t u64Term, uint64_t u64Committed, uint64_t u64Applied,ConfigFun funCfg, std::string &strErrMsg);

    bool Init(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, CHardState &hs, ConfigFun funCfg, std::string &strErrMsg);

    void Uninit(void);

    void ReadLogOpt(vector<CLogOperation*> &opts,int nType = 0);

    void FreeLogOpt(vector<CLogOperation*> &opts);

    void FreeLogOpt(void);

    void ReadMessages(vector<CMessage*> &msgs);

    void FreeMessages(vector<CMessage*> &msgs);

    void FreeMessages(void);
public:
    CRaftConfig *m_pConfig;
    CRaftQueue *m_pMsgQueue;
    CRaftQueue *m_pIoQueue;
    CRaftMemStorage *m_pStorage;
    CRaftLog *m_pRaftLog;
    CRaft *m_pRaftNode;
};
CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CSnapshot &ss);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents,uint64_t u64Term);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents, uint64_t u64Term, uint64_t u64Committed, uint64_t u64Applied);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, EntryVec &ents, ConfigFun funCfg);

CTestRaftFrame* newTestRaft(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CHardState &hs, ConfigFun funCfg);

void idsBySize(int size, vector<uint32_t>* ids);
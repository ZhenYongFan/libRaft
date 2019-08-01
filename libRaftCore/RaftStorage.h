#pragma once
#include "libRaftCore.h"
#include "RaftDef.h"

///\brief 保存Raft日志的稳定部分，对应CUnstableLog
class LIBRAFTCORE_API CRaftStorage
{
public:
    ///\brief 构造函数
    CRaftStorage(void)
    {
        m_u64Committed = 0;
        m_u64Applied = 0;
    };

    ///\brief 析构函数
    virtual ~CRaftStorage(void)
    {
    }

    virtual int InitialState(HardState &hs, ConfState &cs) = 0;
    virtual int FirstIndex(uint64_t &u64Index) = 0;
    virtual int LastIndex(uint64_t &u64Index) = 0;
    virtual int SetCommitted(uint64_t u64Committed) = 0;
    virtual int SetApplied(uint64_t u64tApplied) = 0;
    virtual int Term(uint64_t u64Index, uint64_t &u64Term) = 0;
    virtual int Append(const EntryVec& entries) = 0;
    virtual int Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<Entry> &entries) = 0;
    virtual int SetHardState(const HardState&) = 0;
    virtual int GetSnapshot(Snapshot **snapshot) = 0;
    virtual int CreateSnapshot(uint64_t i, ConfState *cs, const string& data, Snapshot *ss) = 0;
public:
    ///\brief 提交的日志号
    uint64_t m_u64Committed;

    ///\brief 应用的日志号
    ///\attention 应用和提交日志号的关系 m_u64Applied <= m_u64Committed 
    uint64_t m_u64Applied;
};



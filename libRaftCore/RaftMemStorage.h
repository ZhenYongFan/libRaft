#pragma once
#include "libRaftCore.h"
#include "RaftDef.h"
#include "RaftStorage.h"

///\brief 用内存保存Raft日志的稳定部分，对应CUnstableLog
class LIBRAFTCORE_API CRaftMemStorage :public CRaftStorage
{
public:
    ///\brief 构造函数
    ///\param pLogger 用于输出日志的对象
    CRaftMemStorage(CLogger *pLogger);

    ///\brief 析构函数
    virtual ~CRaftMemStorage(void);

    int FirstIndex(uint64_t &u64Index);
    int LastIndex(uint64_t &u64Index);
    virtual int SetCommitted(uint64_t u64Committed);
    virtual int SetApplied(uint64_t u64tApplied);
    int Term(uint64_t u64Index, uint64_t &u64Term);
    int Entries(uint64_t lo, uint64_t hi, uint64_t maxSize, vector<Entry> &entries);
    int GetSnapshot(Snapshot **snapshot);
    int InitialState(HardState &hs, ConfState &cs);
    int SetHardState(const HardState&);

    int Append(const EntryVec& entries);
    int Compact(uint64_t compactIndex);
    int ApplySnapshot(const Snapshot& snapshot);
    int CreateSnapshot(uint64_t i, ConfState *cs, const string& data, Snapshot *ss);

private:
    uint64_t firstIndex();
    uint64_t lastIndex();

public:
    HardState hardState_;
    Snapshot  *m_pSnapShot;
    // ents[i] has raft log position i+snapshot.Metadata.Index
    EntryVec entries_;

    std::mutex m_mutexStorage;         ///< 多线程保护
    CLogger *m_pLogger;
};


#pragma once
#include "libRaftCore.h"
#include "RaftDef.h"

//使用内存数组维护所有的更新日志项。
//对于Leader节点来说，它维护了客户端的更新请求对应的日志项；
//对于Follower节点而言，它维护的是Leader节点复制的日志项。
//无论是Leader还是Follower节点，日志项首先都会被存储在unstable结构，
//然后再由其内部状态机将unstable维护的日志项交给上层应用，
//由应用负责将这些日志项进行持久化并转发至系统其它节点。
//这也是为什么它被称为unstable的原因：
//在unstable中的日志项都是不安全的，尚未持久化存储，可能会因意外而丢失。
// unstable.entries[i] has raft log position i+unstable.offset.
// Note that unstable.offset may be less than the highest log
// position in storage; this means that the next write to storage
// might need to truncate the log before persisting unstable.entries.
class LIBRAFTCORE_API CUnstableLog
{
public:
    CUnstableLog(CLogger *pLogger) : m_pSnapshot(NULL), m_pLogger(pLogger)
    {
    }

    void TruncateAndAppend(const EntryVec& entries);

    bool MaybeFirstIndex(uint64_t &u64First);

    bool MaybeLastIndex(uint64_t &u64Last);

    bool MaybeTerm(uint64_t u64Index, uint64_t  &u64Term);

    void StableTo(uint64_t u64Index, uint64_t u64Term);

    void StableSnapTo(uint64_t u64Index);

    void Restore(const Snapshot& snapshot);

    void Slice(uint64_t u64Low, uint64_t u64High, EntryVec &vecEntries);
protected:
    void AssertCheckOutOfBounds(uint64_t lo, uint64_t hi);
public:
    //the incoming unstable snapshot, if any.
    Snapshot* m_pSnapshot;
    //all entries that have not yet been written to storage.
    EntryVec m_vecEntries;
    uint64_t m_u64Offset;
    CLogger *m_pLogger;
};

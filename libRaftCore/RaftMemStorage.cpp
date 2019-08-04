#include "stdafx.h"
#include "raft.pb.h"
using namespace raftpb;

#include "RaftMemStorage.h"
#include "RaftUtil.h"

CRaftMemStorage::CRaftMemStorage(CLogger *pLogger)
    : m_pSnapShot(new Snapshot())
    , m_pLogger(pLogger)
{
    // When starting from scratch populate the list with a dummy entry at term zero.
    entries_.push_back(Entry());
}

CRaftMemStorage::~CRaftMemStorage(void)
{
    if (m_pSnapShot != NULL)
    {
        delete m_pSnapShot;
        m_pSnapShot = NULL;
    }
}

int CRaftMemStorage::InitialState(HardState &hs, ConfState &cs)
{
    hs = hardState_;
    cs = m_pSnapShot->metadata().conf_state();
    return OK;
}

int CRaftMemStorage::SetHardState(const HardState& hs)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    hardState_ = hs;
    return OK;
}

uint64_t CRaftMemStorage::firstIndex()
{
    return entries_[0].index() + 1;
}

int CRaftMemStorage::FirstIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = firstIndex();
    return OK;
}

int CRaftMemStorage::LastIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = lastIndex();
    return OK;
}

uint64_t CRaftMemStorage::lastIndex()
{
    return entries_[0].index() + entries_.size() - 1;
}

int CRaftMemStorage::SetCommitted(uint64_t u64Committed)
{
    m_u64Committed = u64Committed;
    return OK;
}

int CRaftMemStorage::SetApplied(uint64_t u64tApplied)
{
    m_u64Applied = u64tApplied;
    return OK;
}

int CRaftMemStorage::Term(uint64_t u64Index, uint64_t &u64Term)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Term = 0;
    uint64_t offset = entries_[0].index();
    if (u64Index < offset)
    {
        return ErrCompacted;
    }
    if (u64Index - offset >= entries_.size())
    {
        return ErrUnavailable;
    }
    u64Term = entries_[u64Index - offset].term();
    return OK;
}

int CRaftMemStorage::Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<Entry> &entries)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    uint64_t u64Offset = entries_[0].index();
    if (u64Low <= u64Offset)
    {
        return ErrCompacted;
    }
    if (u64High > lastIndex() + 1)
    {
        return ErrUnavailable;
    }
    // only contains dummy entries.
    if (entries_.size() == 1)
    {
        return ErrUnavailable;
    }

    for (int i = int(u64Low - u64Offset); i < int(u64High - u64Offset); ++i)
    {
        entries.push_back(entries_[i]);
    }
    limitSize(u64MaxSize, entries);
    return OK;
}

int CRaftMemStorage::GetSnapshot(Snapshot **snapshot)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    *snapshot = m_pSnapShot;
    return OK;
}

// Compact discards all log entries prior to compactIndex.
// It is the application's responsibility to not attempt to compact an index
// greater than raftLog.applied.
int CRaftMemStorage::Compact(uint64_t compactIndex)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    uint64_t offset = entries_[0].index();
    if (compactIndex <= offset)
    {
        return ErrCompacted;
    }
    if (compactIndex > lastIndex())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "compact %llu is out of bound lastindex(%llu)", compactIndex, lastIndex());
    }

    uint64_t i = compactIndex - offset;
    EntryVec entries;
    Entry entry;
    entry.set_index(entries_[i].index());
    entry.set_term(entries_[i].term());
    entries.push_back(entry);
    for (i = i + 1; i < entries_.size(); ++i)
    {
        entries.push_back(entries_[i]);
    }
    entries_ = entries;
    return OK;
}

// ApplySnapshot overwrites the contents of this Storage object with
// those of the given snapshot.
int CRaftMemStorage::ApplySnapshot(const Snapshot& snapshot)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    //handle check for old snapshot being applied
    uint64_t index = m_pSnapShot->metadata().index();
    uint64_t snapIndex = snapshot.metadata().index();
    if (index >= snapIndex)
    {
        return ErrSnapOutOfDate;
    }
    m_pSnapShot->CopyFrom(snapshot);
    
    //初始化内存中的日志
    entries_.clear();
    Entry entry;
    entry.set_index(snapshot.metadata().index());
    entry.set_term(snapshot.metadata().term());
    entries_.push_back(entry);
    return OK;
}

// Append the new entries to storage.
// entries[0].Index > ms.entries[0].Index
int CRaftMemStorage::Append(const EntryVec& entries)
{
    if (entries.empty())
    {
        return OK;
    }

    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    EntryVec appendEntries = entries;

    uint64_t first = firstIndex();
    uint64_t last = appendEntries[0].index() + appendEntries.size() - 1;

    if (last < first)
    {
        return OK;
    }

    // truncate compacted entries
    if (first > appendEntries[0].index())
    {
        uint64_t index = first - appendEntries[0].index();
        appendEntries.erase(appendEntries.begin(), appendEntries.begin() + index);
    }

    uint64_t offset = appendEntries[0].index() - entries_[0].index();
    if (entries_.size() > offset)
    {
        entries_.erase(entries_.begin(), entries_.begin() + offset);
        int i;
        for (i = 0; i < appendEntries.size(); ++i)
        {
            entries_.push_back(appendEntries[i]);
        }
        return OK;
    }

    if (entries_.size() == offset)
    {
        int i;
        for (i = 0; i < appendEntries.size(); ++i)
        {
            entries_.push_back(appendEntries[i]);
        }
        return OK;
    }

    m_pLogger->Fatalf(__FILE__, __LINE__, "missing log entry [last: %llu, append at: %llu]",
        lastIndex(), appendEntries[0].index());
    return OK;
}

// CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
// can be used to reconstruct the state at that point.
// If any configuration changes have been made since the last compaction,
// the result of the last ApplyConfChange must be passed in.
int CRaftMemStorage::CreateSnapshot(uint64_t u64Index, ConfState *pConfState, const string& data, Snapshot *ss)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    if (u64Index <= m_pSnapShot->metadata().index())
    {
        return ErrSnapOutOfDate;
    }

    uint64_t offset = entries_[0].index();
    if (u64Index > lastIndex())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "snapshot %d is out of bound lastindex(%llu)", u64Index, lastIndex());
    }

    m_pSnapShot->mutable_metadata()->set_index(u64Index);
    m_pSnapShot->mutable_metadata()->set_term(entries_[u64Index - offset].term());
    if (pConfState != NULL)
    {
        *(m_pSnapShot->mutable_metadata()->mutable_conf_state()) = *pConfState;
    }
    m_pSnapShot->set_data(data);
    if (ss != NULL)
    {
        *ss = *m_pSnapShot;
    }
    return OK;
}

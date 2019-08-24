#include "stdafx.h"
#include "RaftMemStorage.h"
#include "RaftUtil.h"
#include "RaftSerializer.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRaftMemStorage::CRaftMemStorage(CLogger *pLogger,CRaftSerializer *pRaftSerializer)
    : CRaftStorage(pRaftSerializer),
      m_pSnapShot(new CSnapshot()),
      m_pLogger(pLogger)
{
    // When starting from scratch populate the list with a dummy entry at term zero.
    entries_.push_back(CRaftEntry());
}

CRaftMemStorage::~CRaftMemStorage(void)
{
    if (m_pSnapShot != NULL)
    {
        delete m_pSnapShot;
        m_pSnapShot = NULL;
    }
}

int CRaftMemStorage::InitialState(CHardState &hs, CConfState &cs)
{
    hs = hardState_;
    cs = m_pSnapShot->metadata().conf_state();
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::SetHardState(const CHardState& hardState)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    hardState_ = hardState;
    return CRaftErrNo::eOK;
}

uint64_t CRaftMemStorage::firstIndex()
{
    return entries_[0].index() + 1;
}

int CRaftMemStorage::FirstIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = firstIndex();
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::LastIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = lastIndex();
    return CRaftErrNo::eOK;
}

uint64_t CRaftMemStorage::lastIndex()
{
    return entries_[0].index() + entries_.size() - 1;
}

int CRaftMemStorage::SetCommitted(uint64_t u64Committed)
{
    m_u64Committed = u64Committed;
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::SetApplied(uint64_t u64tApplied)
{
    m_u64Applied = u64tApplied;
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::Term(uint64_t u64Index, uint64_t &u64Term)
{
    int nErrorNo = CRaftErrNo::eOK;
    u64Term = 0;
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    uint64_t u64Offset = entries_[0].index();
    if (u64Index < u64Offset)
        nErrorNo = CRaftErrNo::ErrCompacted;
    else if (u64Index - u64Offset >= entries_.size())
        nErrorNo = CRaftErrNo::eErrUnavailable;
    else
        u64Term = entries_[u64Index - u64Offset].term();
    return nErrorNo;
}

// Append the new entries to storage.
// entries[0].Index > ms.entries[0].Index
int CRaftMemStorage::Append(const EntryVec& entries)
{
    int nErrorNo = CRaftErrNo::eOK;
    if (!entries.empty())
    {
        std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
        EntryVec appendEntries = entries;

        uint64_t first = firstIndex();
        uint64_t last = appendEntries[0].index() + appendEntries.size() - 1;

        if (last < first)
        {
            return CRaftErrNo::eOK;
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
            return CRaftErrNo::eOK;
        }

        if (entries_.size() == offset)
        {
            int i;
            for (i = 0; i < appendEntries.size(); ++i)
            {
                entries_.push_back(appendEntries[i]);
            }
            return CRaftErrNo::eOK;
        }

        m_pLogger->Fatalf(__FILE__, __LINE__, "missing log entry [last: %llu, append at: %llu]",
            lastIndex(), appendEntries[0].index());
    }
    return nErrorNo;
}

int CRaftMemStorage::Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<CRaftEntry> &entries)
{
    int nErrorNo = CRaftErrNo::eOK;
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    uint64_t u64Offset = entries_[0].index();
    if (u64Low <= u64Offset)
        nErrorNo = CRaftErrNo::ErrCompacted;
    else if (u64High > lastIndex() + 1)
        nErrorNo = CRaftErrNo::eErrUnavailable;
    else if (entries_.size() == 1)// only contains dummy entries.
        nErrorNo = CRaftErrNo::eErrUnavailable;
    else
    {
        uint64_t u64Size = 0;
        CRaftSerializer *pRaftSerializer = m_pRaftSerializer;
        CRaftSerializer serializer;
        if (pRaftSerializer == NULL)
            pRaftSerializer = &serializer;

        for (int nIndex = int(u64Low - u64Offset); nIndex < int(u64High - u64Offset); ++nIndex)
        {
            if (0 == u64MaxSize)
            {
                entries.push_back(entries_[nIndex]);
                break;
            }
            else
            {
                u64Size += pRaftSerializer->ByteSize(entries_[nIndex]);
                if (u64Size > u64MaxSize)
                    break;
                else
                    entries.push_back(entries_[nIndex]);
            }
        }
    }
    return nErrorNo;
}

int CRaftMemStorage::GetSnapshot(CSnapshot **pSnapshot)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    *pSnapshot = m_pSnapShot;
    return CRaftErrNo::eOK;
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
        return CRaftErrNo::ErrCompacted;
    }
    if (compactIndex > lastIndex())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "compact %llu is out of bound lastindex(%llu)", compactIndex, lastIndex());
    }

    uint64_t i = compactIndex - offset;
    EntryVec entries;
    CRaftEntry entry; // dump entry
    entry.set_index(entries_[i].index());
    entry.set_term(entries_[i].term());
    entries.push_back(entry);
    for (i = i + 1; i < entries_.size(); ++i)
    {
        entries.push_back(entries_[i]);
    }
    entries_ = entries;
    return CRaftErrNo::eOK;
}

// ApplySnapshot overwrites the contents of this Storage object with
// those of the given snapshot.
int CRaftMemStorage::ApplySnapshot(const CSnapshot& snapshot)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    //handle check for old snapshot being applied
    uint64_t index = m_pSnapShot->metadata().index();
    uint64_t snapIndex = snapshot.metadata().index();
    if (index >= snapIndex)
    {
        return CRaftErrNo::ErrSnapOutOfDate;
    }
    m_pSnapShot->CopyFrom(snapshot);
    
    //初始化内存中的日志
    entries_.clear();
    CRaftEntry entry;
    entry.set_index(snapshot.metadata().index());
    entry.set_term(snapshot.metadata().term());
    entries_.push_back(entry);
    return CRaftErrNo::eOK;
}

// CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
// can be used to reconstruct the state at that point.
// If any configuration changes have been made since the last compaction,
// the result of the last ApplyConfChange must be passed in.
int CRaftMemStorage::CreateSnapshot(uint64_t u64Index, CConfState *pConfState, const string& data, CSnapshot *ss)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    if (u64Index <= m_pSnapShot->metadata().index())
    {
        return CRaftErrNo::ErrSnapOutOfDate;
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
    return CRaftErrNo::eOK;
}

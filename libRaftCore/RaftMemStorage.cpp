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
    m_entries.push_back(CRaftEntry());
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
    hs = m_stateHard;
    cs = m_pSnapShot->metadata().conf_state();
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::SetHardState(const CHardState& hardState)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    m_stateHard = hardState;
    return CRaftErrNo::eOK;
}

uint64_t CRaftMemStorage::GetFirstIdx()
{
    return m_entries[0].index() + 1;
}

int CRaftMemStorage::FirstIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = GetFirstIdx();
    return CRaftErrNo::eOK;
}

int CRaftMemStorage::LastIndex(uint64_t &u64Index)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    u64Index = GetLastIdx();
    return CRaftErrNo::eOK;
}

uint64_t CRaftMemStorage::GetLastIdx()
{
    return m_entries[0].index() + m_entries.size() - 1;
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
    uint64_t u64Offset = m_entries[0].index();
    if (u64Index < u64Offset)
        nErrorNo = CRaftErrNo::ErrCompacted;
    else if (u64Index - u64Offset >= m_entries.size())
        nErrorNo = CRaftErrNo::eErrUnavailable;
    else
        u64Term = m_entries[u64Index - u64Offset].term();
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

        uint64_t first = GetFirstIdx();
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

        uint64_t offset = appendEntries[0].index() - m_entries[0].index();
        if (m_entries.size() > offset)
        {
            m_entries.erase(m_entries.begin(), m_entries.begin() + offset);
            int i;
            for (i = 0; i < appendEntries.size(); ++i)
            {
                m_entries.push_back(appendEntries[i]);
            }
            return CRaftErrNo::eOK;
        }

        if (m_entries.size() == offset)
        {
            int i;
            for (i = 0; i < appendEntries.size(); ++i)
            {
                m_entries.push_back(appendEntries[i]);
            }
            return CRaftErrNo::eOK;
        }

        m_pLogger->Fatalf(__FILE__, __LINE__, "missing log entry [last: %llu, append at: %llu]",
            GetLastIdx(), appendEntries[0].index());
    }
    return nErrorNo;
}

int CRaftMemStorage::Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<CRaftEntry> &entries)
{
    int nErrorNo = CRaftErrNo::eOK;
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    uint64_t u64Offset = m_entries[0].index();
    if (u64Low <= u64Offset)
        nErrorNo = CRaftErrNo::ErrCompacted;
    else if (u64High > GetLastIdx() + 1)
        nErrorNo = CRaftErrNo::eErrUnavailable;
    else if (m_entries.size() == 1)// only contains dummy entries.
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
                entries.push_back(m_entries[nIndex]);
                break;
            }
            else
            {
                u64Size += pRaftSerializer->ByteSize(m_entries[nIndex]);
                if (u64Size > u64MaxSize)
                    break;
                else
                    entries.push_back(m_entries[nIndex]);
            }
        }
    }
    return nErrorNo;
}

int CRaftMemStorage::GetSnapshot(CSnapshot & snapshot)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);
    snapshot = *m_pSnapShot;
    return CRaftErrNo::eOK;
}

// Compact discards all log entries prior to compactIndex.
// It is the application's responsibility to not attempt to compact an index
// greater than raftLog.applied.
int CRaftMemStorage::Compact(uint64_t u64CompactIndex)
{
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    uint64_t u64Offset = m_entries[0].index();
    if (u64CompactIndex <= u64Offset)
    {
        return CRaftErrNo::ErrCompacted;
    }
    if (u64CompactIndex > GetLastIdx())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "compact %llu is out of bound lastindex(%llu)", u64CompactIndex, GetLastIdx());
    }

    uint64_t i = u64CompactIndex - u64Offset;
    EntryVec entries;
    CRaftEntry entry; //dummy entry
    entry.set_index(m_entries[i].index());
    entry.set_term(m_entries[i].term());
    entries.push_back(entry);
    for (i = i + 1; i < m_entries.size(); ++i)
    {
        entries.push_back(m_entries[i]);
    }
    m_entries = entries;
    return CRaftErrNo::eOK;
}

// ApplySnapshot overwrites the contents of this Storage object with
// those of the given snapshot.
int CRaftMemStorage::ApplySnapshot(const CSnapshot& snapshot)
{
    int nErrNo;
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    //handle check for old snapshot being applied
    uint64_t index = m_pSnapShot->metadata().index();
    uint64_t snapIndex = snapshot.metadata().index();
    if (index >= snapIndex)
        nErrNo = CRaftErrNo::ErrSnapOutOfDate;
    else
    {
        m_pSnapShot->CopyFrom(snapshot);

        //初始化内存中的日志
        m_entries.clear();
        CRaftEntry entry; //dummy entry
        entry.set_index(snapshot.metadata().index());
        entry.set_term(snapshot.metadata().term());
        m_entries.push_back(entry);
        nErrNo = CRaftErrNo::eOK;
    }
    return nErrNo;
}

// CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
// can be used to reconstruct the state at that point.
// If any configuration changes have been made since the last compaction,
// the result of the last ApplyConfChange must be passed in.
int CRaftMemStorage::CreateSnapshot(uint64_t u64Index, CConfState *pConfState, const string& strData, CSnapshot *pSnapshot)
{
    int nErrNo ;
    std::lock_guard<std::mutex> storageGuard(m_mutexStorage);

    if (u64Index <= m_pSnapShot->metadata().index())
        nErrNo = CRaftErrNo::ErrSnapOutOfDate;
    else
    {
        if (u64Index > GetLastIdx())
        {
            m_pLogger->Fatalf(__FILE__, __LINE__, "snapshot %d is out of bound lastindex(%llu)", u64Index, GetLastIdx());
        }

        uint64_t u64Offset = m_entries[0].index();
        m_pSnapShot->mutable_metadata()->set_index(u64Index);
        m_pSnapShot->mutable_metadata()->set_term(m_entries[u64Index - u64Offset].term());
        if (pConfState != NULL)
            *(m_pSnapShot->mutable_metadata()->mutable_conf_state()) = *pConfState;
        m_pSnapShot->set_data(strData);
        if (pSnapshot != NULL)
            *pSnapshot = *m_pSnapShot;
        nErrNo = CRaftErrNo::eOK;
    }
    return nErrNo;
}

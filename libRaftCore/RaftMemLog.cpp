#include "stdafx.h"
#include <stdint.h>
#include "RaftMemLog.h"
#include "RaftUtil.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRaftMemLog::CRaftMemLog(CRaftStorage *pStorage, CLogger *pLogger)
    : m_pStorage(pStorage),
    m_unstablePart(pLogger),
    m_pLogger(pLogger)
{

}

CRaftMemLog::~CRaftMemLog(void)
{

}

// maybeAppend returns false if the entries cannot be appended. Otherwise,
// it returns last index of new entries.
bool CRaftMemLog::MaybeAppend(uint64_t u64Index, uint64_t u64LogTerm,
    uint64_t u64Committed, const EntryVec& entries, uint64_t &u64LastIndex)
{
    u64LastIndex = 0;
    if (!IsMatchTerm(u64Index, u64LogTerm))
    {
        return false;
    }

    uint64_t lastNewI = u64Index + (uint64_t)entries.size();
    uint64_t ci = findConflict(entries);

    if (ci != 0 )
    {
        if(ci <= m_pStorage->m_u64Committed)
            m_pLogger->Fatalf(__FILE__, __LINE__, "entry %lu conflict with committed entry [committed(%lu)]", ci, m_pStorage->m_u64Committed);
        else
        {
            uint64_t offset = u64Index + 1;
            EntryVec appendEntries(entries.begin() + (ci - offset), entries.end());
            Append(appendEntries);
        }
    }
    //Follower提交之前的日志
    CommitTo(min(u64Committed, lastNewI));
    u64LastIndex = lastNewI;
    return true;
}

bool CRaftMemLog::CommitTo(uint64_t u64Committed)
{
    bool bCommitted = false;
    if (u64Committed > m_pStorage->m_u64Committed)
    {
        if (u64Committed <= GetLastIndex())
        {
            m_pStorage->SetCommitted(u64Committed);
            bCommitted = true;
            m_pLogger->Debugf(__FILE__, __LINE__, "commit to %lu", m_pStorage->m_u64Committed);
        }
        else
        {
            m_pLogger->Fatalf(__FILE__, __LINE__,
                "to commit(%lu) is out of range [lastIndex(%lu)]. Was the raft log corrupted, truncated, or lost?",
                u64Committed, GetLastIndex());
        }
    }
    return bCommitted;
}

uint64_t CRaftMemLog::GetCommitted(void)
{
    return m_pStorage->m_u64Committed;
}

void CRaftMemLog::AppliedTo(uint64_t u64Applied)
{
    if (u64Applied != 0)
    {
        if (u64Applied >= m_pStorage->m_u64Applied && u64Applied <= m_pStorage->m_u64Committed)
            m_pStorage->SetApplied(u64Applied);
        else
            m_pLogger->Fatalf(__FILE__, __LINE__, "applied(%lu) is out of range [prevApplied(%lu), committed(%lu)]", u64Applied, m_pStorage->m_u64Applied, m_pStorage->m_u64Committed);
    }
}

uint64_t CRaftMemLog::GetApplied(void)
{
    return m_pStorage->m_u64Applied;
}

void CRaftMemLog::StableTo(uint64_t u64Index, uint64_t u64Term)
{
    m_unstablePart.StableTo(u64Index, u64Term);
}

void CRaftMemLog::stableSnapTo(uint64_t i)
{
    m_unstablePart.StableSnapTo(i);
}

uint64_t CRaftMemLog::GetLastTerm(void)
{
    uint64_t u64Term;
    int nErrorNo = GetTerm(GetLastIndex(), u64Term);
    if (!SUCCESS(nErrorNo))
        m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected error when getting the last term (%s)", GetErrorString(nErrorNo));
    return u64Term;
}

int CRaftMemLog::GetEntries(uint64_t u64Index, uint64_t maxSize, EntryVec &entries)
{
    entries.clear();
    uint64_t u64LastIndex = GetLastIndex();

    if (u64Index > u64LastIndex)
    {
        return OK;
    }

    return GetSliceEntries(u64Index, u64LastIndex + 1, maxSize, entries);
}

// allEntries returns all entries in the log.
void CRaftMemLog::allEntries(EntryVec &entries)
{
    int nErrorNo = GetEntries(GetFirstIndex(), noLimit, entries);
    if (SUCCESS(nErrorNo))
    {
        return;
    }

    if (nErrorNo == ErrCompacted) // try again if there was a racing compaction 
    {
        return allEntries(entries);
    }
    // TODO (xiangli): handle error? 
    m_pLogger->Fatalf(__FILE__, __LINE__, "allEntries fatal: %s", GetErrorString(nErrorNo));
}

bool CRaftMemLog::IsUpToDate(uint64_t u64Index, uint64_t u64Term)
{
    uint64_t u64LastTerm = GetLastTerm();
    return (u64Term > u64LastTerm) 
        || (u64Term == u64LastTerm && u64Index >= GetLastIndex());
}

bool CRaftMemLog::MaybeCommit(uint64_t u64Index, uint64_t u64Term)
{
    bool bCommit = false;
    uint64_t u64TestTerm;
    int nErrorNo = GetTerm(u64Index, u64TestTerm);
    if (u64Index > m_pStorage->m_u64Committed && ZeroTermOnErrCompacted(u64TestTerm, nErrorNo) == u64Term)
        bCommit = CommitTo(u64Index);
    return bCommit;
}

void CRaftMemLog::Restore(const CSnapshot& snapshot)
{
    m_pLogger->Infof(__FILE__, __LINE__, "log [%s] starts to restore snapshot [index: %lu, term: %lu]",
        String().c_str(), snapshot.metadata().index(), snapshot.metadata().term());
    m_pStorage->SetCommitted(snapshot.metadata().index());
    m_unstablePart.Restore(snapshot);
}

int CRaftMemLog::InitialState(CHardState &hs, CConfState &cs)
{
    return m_pStorage->InitialState(hs, cs);
}

// append entries to unstable storage and return last index
// fatal if the first index of entries < committed_
uint64_t CRaftMemLog::Append(const EntryVec& entries)
{
    if (entries.empty())
    {
        return GetLastIndex();
    }

    uint64_t after = entries[0].index() - 1;
    if (after < m_pStorage->m_u64Committed)
        m_pLogger->Fatalf(__FILE__, __LINE__, "after(%lu) is out of range [committed(%lu)]", after, m_pStorage->m_u64Committed);

    m_unstablePart.TruncateAndAppend(entries);
    return GetLastIndex();
}

// findConflict finds the index of the conflict.
// It returns the first pair of conflicting entries between the existing
// entries and the given entries, if there are any.
// If there is no conflicting entries, and the existing entries contains
// all the given entries, zero will be returned.
// If there is no conflicting entries, but the given entries contains new
// entries, the index of the first new entry will be returned.
// An entry is considered to be conflicting if it has the same index but
// a different term.
// The first entry MUST have an index equal to the argument 'from'.
// The index of the given entries MUST be continuously increasing.
uint64_t CRaftMemLog::findConflict(const EntryVec& entries)
{
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (!IsMatchTerm(entries[i].index(), entries[i].term()))
        {
            const CRaftEntry& entry = entries[i];
            uint64_t index = entry.index();
            uint64_t term = entry.term();

            if (index <= GetLastIndex())
            {
                uint64_t dummy;
                int err = GetTerm(index, dummy);
                m_pLogger->Infof(__FILE__, __LINE__, "found conflict at index %lu [existing term: %lu, conflicting term: %lu]",
                    index, ZeroTermOnErrCompacted(dummy, err), term);
            }
            return index;
        }
    }
    return 0;
}

void CRaftMemLog::unstableEntries(EntryVec &entries)
{
    entries = m_unstablePart.m_vecEntries;
}

// nextEntries returns all the available entries for execution.
// If applied is smaller than the index of snapshot, it returns all committed
// entries after the index of snapshot.
void CRaftMemLog::nextEntries(EntryVec& entries)
{
    entries.clear();
    uint64_t offset = max(m_pStorage->m_u64Applied + 1, GetFirstIndex());
    if (m_pStorage->m_u64Committed + 1 > offset)
    {
        int err = GetSliceEntries(offset, m_pStorage->m_u64Committed + 1, noLimit, entries);
        if (!SUCCESS(err))
        {
            m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected error when getting unapplied entries (%s)", GetErrorString(err));
        }
    }
}

string CRaftMemLog::String(void)
{
    char tmp[200];
    snprintf(tmp, sizeof(tmp), "committed=%llu, applied=%llu, unstable.offset=%llu, len(unstable.Entries)=%llu",
        m_pStorage->m_u64Committed, m_pStorage->m_u64Applied, 
        m_unstablePart.m_u64Offset, m_unstablePart.m_vecEntries.size());
    return tmp;
}

// hasNextEntries returns if there is any available entries for execution. This
// is a fast check without heavy raftLog.slice() in raftLog.nextEnts().
bool CRaftMemLog::hasNextEntries() {
  return m_pStorage->m_u64Committed + 1 > max(m_pStorage->m_u64Applied + 1, GetFirstIndex());
}

int CRaftMemLog::snapshot(CSnapshot **snapshot)
{
    if (m_unstablePart.m_pSnapshot != NULL)
    {
        *snapshot = m_unstablePart.m_pSnapshot;
        return OK;
    }

    return m_pStorage->GetSnapshot(snapshot);
}

uint64_t CRaftMemLog::ZeroTermOnErrCompacted(uint64_t u64Term, int nErrorNo)
{
    if (SUCCESS(nErrorNo))
        return u64Term;
    else if (nErrorNo == ErrCompacted)
        return 0;
    else
        m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected error: %s", GetErrorString(nErrorNo));
    return 0;
}

void CRaftMemLog::UnstableEntries(EntryVec &entries)
{
    entries.clear();
    entries = m_unstablePart.m_vecEntries;
}

bool CRaftMemLog::IsMatchTerm(uint64_t u64Index, uint64_t u64Term)
{
    bool bMatch = false;
    uint64_t u64TempTerm;

    int nErrorNo = GetTerm(u64Index, u64TempTerm);
    if (SUCCESS(nErrorNo))
        bMatch = (u64TempTerm == u64Term);
    return bMatch;
}

int CRaftMemLog::GetTerm(uint64_t u64Index, uint64_t &u64Term)
{
    uint64_t dummyIndex;
    int nErrorNo = OK;

    u64Term = 0;
    // the valid term range is [index of dummy entry, last index]
    dummyIndex = GetFirstIndex() - 1;
    if (u64Index < dummyIndex || u64Index > GetLastIndex())
    {
        return OK;
    }

    bool ok = m_unstablePart.MaybeTerm(u64Index, u64Term);
    if (ok)
    {
        goto out;
    }

    nErrorNo = m_pStorage->Term(u64Index, u64Term);
    if (nErrorNo != OK && nErrorNo != ErrCompacted && nErrorNo != ErrUnavailable)
        m_pLogger->Fatalf(__FILE__, __LINE__, "term err:%s", GetErrorString(nErrorNo));
out:
    return nErrorNo;
}

uint64_t CRaftMemLog::GetFirstIndex(void)
{
    uint64_t u64FirstIndex;
    if (!m_unstablePart.MaybeFirstIndex(u64FirstIndex))
    {
        int nErrorNo = m_pStorage->FirstIndex(u64FirstIndex);
        if (!SUCCESS(nErrorNo))
            m_pLogger->Fatalf(__FILE__, __LINE__, "firstIndex error:%s", GetErrorString(nErrorNo));
    }
    return u64FirstIndex;
}

uint64_t CRaftMemLog::GetLastIndex(void)
{
    uint64_t u64LastIndex;
    if (!m_unstablePart.MaybeLastIndex(u64LastIndex))
    {
        int nErrorNo = m_pStorage->LastIndex(u64LastIndex);
        if (!SUCCESS(nErrorNo))
            m_pLogger->Fatalf(__FILE__, __LINE__, "lastIndex error:%s", GetErrorString(nErrorNo));
    }
    return u64LastIndex;
}

// slice returns a slice of log entries from lo through hi-1, inclusive.
int CRaftMemLog::GetSliceEntries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, EntryVec &entries)
{
    int nErrorNo = CheckOutOfBounds(u64Low, u64High);
    if (!SUCCESS(nErrorNo))
        return nErrorNo;
    if (u64Low == u64High)
        return OK;

    if (u64Low < m_unstablePart.m_u64Offset)
    {
        nErrorNo = m_pStorage->Entries(u64Low, min(u64High, m_unstablePart.m_u64Offset), u64MaxSize, entries);
        if (nErrorNo == ErrCompacted)
            return nErrorNo;
        else if (nErrorNo == ErrUnavailable)
            m_pLogger->Fatalf(__FILE__, __LINE__, "entries[%lu:%lu) is unavailable from storage", u64Low, min(u64High, m_unstablePart.m_u64Offset));
        else if (!SUCCESS(nErrorNo))
            m_pLogger->Fatalf(__FILE__, __LINE__, "storage entries err:%s", GetErrorString(nErrorNo));

        if ((uint64_t)entries.size() < min(u64High, m_unstablePart.m_u64Offset) - u64Low)
            return OK;
    }

    if (u64High > m_unstablePart.m_u64Offset)
    {
        EntryVec unstable;
        m_unstablePart.Slice(max(u64Low, m_unstablePart.m_u64Offset), u64High, unstable);
        if (entries.size() > 0)
            entries.insert(entries.end(), unstable.begin(), unstable.end());
        else
            entries = unstable;
    }

    limitSize(u64MaxSize, entries);
    return OK;
}

int CRaftMemLog::CheckOutOfBounds(uint64_t u64Low, uint64_t u64High)
{
    int nCheck = -1;
    if (u64Low > u64High)
        m_pLogger->Fatalf(__FILE__, __LINE__, "invalid slice %lu > %lu", u64Low, u64High);
    else
    {
        uint64_t u64First = GetFirstIndex();
        if (u64Low < u64First)
            nCheck = ErrCompacted;
        else
        {
            uint64_t u64Last = GetLastIndex();
            if (u64High > u64Last + 1)
                m_pLogger->Fatalf(__FILE__, __LINE__, "slice[%lu,%lu) out of bound [%lu,%lu]", u64Low, u64High, u64First, u64Last);
            else
                nCheck = OK;
        }
    }
    return nCheck;
}

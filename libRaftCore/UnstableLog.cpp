#include "stdafx.h"
#include "raft.pb.h"
using namespace raftpb;

#include "UnstableLog.h"

// maybeFirstIndex returns the index of the first possible entry in entries
// if it has a snapshot.
bool CUnstableLog::MaybeFirstIndex(uint64_t &u64First)
{
    bool bGet = true;
    if (m_pSnapshot != NULL)
        u64First = m_pSnapshot->metadata().index() + 1;
    else
    {
        u64First = 0;
        bGet = false;
    }
    return bGet;
}

// maybeLastIndex returns the last index if it has at least one
// unstable entry or snapshot.
bool CUnstableLog::MaybeLastIndex(uint64_t &u64Last)
{
    bool bGet = true;
    if (m_vecEntries.size() > 0)
        u64Last = m_u64Offset + m_vecEntries.size() - 1;
    else if (m_pSnapshot != NULL)
        u64Last = m_pSnapshot->metadata().index();
    else
    {
        u64Last = 0;
        bGet = false;
    }
    return bGet;
}

// maybeTerm returns the term of the entry at index i, if there is any.
bool CUnstableLog::MaybeTerm(uint64_t u64Index, uint64_t &u64Term)
{
    bool bGet = false;
    u64Term = 0;
    if (u64Index < m_u64Offset)
    {
        if (NULL != m_pSnapshot)
        {
            if (m_pSnapshot->metadata().index() == u64Index)
            {
                u64Term = m_pSnapshot->metadata().term();
                bGet = true;
            }
        }
    }
    else
    {
        uint64_t u64Last;
        if (MaybeLastIndex(u64Last))
        {
            if (u64Index <= u64Last)
            {
                u64Term = m_vecEntries[u64Index - m_u64Offset].term();
                bGet = true;
            }
        }
    }
    return bGet;
}

void CUnstableLog::StableTo(uint64_t u64Index, uint64_t u64Term)
{
    uint64_t u64GetTerm;
    if (MaybeTerm(u64Index, u64GetTerm))
    {
        // if i < offset, term is matched with the snapshot
        // only update the unstable entries if term is matched with
        // an unstable entry.
        if (u64GetTerm == u64Term && u64Index >= m_u64Offset)
        {
            m_vecEntries.erase(m_vecEntries.begin(), m_vecEntries.begin() + (u64Index + 1 - m_u64Offset));
            m_u64Offset = u64Index + 1;
        }
    }
}

void CUnstableLog::StableSnapTo(uint64_t u64Index)
{
    if (m_pSnapshot != NULL && m_pSnapshot->metadata().index() == u64Index)
    {
        delete m_pSnapshot;
        m_pSnapshot = NULL;
    }
}

void CUnstableLog::Restore(const Snapshot& snapshot)
{
    m_u64Offset = snapshot.metadata().index() + 1;
    m_vecEntries.clear();
    if (m_pSnapshot == NULL)
        m_pSnapshot = new Snapshot();
    m_pSnapshot->CopyFrom(snapshot);
}

void CUnstableLog::TruncateAndAppend(const EntryVec& entriesApp)
{
    uint64_t u64After = entriesApp[0].index();

    if (u64After == m_u64Offset + uint64_t(m_vecEntries.size()))
    {
        // after is the next index in the u.entries
        // directly append
        m_vecEntries.insert(m_vecEntries.end(), entriesApp.begin(), entriesApp.end());
        if(m_pLogger != NULL)
            m_pLogger->Infof(__FILE__, __LINE__, "ENTRY size: %d", m_vecEntries.size());
    }
    else if (u64After <= m_u64Offset)
    {
        // The log is being truncated to before our current offset
        // portion, so set the offset and replace the entries
        if (m_pLogger != NULL)
            m_pLogger->Infof(__FILE__, __LINE__, "replace the unstable entries from index %llu", u64After);
        m_u64Offset = u64After;
        m_vecEntries = entriesApp;
    }
    else
    {
        // truncate to after and copy to u.entries then append
        if (m_pLogger != NULL)
            m_pLogger->Infof(__FILE__, __LINE__, "truncate the unstable entries before index %llu", u64After);
        vector<Entry> vecEntries; //vector没有删除头尾的接口，此处必须使用临时变量
        Slice(m_u64Offset, u64After, vecEntries);
        m_vecEntries = vecEntries;
        m_vecEntries.insert(m_vecEntries.end(), entriesApp.begin(), entriesApp.end());
    }
}

void CUnstableLog::Slice(uint64_t u64Low, uint64_t u64High, EntryVec &vecEntries)
{
    AssertCheckOutOfBounds(u64Low, u64High);
    vecEntries.assign(m_vecEntries.begin() + (u64Low - m_u64Offset), m_vecEntries.begin() + (u64High - m_u64Offset));
}

//offset <= lo <= hi <= offset+len(entries)
void CUnstableLog::AssertCheckOutOfBounds(uint64_t u64Low, uint64_t u64High)
{
    if (u64Low > u64High)
    {
        if (m_pLogger != NULL)
            m_pLogger->Fatalf(__FILE__, __LINE__, "invalid unstable.slice %llu > %llu", u64Low, u64High);
    }
    uint64_t u64Upper = m_u64Offset + (uint64_t)m_vecEntries.size();
    if (u64Low < m_u64Offset || u64Upper < u64High)
    {
        if (m_pLogger != NULL)
            m_pLogger->Fatalf(__FILE__, __LINE__, "unstable.slice[%llu,%llu) out of bound [%llu,%llu]", u64Low, u64High, m_u64Offset, u64Upper);
    }
}

#include "stdafx.h"
#include <algorithm>
#include "Progress.h"
#include "RaftLogger.h"

CProgress::CProgress(uint64_t next, int maxInfilght, CLogger *pLogger)
    : m_u64MatchLogIndex(0),
    m_u64NextLogIndex(next),
    state_(ProgressStateProbe),
    m_bPaused(false),
    pendingSnapshot_(0),
    m_bRecentActive(false),
    ins_(maxInfilght, pLogger),
    m_pLogger(pLogger)
{
}

CProgress::~CProgress()
{
}

void CProgress::ResetState(ProgressState state)
{
    m_bPaused = false;
    pendingSnapshot_ = 0;
    state_ = state;
    ins_.Reset();
}

void CProgress::BecomeProbe(void)
{
    // If the original state is ProgressStateSnapshot, progress knows that
    // the pending snapshot has been sent to this peer successfully, then
    // probes from pendingSnapshot + 1.
    if (state_ == ProgressStateSnapshot)
    {
        uint64_t pendingSnapshot = pendingSnapshot_;
        ResetState(ProgressStateProbe);
        m_u64NextLogIndex = std::max(m_u64MatchLogIndex + 1, pendingSnapshot + 1);
    }
    else
    {
        ResetState(ProgressStateProbe);
        m_u64NextLogIndex = m_u64MatchLogIndex + 1;
    }
}

void CProgress::BecomeReplicate(void)
{
    ResetState(ProgressStateReplicate);
    m_u64NextLogIndex = m_u64MatchLogIndex + 1;
}

void CProgress::BecomeSnapshot(uint64_t snapshoti)
{
    ResetState(ProgressStateSnapshot);
    pendingSnapshot_ = snapshoti;
}

// maybeUpdate returns false if the given index no comes from an outdated message.
// Otherwise it updates the progress and returns true.
bool CProgress::MaybeUpdate(uint64_t u64Index)
{
    bool bUpdated = false;
    if (m_u64MatchLogIndex < u64Index)
    {
        m_u64MatchLogIndex = u64Index;
        bUpdated = true;
        Resume();
    }
    if (m_u64NextLogIndex < u64Index + 1)
        m_u64NextLogIndex = u64Index + 1;
    return bUpdated;
}

void CProgress::optimisticUpdate(uint64_t n)
{
    m_u64NextLogIndex = n + 1;
}

void CProgress::snapshotFailure(void)
{
    pendingSnapshot_ = 0;
}

// maybeDecrTo returns false if the given to index comes from an out of order message.
// Otherwise it decreases the progress next index to min(rejected, last) and returns true.
bool CProgress::maybeDecrTo(uint64_t rejected, uint64_t last)
{
    if (state_ == ProgressStateReplicate)
    {
        // the rejection must be stale if the progress has matched and "rejected"
        // is smaller than "match".
        if (rejected <= m_u64MatchLogIndex)
        {
            return false;
        }
        // directly decrease next to match + 1
        m_u64NextLogIndex = m_u64MatchLogIndex + 1;
        return true;
    }

    // the rejection must be stale if "rejected" does not match next - 1
    if (m_u64NextLogIndex - 1 != rejected)
    {
        return false;
    }

    m_u64NextLogIndex = std::min(rejected, last + 1);
    if (m_u64NextLogIndex < 1)
        m_u64NextLogIndex = 1;
    Resume();
    return true;
}

void CProgress::Pause(void)
{
    m_bPaused = true;
}

void CProgress::Resume(void)
{
    m_bPaused = false;
}

const char* CProgress::GetStateText()
{
    if (state_ == ProgressStateProbe)
    {
        return "ProgressStateProbe";
    }
    if (state_ == ProgressStateSnapshot)
    {
        return "ProgressStateSnapshot";
    }
    if (state_ == ProgressStateReplicate)
    {
        return "ProgressStateReplicate";
    }
    return "unknown state";
}

// IsPaused returns whether sending log entries to this node has been
// paused. A node may be paused because it has rejected recent
// MsgApps, is currently waiting for a snapshot, or has reached the
// MaxInflightMsgs limit.
bool CProgress::IsPaused()
{
    switch (state_)
    {
    case ProgressStateProbe:
        return m_bPaused;
    case ProgressStateReplicate:
        return ins_.IsFull();
    case ProgressStateSnapshot:
        return true;
    }
    return true;
}

// needSnapshotAbort returns true if snapshot progress's Match
// is equal or higher than the pendingSnapshot.
bool CProgress::needSnapshotAbort(void)
{
    return state_ == ProgressStateSnapshot && m_u64MatchLogIndex >= pendingSnapshot_;
}

std::string CProgress::GetInfoText(void)
{
    char tmp[500];
    snprintf(tmp, sizeof(tmp), "next = %llu, match = %llu, state = %s, waiting = %d, pendingSnapshot = %llu",
        m_u64NextLogIndex, m_u64MatchLogIndex, GetStateText(), IsPaused(), pendingSnapshot_);
    return std::string(tmp);
}

void inflights::Add(uint64_t infight)
{
    if (IsFull())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "cannot add into a full inflights");
    }

    uint64_t next = m_nStartPos + m_nCount;
    uint64_t size = m_nSize;

    if (next >= size)
        next -= size;

    if (next >= buffer_.size())
        growBuf();
    buffer_[next] = infight;
    m_nCount++;
}

// grow the inflight buffer by doubling up to inflights.size. We grow on demand
// instead of preallocating to inflights.size to handle systems which have
// thousands of Raft groups per process.
void inflights::growBuf(void)
{
    uint64_t newSize = buffer_.size() * 2;
    if (newSize == 0)
        newSize = 1;
    else if (newSize > m_nSize)
        newSize = m_nSize;
    buffer_.resize(newSize);
}

// freeTo frees the inflights smaller or equal to the given `to` flight.
void inflights::freeTo(uint64_t to)
{
    if (m_nCount == 0 || to < buffer_[m_nStartPos])
    {
        return;
    }

    int i = 0, idx = m_nStartPos;
    for (i = 0; i < m_nCount; ++i)
    {
        if (to < buffer_[idx])
        {  // found the first large inflight
            break;
        }

        // increase index and maybe rotate
        int size = m_nSize;
        ++idx;
        if (idx >= size)
        {
            idx -= size;
        }
    }

    // free i inflights and set new start index
    m_nCount -= i;
    m_nStartPos = idx;
    if (m_nCount == 0)
    {
        // inflights is empty, reset the start index so that we don't grow the
        // buffer unnecessarily.
        m_nStartPos = 0;
    }
}

void inflights::freeFirstOne(void)
{
    freeTo(buffer_[m_nStartPos]);
}

bool inflights::IsFull(void)
{
    return m_nCount == m_nSize;
}

void inflights::Reset(void)
{
    m_nCount = 0;
    m_nStartPos = 0;
}

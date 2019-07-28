#pragma once
#include "libRaftCore.h"

class CLogger;

//etcd中的环形队列inflights https://blog.csdn.net/skh2015java/article/details/86716979
///\brief 环形队列
struct LIBRAFTCORE_API inflights
{
    // the starting index in the buffer
    int start_;

    // number of inflights in the buffer
    int count_;

    // the size of the buffer
    int size_;

    // buffer contains the index of the last entry
    // inside one message.
    std::vector<uint64_t> buffer_;

    CLogger* logger_;

    void add(uint64_t infight);
    void growBuf(void);
    void freeTo(uint64_t to);
    void freeFirstOne();
    bool full();
    void reset();

    inflights(int size, CLogger *logger)
        : start_(0),
        count_(0),
        size_(size),
        logger_(logger)
    {
        buffer_.resize(size);
    }
    ~inflights()
    {
    }
};

enum ProgressState {
  ProgressStateProbe     = 0, ///< 表示节点一次不能向目标节点发送多条消息，只能待一条消息被响应之后，才能发送下一条消息
  ProgressStateReplicate = 1, ///< 表示正常的Entry记录复制状态，Leader节点向目标节点发送完消息之后，无需等待响应，即可开始后续消息发送。
  ProgressStateSnapshot  = 2  ///< 表示Leader节点正在向目标节点发送快照数据
};

// Progress represents a follower’s progress in the view of the leader. Leader maintains
// progresses of all followers, and sends entries to the follower based on its progress.
class LIBRAFTCORE_API CProgress
{
public:
    CProgress(uint64_t u64Next, int nMaxInfilght, CLogger *pLogger);
    ~CProgress(void);
    
    const char* stateString();
    void ResetState(ProgressState state);

    void BecomeProbe(void);
    void BecomeReplicate(void);
    void BecomeSnapshot(uint64_t snapshoti);
    
    bool MaybeUpdate(uint64_t u64Index);
    void optimisticUpdate(uint64_t n);
    bool maybeDecrTo(uint64_t rejected, uint64_t last);
    void snapshotFailure();

    void Pause(void);
    void Resume(void);
    bool IsPaused(void);

    bool needSnapshotAbort();
    std::string GetInfoText(void);
public:
    uint64_t m_u64NextLogIndex;  ///< 对于每个服务器，需要下发的下一日志的索引值
    uint64_t m_u64MatchLogIndex; ///< 对于每个服务器，已经复制给他的最高日志索引值

    // State defines how the leader should interact with the follower.
    //
    // When in ProgressStateProbe, leader sends at most one replication message
    // per heartbeat interval. It also probes actual progress of the follower.
    //
    // When in ProgressStateReplicate, leader optimistically increases next
    // to the latest entry sent after sending replication message. This is
    // an optimized state for fast replicating log entries to the follower.
    //
    // When in ProgressStateSnapshot, leader should have sent out snapshot
    // before and stops sending any replication message.
    ProgressState state_;

    // Paused is used in ProgressStateProbe.
    // When Paused is true, raft should pause sending replication message to this peer.
    bool m_bPaused;

    // PendingSnapshot is used in ProgressStateSnapshot.
    // If there is a pending snapshot, the pendingSnapshot will be set to the
    // index of the snapshot. If pendingSnapshot is set, the replication process of
    // this Progress will be paused. raft will not resend snapshot until the pending one
    // is reported to be failed.
    uint64_t pendingSnapshot_;

    // RecentActive is true if the progress is recently active. Receiving any messages
    // from the corresponding follower indicates the progress is active.
    // RecentActive can be reset to false after an election timeout.
    bool m_bRecentActive;

    // inflights is a sliding window for the inflight messages.
    // Each inflight message contains one or more log entries.
    // The max number of entries per message is defined in raft config as MaxSizePerMsg.
    // Thus inflight effectively limits both the number of inflight messages
    // and the bandwidth each Progress can use.
    // When inflights is full, no more message should be sent.
    // When a leader sends out a message, the index of the last
    // entry should be added to inflights. The index MUST be added
    // into inflights in order.
    // When a leader receives a reply, the previous inflights should
    // be freed by calling inflights.freeTo with the index of the last
    // received entry.
    inflights ins_;
    CLogger* m_pLogger;
};


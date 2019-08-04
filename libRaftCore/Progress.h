#pragma once
#include "libRaftCore.h"

class CLogger;

//etcd中的环形队列inflights https://blog.csdn.net/skh2015java/article/details/86716979
///\brief 环形队列
struct LIBRAFTCORE_API inflights
{
    ///\brief 缓冲区中存数据的起始位置
    int m_nStartPos;

    ///\brief 队列中当前的对象个数
    int m_nCount;

    ///\brief 队列缓冲区大小
    int m_nSize;

    ///\brief buffer contains the index of the last entry inside one message.
    std::vector<uint64_t> buffer_;
   
    ///\brief 日志输出器
    CLogger* m_pLogger;

    void Add(uint64_t infight);
    void growBuf(void);
    void freeTo(uint64_t to);
    void freeFirstOne(void);
    bool IsFull(void);
    void Reset(void);

    inflights(int nSize, CLogger *logger)
        : m_nStartPos(0),
        m_nCount(0),
        m_nSize(nSize),
        m_pLogger(logger)
    {
        buffer_.resize(nSize);
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

///\brief 追随者日志同步的进度，由Leader负责维护，根据进度向Follower发送日志对象
class LIBRAFTCORE_API CProgress
{
public:
    ///\brief 构造函数
    ///\param u64Next 下一个日志号
    ///\param nMaxInfilght 环形队列最大容量
    ///\param pLogger 日志输出对象
    CProgress(uint64_t u64Next, int nMaxInfilght, CLogger *pLogger);

    ///\brief 析构函数
    ///\attention 为了效率，此处不是虚函数呀
    ~CProgress(void);
    
    ///\brief 取得状态对应的名称
    ///\return 状态对应的名称
    const char* GetStateText(void);

    ///\brief 设置状态，并复位
    ///\param statePro 状态
    void ResetState(ProgressState statePro);

    ///\brief 状态转换为Probe
    void BecomeProbe(void);

    ///\brief 状态转换为Replicate
    void BecomeReplicate(void);

    ///\brief 状态转换为Snapshot
    void BecomeSnapshot(uint64_t snapshoti);
    
    ///\brief 尝试更新
    ///\param u64Index 日志索引号
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

    ///\brief 在ProgressStateProbe状态时生效，如果是true，raft暂停向其发送复制消息
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


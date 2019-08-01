#pragma once

#include "libRaftCore.h"
#include "RaftLog.h"
#include "Progress.h"
#include "RaftQueue.h"
using namespace std;

class CReadOnly;
class CReadState;

///\brief Raft核心状态机
class LIBRAFTCORE_API CRaft
{
public:
    ///\brief 发起选举的类型
    enum CampaignType
    {
        // campaignPreElection represents the first phase of a normal election when
        // Config.PreVote is true.
        campaignPreElection = 1,
        // campaignElection represents a normal (time-based) election (the second phase
        // of the election when Config.PreVote is true).
        campaignElection = 2,
        // campaignTransfer represents the type of leader transfer
        campaignTransfer = 3
    };

public:
    CRaft(CRaftConfig *pConfig, CRaftLog *pRaftLog, CRaftQueue *pMsgQueue, CRaftQueue *pIoQueue, CLogger *pLogger);
    
    ///\brief 析构函数
    virtual ~CRaft(void);
    
    ///\brief 初始化
    ///\param strErrMsg 如果初始化失败，返回错误信息
    ///\return 成功标志 true 成功；false 失败
    virtual bool Init(string &strErrMsg);

    ///\brief 释放资源
    virtual void Uninit(void);

    ///\brief 每个Tick执行的函数
    virtual void OnTick(void);
    
    ///\brief 处理消息
    virtual int  Step(const Message& msg);

public: // for test
    
    ///\brief 变化角色（状态）
    void BecomeFollower(uint64_t u64Term, uint32_t nLeaderID);
    void BecomeCandidate(void);
    void BecomePreCandidate(void);
    void BecomeLeader(void);

    ///\brief 广播日志
    void BcastAppend(void);

    ///\brief 取得当前状态
    inline EStateType GetState(void)
    {
        return m_stateRaft;
    }

    ///\brief 取得当前任期
    inline uint64_t GetTerm(void)
    {
        return m_u64Term;
    }

    ///\brief 取得日志对象
    inline CRaftLog *GetLog(void)
    {
        return m_pRaftLog;
    }

protected:

    void SetHardState(const HardState &hs);
    void GetNodes(vector<uint32_t> &nodes);
    bool HasLeader(void);
    void GetSoftState(CSoftState &ss);
    void GetHardState(HardState &hs);

    ///\brief 取得法定人数
    int GetQuorum(void);

    ///\brief 发送Raft Peer之间的消息
    void SendMsg(Message *pMsg);

    ///\brief Follower发起读请求
    void SendReadReady(CReadState *pReadState);

    ///\brief Leader和代理写的Follower提交日志后，发起写操作和移动Apply日志序号的工作
    void SendWriteReady(CReadState *pReadState);

    ///\brief 非代理写的Follower提交日志后，发起Apply日志的工作
    void SendApplyReady(uint64_t u64ApplyTo);

    void SendAppend(uint32_t nToID);

    ///\brief 广播心跳
    void BcastHeartbeat(void);
    void BcastHeartbeatWithCtx(const string &strContext);
    void SendHeartbeat(uint32_t nToID, const string &strContext);

    ///\brief 角色变化时（除变为预选）重置相关属性
    ///\param u64Term 新的任期号
    void Reset(uint64_t u64Term);

    ///\brief Follower 处理消息
    ///\param msg 要处理的消息
    virtual void StepByFollower(const Message& msg);

    ///\brief Candidate 处理消息
    ///\param msg 要处理的消息
    virtual void StepByCandidate(const Message& msg);

    ///\brief Leader 处理消息
    ///\param msg 要处理的消息
    virtual void StepByLeader(const Message& msg);

    ///\brief 取得竞选类型对应的名称（一般用于调试）
    ///\param typeCampaign 竞选类型
    ///\return 对应竞选类型的名称
    const char* GetCampaignString(CampaignType typeCampaign);

    ///\brief 发起竞选
    ///\param typeCampaign 发起此次竞选的类型
    void Campaign(CampaignType typeCampaign);
    
    ///\brief Leader 尝试加大提交索引号
    ///\return 提交索引号变化标志 true 变大；false 没有变化
    ///\attention 当提交号变化，会调用CommitWrite向具体服务提交写操作，还会向Follower发送广播日志的消息
    bool MaybeCommit(void);

    ///\brief 发起提交日志后的写操作，以便具体应用到对应服务中
    ///\param u64Committed 已经提交的日志号
    void CommitWrite(uint64_t u64Committed);

    ///\brief Leader记录日志
    void AppendEntry(EntryVec &entries);

    ///\brief Follower接受日志消息
    void OnAppendEntries(const Message& msg);

    ///\brief Follower接受心跳消息
    void OnHeartbeat(const Message& msg);
    
    ///\brief Follower代理写操作
    void OnProxyMsgProp(const Message& msg);
    
    ///\brief Follower代理读操作
    void OnProxyMsgReadIndex(const Message& msg);

    ///\brief Follower处理Leader返回的读操作应答，启动项Client返回
    void OnMsgReadIndexResp(const Message &msg);

    ///\brief Follower处理Leader发出的立即超时消息，启动竞选
    void OnMsgTimeoutNow(const Message &msg);

    void OnSnapshot(const Message& msg);

    ///\brief Follower和Candidates处理定时器消息
    void OnTickElection(void);

    ///\brief Leader处理定时消息
    void OnTickHeartbeat(void);

    ///\brief 投票并且计算的的票数
    int  Poll(uint32_t nRaftID, MessageType typeMsg, bool bAccepted);

    Message* CloneMessage(const Message& msg);

    ///\brief 处理 Msg
    virtual void OnMsg(const Message& msg);

    ///\brief Follower处理 MsgHup
    virtual int OnMsgHup(const Message& msg);

    ///\brief 处理MsgVote和MsgPreVote
    virtual int OnMsgVote(const Message &msg);

    ///\brief Leader处理法定人数检查，可能会修改角色为Follower
    virtual void OnMsgCheckQuorum(const Message &msg);

    ///\brief Leader处理写请求
    virtual void OnMsgProp(const Message &msg);
    
    ///\brief Leader处理读请求
    virtual void OnMsgReadIndex(const Message &msg);
    
    ///\brief Leader处理涉及到组员的请求
    void OnMsgProgress(const Message &msg);
    
    ///\brief Leader处理组员的心跳应答 
    void OnHeartbeatResp(const Message& msg, CProgress *pProgress);
    
    ///\brief Leader处理组员的追加日志应答 
    void OnAppResp(const Message& msg, CProgress *pProgress);
    void OnMsgSnapStatus(const Message &msg, CProgress * pProgress);
    void OnMsgUnreachable(const Message &msg, CProgress * pProgress);
    
    ///\brief Leader处理组员的切换Leader的申请
    void OnMsgTransferLeader(const Message &msg, CProgress * pProgress);
    
    ///\brief Leader每选举Tick数，检查一下法定人数，假设，在此期间应该有正常通讯
    bool CheckQuorumActive(void);

    bool IsPromotable(void);
    bool Restore(const Snapshot& snapshot);

    void DelProgress(uint32_t id);
    void AddNode(uint32_t id);
    void RemoveNode(uint32_t id);

    bool PastElectionTimeout(void);
    void ResetRandomizedElectionTimeout(void);

    void ResetProgress(uint32_t nRaftID, uint64_t u64Match, uint64_t u64Next);
    void AbortLeaderTransfer();

    void SendTimeoutNow(uint32_t nToID);
    void ResetPendingConf(void);
public:
    uint32_t m_nVoteID;             ///< 当前投票目标的ID
    uint32_t m_nLeaderID;           ///< 当前领导者ID
    uint32_t m_nLeaderTransfereeID; ///< Leader 角色转移目标的ID
    EStateType m_stateRaft;         ///< 当前状态（领导者，追随者，竞选者，备选者） 
    uint64_t m_u64Term;             ///< 当前任期
    unordered_map<uint32_t, bool> m_mapVotes;         ///< 当前接受投票的状态
    unordered_map<uint32_t, CProgress*> m_mapProgress; ///< 各子节点状态

    CRaftQueue *m_pMsgQueue;         ///< Message缓冲区
    CRaftQueue *m_pIoQueue;          ///< 数据IO队列
    std::list<Message *> m_listWrite;///< Leader和代理Follower正在执行的写任务
    CReadOnly* m_pReadOnly;          ///< Leader 正在执行的读任务

    // New configuration is ignored if there exists unapplied configuration.
    bool m_bPendingConf;

    //ticks 是逻辑时钟单位，可以理解为每次timer到期就是一个tick

    ///\brief 对Leader和Candidate是上次选举超时之后的Tick数,对Follower是自上次选举超时或收到最后一个有效消息之后的Tick数
    int m_nTicksElectionElapsed;  

    ///\brief Leader上次心跳通讯之后的Tick数
    int m_nTicksHeartbeatElapsed; 

    ///\brief 为了减少冲突，随机生成的选举超时Tick数取值范围在[electiontimeout, 2 * electiontimeout - 1]之间
    int m_nTicksRandomizedElectionTimeout;

    CLogger* m_pLogger;     ///< 日志输入对象

    CRaftConfig *m_pConfig;  ///< 节点配置信息

    CRaftLog *m_pRaftLog;   ///< Raft算法的日志数据管理器
};


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
    enum ECampaignType
    {
        ///\brief 预选阶段，用下一任期号尝试预选，通过后才任期号加1开始正式选举
        ///\remark 防止当前节点的任期号无效增长
        ///\attention 当预选模式打开时生效
        campaignPreElection = 1,

        ///\brief 正式选举阶段，在指定时间内选举成功，或者超时
        ///\attention 如果当前配置支持预选，则这相当于选举的第二阶段
        campaignElection = 2,

        campaignTransfer = 3 ///< 由sa指定选举某个Raft，要求当前Leader在适当时机放弃领导者状态，支持该Raft选举
    };
public:
    ///\brief 构造函数
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
    ///\attention OnTick和Step是对外接口，需要考虑多线程安全问题,下面其他接口为偷懒模式下的单元测试接口
    virtual void OnTick(void);
    
    ///\brief 处理消息
    ///\param msg 消息体
    ///\return 错误号
    ///\attention OnTick和Step是对外接口，需要考虑多线程安全问题,下面其他接口为偷懒模式下的为单元测试接口
    virtual int  Step(const CMessage& msg);

    //下面的Public接口为了单元测试临时由protected改为public，代码重构完成采用单独测试派生类的方式改善

    ///\brief Leader记录日志
    ///\param entries 日志对象，自动添加任期号和日志索引号
    ///\attention 考虑到单节点模式，在函数最后尝试提交操作
    void AppendEntry(EntryVec &entries);
    
    ///\brief 转化为正式Follower状态
    void BecomeFollower(uint64_t u64Term, uint32_t nLeaderID);

    ///\brief 转化为正式选举状态
    void BecomeCandidate(void);
    
    ///\brief 转化为预选状态
    void BecomePreCandidate(void);

    ///\brief 转换为Leader状态
    ///\attention 防止误覆盖Follower日志，一旦转化为Leader，马上追加一条空日志
    void BecomeLeader(void);

    ///\brief 状态变化时（除变为预选）重置相关属性
    ///\param u64Term 新的任期号
    void Reset(uint64_t u64Term);

    ///\brief Leader向Follower广播日志
    void BcastAppend(void);

    ///\brief 取得当前状态机的状态
    inline EStateType GetState(void)
    {
        return m_stateRaft;
    }

    ///\brief 设置当前状态机的状态
    ///\param stateRaft 要设置的状态
    inline void SetState(EStateType stateRaft)
    {
        m_stateRaft = stateRaft;
    }

    ///\brief 取得当前Leader
    inline uint32_t GetLeader(void)
    {
        return m_nLeaderID;
    }

    ///\brief 取得当前LeaderTransferee
    inline uint32_t GetLeaderTransferee(void)
    {
        return m_nLeaderTransfereeID;
    }

    ///\brief 取得当前选举对象
    inline uint32_t GetVoted(void)
    {
        return m_nVoteID;
    }

    ///\brief 取得当前选举对象
    inline void SetVoted(uint32_t nVoteID)
    {
        m_nVoteID = nVoteID;
    }

    ///\brief 取得当前任期
    inline uint64_t GetTerm(void)
    {
        return m_u64Term;
    }

    ///\brief 设置当前任期
    ///\attention 单元测试专用
    inline void SetTerm(uint64_t u64Term)
    {
        m_u64Term = u64Term;
    }

    ///\brief 取得日志管理对象
    inline CRaftLog *GetLog(void)
    {
        return m_pRaftLog;
    }

    ///\brief 设置“硬”状态,即需要持久化的状态
    ///\param hs “硬”状态,即需要持久化的状态
    void SetHardState(const CHardState &hs);

    ///\brief 判断某节点是否投票
    ///\param nRaftID 节点ID
    ///\return 投票情况: 0 投赞成票；1 投反对票; 2 没有投票
    int CheckVoted(uint32_t nRaftID);

    void ReadMessages(vector<CMessage*> &msgs);

    void FreeMessages(vector<CMessage*> &msgs);

    ///\brief Follower和Candidates处理Tick定时器消息
    void OnTickElection(void);

    ///\brief Leader处理Tick定时消息
    ///\attention 如果在一个选举超时周期内没有和Follower有效通讯，则放弃Leader状态
    void OnTickHeartbeat(void);

    ///\brief 检查是否选举超时
    ///\attention 此时检查的是ResetRandomizedElectionTimeout随机生成选举超时
    ///\sa ResetRandomizedElectionTimeout
    bool PastElectionTimeout(void);

    ///\brief 随机算法重置选举计时器
    ///\attention 1~2倍的选举超时
    void ResetRandomizedElectionTimeout(void);

    ///\brief Follower接受日志消息 \n
    /// 1.消息的索引值小于当前节点已提交的值，则返回MsgAppResp消息类型，并返回已提交的位置 \n
    /// 2.如果消息追加成功，则返回MsgAppResp消息类型，并返回最后一条记录的索引值 \n
    /// 3.如果追加失败，则Reject设为true，并返回raftLog中最后一条记录的索引
    void OnAppendEntries(const CMessage& msg);

    ///\brief Follower接受心跳消息
    ///\param msgHeartbeat 心跳消息
    void OnHeartbeat(const CMessage& msgHeartbeat);

    bool IsPromotable(void);
    void DelProgress(uint32_t id);

    void AddNode(uint32_t id);
    void RemoveNode(uint32_t id);

    ///\brief Leader 尝试加大提交索引号
    ///\return 提交索引号变化标志 true 变大；false 没有变化
    ///\attention 当提交号变化，会调用CommitWrite向具体服务提交写操作，还会向Follower发送广播日志的消息
    bool MaybeCommit(void);

    void ResetProgress(uint32_t nRaftID, uint64_t u64Match, uint64_t u64Next);

    ///\brief 向指定节点发送日志
    ///\param nToID 目标节点ID
    void SendAppend(uint32_t nToID);

    ///\brief 取得节点ID列表
    ///\param nodes 节点ID列表
    void GetNodes(vector<uint32_t> &nodes);

    bool Restore(const CSnapshot& snapshot);
protected:
    
    bool HasLeader(void);
    void GetSoftState(CSoftState &ss);
    void GetHardState(CHardState &hs);

    ///\brief 取得法定人数，即有效节点数的1/2 + 1
    int GetQuorum(void);

    ///\brief 发送Raft Peer之间的消息
    void SendMsg(CMessage *pMsg);

    ///\brief Follower发起读请求
    void SendReadReady(CReadState *pReadState);

    ///\brief Leader和代理写的Follower提交日志后，发起写操作和移动Apply日志序号的工作
    void SendWriteReady(CReadState *pWriteState);

    ///\brief 非代理写的Follower提交日志后，发起Apply日志的工作
    void SendApplyReady(uint64_t u64ApplyTo);
    
    ///\brief Leader向所有Follower发送心跳信息
    ///\attention 顺便发送读队列中最后一个请求的唯一标识，在ReadOnlySafe时处理读请求
    void BcastHeartbeat(void);

    ///\brief Leader向所有Follower发送心跳信息
    ///\param strContext 顺便发送读队列中最后一个请求的唯一标识，在ReadOnlySafe时处理读请求
    void BcastHeartbeatWithCtx(const string &strContext);

    ///\brief Leader向一个Follower发送心跳信息
    ///\param nToID Follower ID
    ///\param strContext 顺便发送读队列中最后一个请求的唯一标识，在ReadOnlySafe时处理读请求
    ///\attention 顺便发送Leader提交日志号和已同步日志索引号的最小值，方便Follower提交
    void SendHeartbeat(uint32_t nToID, const string &strContext);

    ///\brief Follower 处理消息
    ///\param msg 要处理的消息
    virtual void StepByFollower(const CMessage& msg);

    ///\brief Candidate 处理消息
    ///\param msg 要处理的消息
    virtual void StepByCandidate(const CMessage& msg);

    ///\brief Leader 处理消息
    ///\param msg 要处理的消息
    virtual void StepByLeader(const CMessage& msg);

    ///\brief 取得竞选类型对应的名称（一般用于调试）
    ///\param typeCampaign 竞选类型
    ///\return 对应竞选类型的名称
    const char* GetCampaignString(ECampaignType typeCampaign);

    ///\brief 发起竞选
    ///\param typeCampaign 发起此次竞选的类型，预选，正式选举，sa指定
    void Campaign(ECampaignType typeCampaign);
    
    ///\brief 发起提交日志后的写操作，以便具体应用到对应服务中
    ///\param u64Committed 已经提交的日志号
    void CommitWrite(uint64_t u64Committed);
   
    ///\brief Follower代理写操作
    ///\param msgProp Propose 消息
    void OnProxyMsgProp(const CMessage& msgProp);
    
    ///\brief Follower代理读操作
    ///\param msgRead 读请求消息
    void OnProxyMsgReadIndex(const CMessage& msgRead);

    ///\brief Follower处理Leader返回的读操作应答，启动项Client返回
    ///\param msgReadResp 读请求的应答消息
    void OnMsgReadIndexResp(const CMessage &msgReadResp);

    ///\brief Follower处理Leader发出的立即超时消息，启动竞选
    ///\param msgTimeout 立即超时消息
    void OnMsgTimeoutNow(const CMessage &msgTimeout);

    void OnSnapshot(const CMessage& msg);

    ///\brief 投票并且计算的的票数
    ///\param nRaftID 投票人
    ///\param typeMsg 消息类型，是正式选举还是预选
    ///\param bAccepted 是否投赞成票
    ///\return 当前投赞成的票数
    int  Poll(uint32_t nRaftID, CMessage::EMessageType typeMsg, bool bAccepted);

    ///\brief 处理 Msg
    ///\param msg 要处理的消息
    ///\attention 此时已经完成msg的合法性检查
    virtual void OnMsg(const CMessage& msg);

    ///\brief Follower处理 MsgHup
    virtual int OnMsgHup(const CMessage& msg);

    ///\brief 处理MsgVote和MsgPreVote
    virtual int OnMsgVote(const CMessage &msg);

    ///\brief Leader处理法定人数检查，可能会修改状态为Follower
    ///\attention 如果和大部分Follower没有进行有效应答，则认为分裂，转为Follower
    virtual void OnMsgCheckQuorum(const CMessage &msg);

    ///\brief Leader处理写请求
    virtual void OnMsgProp(const CMessage &msg);
    
    ///\brief Leader处理读请求
    virtual void OnMsgReadIndex(const CMessage &msg);
    
    ///\brief Leader处理涉及到组员的请求
    void OnMsgProgress(const CMessage &msg);
    
    ///\brief Leader处理组员的心跳应答 
    void OnHeartbeatResp(const CMessage& msg, CProgress *pProgress);
    
    ///\brief Leader处理组员的追加日志应答 
    void OnAppResp(const CMessage& msg, CProgress *pProgress);
    void OnMsgSnapStatus(const CMessage &msg, CProgress * pProgress);
    void OnMsgUnreachable(const CMessage &msg, CProgress * pProgress);
    
    ///\brief Leader处理组员的切换Leader的申请
    void OnMsgTransferLeader(const CMessage &msg, CProgress * pProgress);
    
    ///\brief Leader每选举Tick数，检查一下法定人数，假设，在此期间应该有正常通讯
    bool CheckQuorumActive(void);

 
    ///\brief 终止Leader状态转移动作
    void AbortLeaderTransfer(void);
    
    ///\brief 日志追平后，立刻触发目标的选举流程
    ///\param nToID 选举目标
    void SendTimeoutNow(uint32_t nToID);
    
    void ResetPendingConf(void);
public:
    uint32_t m_nVoteID;             ///< 当前投票目标的ID
    uint32_t m_nLeaderID;           ///< 当前领导者ID
    uint32_t m_nLeaderTransfereeID; ///< Leader 状态转移目标的ID
    EStateType m_stateRaft;         ///< 当前状态（领导者，追随者，竞选者，备选者） 
    uint64_t m_u64Term;             ///< 当前任期
    unordered_map<uint32_t, bool> m_mapVotes;         ///< 当前接受投票的状态
    unordered_map<uint32_t, CProgress*> m_mapProgress; ///< 各子节点状态

    CRaftQueue *m_pMsgQueue;         ///< Message缓冲区
    CRaftQueue *m_pIoQueue;          ///< 数据IO队列
    std::list<CMessage *> m_listWrite;///< Leader和代理Follower正在执行的写任务
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


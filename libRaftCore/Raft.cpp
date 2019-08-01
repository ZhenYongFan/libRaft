#include "stdafx.h"
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include "Raft.h"
#include "RaftUtil.h"
#include "ReadOnly.h"

const static string kCampaignPreElection = "CampaignPreElection";
const static string kCampaignElection = "CampaignElection";
const static string kCampaignTransfer = "CampaignTransfer";
HardState kEmptyState;

CRaft::CRaft(CRaftConfig *pConfig, CRaftLog *pRaftLog, CRaftQueue *pQueue, CRaftQueue *pIoQueue, CLogger *pLogger)
    : m_pConfig(pConfig),
    m_u64Term(0),
    m_nVoteID(0),
    m_pRaftLog(pRaftLog),
    m_nLeaderID(None),
    m_nLeaderTransfereeID(None),
    m_pMsgQueue(pQueue), m_pIoQueue(pIoQueue),
    m_pReadOnly(new CReadOnly(pConfig->m_optionReadOnly, pLogger)),
    m_pLogger(pLogger)
{
    m_stateRaft = eStateFollower;
    m_bPendingConf = false;
    m_nTicksElectionElapsed = 0;
    m_nTicksHeartbeatElapsed = 0;
    m_nTicksRandomizedElectionTimeout = 1000;
}

CRaft::~CRaft(void)
{
    if (NULL != m_pReadOnly)
    {
        delete m_pReadOnly;
        m_pReadOnly = NULL;
    }
}

bool CRaft::Init(string &strErrMsg)
{
    HardState hs;
    ConfState cs;
    vector<uint32_t> peers;
    m_pConfig->GetPeers(peers);

    int err = m_pRaftLog->InitialState(hs, cs);
    if (!SUCCESS(err))
        m_pLogger->Fatalf(__FILE__, __LINE__, "storage InitialState fail: %s", GetErrorString(err));
    if (cs.nodes_size() > 0)
    {
        if (peers.size() > 0)
            m_pLogger->Fatalf(__FILE__, __LINE__, "cannot specify both newRaft(peers) and ConfState.Nodes)");
        peers.clear();
        for (int i = 0; i < int(cs.nodes_size()); ++i)
            peers.push_back(cs.nodes(i));
    }
    for (int i = 0; i < int(peers.size()); ++i)
        m_mapProgress[peers[i]] = new CProgress(1, m_pConfig->m_nMaxInfilght, m_pLogger);

    if (!isHardStateEqual(hs, kEmptyState))
        SetHardState(hs);

    //if (config.applied > 0)
    //    pRaftLog->AppliedTo(config.applied);

    BecomeFollower(m_u64Term, None);
    vector<string> peerStrs;
    char tmp[32];
    for (auto iter = m_mapProgress.begin(); iter != m_mapProgress.end(); ++iter)
    {
        snprintf(tmp, sizeof(tmp), "%u", iter->first);
        peerStrs.push_back(tmp);
    }
    string nodeStr = joinStrings(peerStrs, ",");

    m_pLogger->Infof(__FILE__, __LINE__,
        "newRaft %lu [peers: [%s], term: %llu, commit: %llu, applied: %llu, lastindex: %llu, lastterm: %llu]",
        m_pConfig->m_nRaftID, nodeStr.c_str(), m_u64Term, 
        m_pRaftLog->GetCommitted(), m_pRaftLog->GetApplied(),
        m_pRaftLog->GetLastIndex(), m_pRaftLog->GetLastTerm());
    return true;
}

void CRaft::Uninit(void)
{
    //释放对应节点的信息
    for (auto iter :m_mapProgress)
        delete iter.second;
    m_mapProgress.clear();
    //释放写请求
    for (auto pMsg : m_listWrite)
        delete pMsg;
    m_listWrite.clear();
}

void CRaft::GetSoftState(CSoftState &state)
{
    state.m_nLeaderID = m_nLeaderID;
    state.m_stateRaft = m_stateRaft;
}

void CRaft::GetHardState(HardState &stateHard)
{
    stateHard.set_term(m_u64Term);
    stateHard.set_vote(m_nVoteID);
    stateHard.set_commit(m_pRaftLog->GetCommitted());
}

void CRaft::GetNodes(vector<uint32_t> &nodeIDs)
{
    nodeIDs.clear();
    for (auto iter :m_mapProgress)
        nodeIDs.push_back(iter.first);
}

void CRaft::SetHardState(const HardState &stateHard)
{
    if (stateHard.commit() < m_pRaftLog->GetCommitted() || stateHard.commit() > m_pRaftLog->GetLastIndex())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__,
            "%x state.commit %llu is out of range [%llu, %llu]", m_pConfig->m_nRaftID, stateHard.commit(), m_pRaftLog->GetCommitted(), m_pRaftLog->GetLastIndex());
    }

    m_pRaftLog->CommitTo(stateHard.commit());
    m_u64Term = stateHard.term();
    m_nVoteID = stateHard.vote();
}

void CRaft::OnTick(void)
{
    switch (m_stateRaft)
    {
    case eStateFollower:
    case eStateCandidate:
    case eStatePreCandidate:
        OnTickElection();
        break;
    case eStateLeader:
        OnTickHeartbeat();
        break;
    default:
        m_pLogger->Fatalf(__FILE__, __LINE__, "supported state %d", m_stateRaft);
        break;
    }
}

Message* CRaft::CloneMessage(const Message& msg)
{
    return new Message(msg);
}

// CheckQuorumActive returns true if the quorum is active from
// the view of the local raft state machine. Otherwise, it returns false.
// CheckQuorumActive also resets all RecentActive to false.
bool CRaft::CheckQuorumActive(void)
{
    int nActiveNum = 0;
    for (auto iter: m_mapProgress)
    {
        if (iter.first == m_pConfig->m_nRaftID) // self is always active
            nActiveNum ++;
        else if (iter.second->m_bRecentActive)
        {
            nActiveNum ++;
            iter.second->m_bRecentActive = false;
        }
    }
    return nActiveNum >= GetQuorum();
}

bool CRaft::HasLeader(void)
{
    return m_nLeaderID != None;
}

int CRaft::GetQuorum(void)
{
    return int(m_mapProgress.size() / 2) + 1;
}

// send persists state to stable storage and then sends to its mailbox.
void CRaft::SendMsg(Message *pMsg)
{
    pMsg->set_from(m_pConfig->m_nRaftID);
    MessageType typeMsg = pMsg->type();

    if (typeMsg == MsgVote || typeMsg == MsgVoteResp || typeMsg == MsgPreVote || typeMsg == MsgPreVoteResp)
    {
        if (pMsg->term() == 0)
        {
            // All {pre-,}campaign messages need to have the term set when
            // sending.
            // - MsgVote: m.Term is the term the node is campaigning for,
            //   non-zero as we increment the term when campaigning.
            // - MsgVoteResp: m.Term is the new r.Term if the MsgVote was
            //   granted, non-zero for the same reason MsgVote is
            // - MsgPreVote: m.Term is the term the node will campaign,
            //   non-zero as we use m.Term to indicate the next term we'll be
            //   campaigning for
            // - MsgPreVoteResp: m.Term is the term received in the original
            //   MsgPreVote if the pre-vote was granted, non-zero for the
            //   same reasons MsgPreVote is
            m_pLogger->Fatalf(__FILE__, __LINE__, "term should be set when sending %s", CRaftUtil::MsgType2String(typeMsg));
        }
    }
    else
    {
        if (pMsg->term() != 0)
        {
            m_pLogger->Fatalf(__FILE__, __LINE__, "term should not be set when sending %s (was %llu)", CRaftUtil::MsgType2String(typeMsg), pMsg->term());
        }
        // do not attach term to MsgProp, MsgReadIndex
        // proposals are a way to forward to the leader and
        // should be treated as local message.
        // MsgReadIndex is also forwarded to leader.
        if (typeMsg != MsgProp && typeMsg != MsgReadIndex)
            pMsg->set_term(m_u64Term);
    }
    m_pMsgQueue->Push(pMsg);
}

void CRaft::SendReadReady(CReadState *pReadState)
{
    CLogOperation *pOperation = new CLogOperation;
    pOperation->m_nType = 0;
    pOperation->m_pOperation = pReadState;
    m_pIoQueue->Push(pOperation);
}

void CRaft::SendWriteReady(CReadState *pReadState)
{
    CLogOperation *pOperation = new CLogOperation;
    pOperation->m_nType = 1;
    pOperation->m_pOperation = pReadState;
    m_pIoQueue->Push(pOperation);
}

void CRaft::SendApplyReady(uint64_t u64ApplyTo)
{
    CLogOperation *pOperation = new CLogOperation;
    pOperation->m_nType = 2;
    pOperation->m_u64ApplyTo = u64ApplyTo;
    m_pIoQueue->Push(pOperation);
}

// BcastAppend sends RPC, with entries to all peers that are not up-to-date
// according to the progress recorded in r.prs.
void CRaft::BcastAppend(void)
{
    for (auto iter = m_mapProgress.begin(); iter != m_mapProgress.end(); ++iter)
    {
        if (iter->first != m_pConfig->m_nRaftID)
            SendAppend(iter->first);
    }
}

// SendAppend sends RPC, with entries to the given peer.
void CRaft::SendAppend(uint32_t nToID)
{
    //获取目标节点的Progress
    CProgress *pProgress = m_mapProgress[nToID];
    //检测当前节点是否可以向目标节点发送消息   
	if (pProgress == NULL || pProgress->IsPaused())
    {
        m_pLogger->Infof(__FILE__, __LINE__, "node %x paused", nToID);
        return;
    }
    //创建待发送的消息
    Message *pMsg = new Message();
    pMsg->set_to(nToID);

    //获取Next索引对应的记录的Term值
    uint64_t u64Term;
    int nErrorTerm = m_pRaftLog->GetTerm(pProgress->m_u64NextLogIndex - 1, u64Term);
    //获取需要发送的Entry记录
    EntryVec entries;
    int nErrorEntries = m_pRaftLog->GetEntries(pProgress->m_u64NextLogIndex, m_pConfig->m_nMaxMsgSize, entries);
    if (!SUCCESS(nErrorTerm) || !SUCCESS(nErrorEntries))
    {
        //1.上述两次raftLog查找出现异常时(获取不到需要发送的Entry记录)，就会形成MsgSnap消息，将快照数据发送到指定节点。
        //2.向该节点发送MsgSnap类型的消息
        //3.将目标Follower节点对应的Progress切换成ProgressStateSnapshot状态
        if (!pProgress->m_bRecentActive)
        {
            //如果该节点已经不存活，则退出(Recent Active为true表示该节点存活）
            m_pLogger->Debugf(__FILE__, __LINE__, "ignore sending snapshot to %llu since it is not recently active", nToID);
            delete pMsg;
            return;
        }

        pMsg->set_type(MsgSnap);
        Snapshot *snapshot;
        int err = m_pRaftLog->snapshot(&snapshot);
        if (!SUCCESS(err))
        {
            if (err == ErrSnapshotTemporarilyUnavailable)
            {
                delete pMsg;
                m_pLogger->Debugf(__FILE__, __LINE__, "%llu failed to send snapshot to %llu because snapshot is temporarily unavailable", m_pConfig->m_nRaftID, nToID);
                return;
            }
            m_pLogger->Fatalf(__FILE__, __LINE__, "get snapshot err: %s", GetErrorString(err));
        }

        if (isEmptySnapshot(snapshot))
            m_pLogger->Fatalf(__FILE__, __LINE__, "need non-empty snapshot");

        Snapshot *s = pMsg->mutable_snapshot();
        s->CopyFrom(*snapshot);
        uint64_t sindex = snapshot->metadata().index();
        uint64_t sterm = snapshot->metadata().term();
        m_pLogger->Debugf(__FILE__, __LINE__, "%x [firstindex: %llu, commit: %llu] sent snapshot[index: %llu, term: %llu] to %x [%s]",
            m_pConfig->m_nRaftID, m_pRaftLog->GetFirstIndex(), m_pRaftLog->GetCommitted(), sindex, sterm, nToID, pProgress->GetInfoText().c_str());
        pProgress->BecomeSnapshot(sindex);
        m_pLogger->Debugf(__FILE__, __LINE__, "%x paused sending replication messages to %x [%s]", m_pConfig->m_nRaftID, nToID, pProgress->GetInfoText().c_str());
    }
    else
    {
        pMsg->set_type(MsgApp);
        pMsg->set_index(pProgress->m_u64NextLogIndex - 1);   //新日志条目之前的日志索引值
        pMsg->set_logterm(u64Term);                          //新日志条目之前的Term值
        pMsg->set_commit(m_pRaftLog->GetCommitted());        //Leader已经提交的日志的索引值

        for (size_t i = 0; i < entries.size(); ++i)
        {
            Entry *entry = pMsg->add_entries();
            entry->CopyFrom(entries[i]);
        }

        if (entries.size() > 0)
        {
            uint64_t last;
            switch (pProgress->state_)
            {
             // optimistically increase the next when in ProgressStateReplicate
            case ProgressStateReplicate:
                last = entries[entries.size() - 1].index();
                pProgress->optimisticUpdate(last);
                pProgress->ins_.add(last);
                break;
            case ProgressStateProbe:
                pProgress->Pause();
                break;
            default:
                m_pLogger->Fatalf(__FILE__, __LINE__, "%x is sending append in unhandled state %s", m_pConfig->m_nRaftID, pProgress->stateString());
                break;
            }
        }
    }
    SendMsg(pMsg);
}

// BcastHeartbeat sends RPC, without entries to all the peers.
void CRaft::BcastHeartbeat(void)
{
    string strContext = m_pReadOnly->LastPendingRequestCtx();
    BcastHeartbeatWithCtx(strContext);
}

void CRaft::BcastHeartbeatWithCtx(const string &strContext)
{
    for (auto iter = m_mapProgress.begin(); iter != m_mapProgress.end(); ++iter)
    {
        //无需向自己发送心跳消息
        if (iter->first != m_pConfig->m_nRaftID) 
            SendHeartbeat(iter->first, strContext);
    }
}

// SendHeartbeat sends an empty MsgApp
void CRaft::SendHeartbeat(uint32_t nToID, const string &strContext)
{
    // Attach the commit as min(to.matched, r.committed).
    // When the leader sends out heartbeat message,
    // the receiver(follower) might not be matched with the leader
    // or it might not have all the committed entries.
    // The leader MUST NOT forward the follower's commit to
    // an unmatched index.
    uint64_t u64Committed = min(m_mapProgress[nToID]->m_u64MatchLogIndex, m_pRaftLog->GetCommitted());
    Message *pMsg = new Message();
    pMsg->set_to(nToID);
    pMsg->set_type(MsgHeartbeat);
    pMsg->set_commit(u64Committed);
    pMsg->set_context(strContext);
    SendMsg(pMsg);
}

///\brief 逆序比较器,核心是用对象自带的小于号操作符
template <typename T>
class CReverseCompartor
{
public:
    ///\brief 用对象自带的小于号操作符进行比较
    bool operator()(const T &objLeft, const T &objRight)
    {
        return objRight < objLeft;
    }
};

// MaybeCommit attempts to advance the commit index. Returns true if
// the commit index changed (in which case the caller should call
// r.BcastAppend).
bool CRaft::MaybeCommit(void)
{
    vector<uint64_t> mis;
    for (auto iter :m_mapProgress)
        mis.push_back(iter.second->m_u64MatchLogIndex);
    sort(mis.begin(), mis.end(), CReverseCompartor<uint64_t>());
    uint64_t u64Index = mis[GetQuorum() - 1];
    bool bCommitted = m_pRaftLog->MaybeCommit(u64Index, m_u64Term);
    if (bCommitted)
        CommitWrite(u64Index);
    return bCommitted;
}

void CRaft::CommitWrite(uint64_t u64Committed)
{
    if (!m_listWrite.empty())
    {
        auto iter = m_listWrite.begin();
        for (; iter != m_listWrite.end(); iter++)
        {
            Message *pMsg = *iter;
            Entry *pEntry = pMsg->mutable_entries(0);
            if (pEntry->index() <= u64Committed)
            {
                if (pMsg->from() == None || pMsg->from() == m_pConfig->m_nRaftID)
                    SendWriteReady(new CReadState(pEntry->index(), pMsg->entries(0).data()));
                delete pMsg;
            }
            else
                break;
        }
        m_listWrite.erase(m_listWrite.begin(), iter);
    }
    else
        SendApplyReady(u64Committed);
}

/*
1.将raft的Term设置为term，Vote置为空
2.清空leader字段
3.清零选举计时器和心跳计时器，随机算法重置选举计时器electionElapsed
4.重置votes字段，votes是收到的选票
5.重置prs和learners
6.重置pendingConfIndex
7.重置readOnly 只读请求相关的设置
*/
void CRaft::Reset(uint64_t u64Term)
{
    if (m_u64Term != u64Term)
    {
        m_u64Term = u64Term;
        m_nVoteID = None;
    }
    m_nLeaderID = None;

    m_nTicksElectionElapsed = 0;
    m_nTicksHeartbeatElapsed = 0;
    ResetRandomizedElectionTimeout();

    AbortLeaderTransfer();
    m_mapVotes.clear();
    
    for (auto &iter : m_mapProgress)
    {
        uint32_t nRaftID = iter.first;
        CProgress *pProgress = iter.second;
        delete pProgress;
        pProgress = new CProgress(m_pRaftLog->GetLastIndex() + 1, m_pConfig->m_nMaxInfilght, m_pLogger);
        iter.second = pProgress;
        if (nRaftID == m_pConfig->m_nRaftID)
            pProgress->m_u64MatchLogIndex = m_pRaftLog->GetLastIndex();
    }

    m_bPendingConf = false;

    ReadOnlyOption optMode = m_pReadOnly->m_optMode;
    delete m_pReadOnly;
    m_pReadOnly = new CReadOnly(optMode, m_pLogger);

    for (auto pMsg : m_listWrite)
        delete pMsg;
    m_listWrite.clear();
}

void CRaft::AppendEntry(EntryVec &entries)
{
    uint64_t u64Index = m_pRaftLog->GetLastIndex();
    m_pLogger->Debugf(__FILE__, __LINE__, "lastIndex:%llu", u64Index);
    for (size_t nIndex = 0; nIndex < entries.size(); ++nIndex)
    {
        entries[nIndex].set_term(m_u64Term);
        entries[nIndex].set_index(u64Index + 1 + nIndex);
    }
    m_pRaftLog->Append(entries);
    m_mapProgress[m_pConfig->m_nRaftID]->MaybeUpdate(m_pRaftLog->GetLastIndex());
    // Regardless of MaybeCommit's return, our caller will call BcastAppend.
    //此次考虑到单节点的场景
    MaybeCommit();
}

//由Follower和Candidate递增计数器，如果超时则发送MsgHup消息
void CRaft::OnTickElection(void)
{
    m_nTicksElectionElapsed++;
    if (IsPromotable() && PastElectionTimeout())
    {
        m_nTicksElectionElapsed = 0;
        Message msg;
        msg.set_from(m_pConfig->m_nRaftID);
        msg.set_type(MsgHup);
        Step(msg);
    }
}

// OnTickHeartbeat is run by leaders to send a MsgBeat after r.heartbeatTimeout.
void CRaft::OnTickHeartbeat(void)
{
    m_nTicksHeartbeatElapsed++; //递增心跳计时器
    m_nTicksElectionElapsed++;  //递增选举计时器

    if (m_nTicksElectionElapsed >= m_pConfig->m_nTicksElection)
    {
        m_nTicksElectionElapsed = 0;  //重置选举计时器，Leader节点不会主动发起选举
        if (m_pConfig->m_bCheckQuorum) //检测本选举周期内当前Leader节点是否与集群中的大多数节点连通
        {
            Message msg;
            msg.set_from(m_pConfig->m_nRaftID);
            msg.set_type(MsgCheckQuorum);
            Step(msg);//执行后可能角色转换为Follower
        }
        // If current leader cannot transfer leadership in electionTimeout, it becomes leader again.
        if (m_stateRaft == eStateLeader && m_nLeaderTransfereeID != None)
            AbortLeaderTransfer(); //放弃Leader转移
    }
    //检测当前节点是否是Leader，不是Leader则直接返回
    if (m_stateRaft != eStateLeader)
        return;

    //心跳计时器超时，则发生MsgBeat消息，继续维持向其余节点发送心跳
    if (m_nTicksHeartbeatElapsed >= m_pConfig->m_nTicksHeartbeat)
    {
        m_nTicksHeartbeatElapsed = 0;  //重置心跳计时器
        Message msg;
        msg.set_from(m_pConfig->m_nRaftID);
        msg.set_type(MsgBeat);
        Step(msg); //发生心跳信息
    }
}

// promotable indicates whether state machine can be promoted to leader,
// which is true when its own id is in progress list.
bool CRaft::IsPromotable(void)
{
    return m_mapProgress.find(m_pConfig->m_nRaftID) != m_mapProgress.end();
}

// PastElectionTimeout returns true iff r.electionElapsed is greater
// than or equal to the randomized election timeout in
// [electiontimeout, 2 * electiontimeout - 1].
bool CRaft::PastElectionTimeout(void)
{
    return m_nTicksElectionElapsed >= m_nTicksRandomizedElectionTimeout;
}

//随机算法重置选举计时器
void CRaft::ResetRandomizedElectionTimeout(void)
{
    m_nTicksRandomizedElectionTimeout = m_pConfig->m_nTicksElection + rand() % m_pConfig->m_nTicksElection;
}

void CRaft::BecomeFollower(uint64_t u64Term, uint32_t nLeaderID)
{
    Reset(u64Term);
    m_nLeaderID = nLeaderID;
    m_stateRaft = eStateFollower;
    m_pLogger->Infof(__FILE__, __LINE__, "%x became follower at term %lu", m_pConfig->m_nRaftID, m_u64Term);
}

void CRaft::BecomeCandidate(void)
{
    if (m_stateRaft == eStateLeader)
        m_pLogger->Fatalf(__FILE__, __LINE__, "invalid transition [leader -> candidate]");

    Reset(m_u64Term + 1);
    m_nVoteID = m_pConfig->m_nRaftID;
    m_stateRaft = eStateCandidate;
    m_pLogger->Infof(__FILE__, __LINE__, "%x became candidate at term %llu", m_pConfig->m_nRaftID, m_u64Term);
}

void CRaft::BecomePreCandidate(void)
{
    // TODO(xiangli) remove the panic when the raft implementation is stable
    if (m_stateRaft == eStateLeader)
        m_pLogger->Fatalf(__FILE__, __LINE__, "invalid transition [leader -> pre-candidate]");
    // Becoming a pre-candidate changes our step functions and state,
    // but doesn't change anything else. In particular it does not increase
    // r.Term or change r.Vote.
    m_stateRaft = eStatePreCandidate;
    m_pLogger->Infof(__FILE__, __LINE__, "%x became pre-candidate at term %llu", m_pConfig->m_nRaftID, m_u64Term);
}

void CRaft::BecomeLeader(void)
{
    if (m_stateRaft == eStateFollower)
        m_pLogger->Fatalf(__FILE__, __LINE__, "invalid transition [follower -> leader]");

    Reset(m_u64Term);
    m_nLeaderID = m_pConfig->m_nRaftID;
    m_stateRaft = eStateLeader;

    EntryVec entries;
    int err = m_pRaftLog->GetEntries(m_pRaftLog->GetCommitted() + 1, noLimit, entries);
    if (!SUCCESS(err))
        m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected error getting uncommitted entries (%s)", GetErrorString(err));

    int nNum = GetNumOfPendingConf(entries);
    if (nNum > 1)
        m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected multiple uncommitted config entry");
    else if (nNum == 1)
        m_bPendingConf = true;
    
    //追加1条空日志
    entries.clear();
    entries.push_back(Entry());
    AppendEntry(entries);
    m_pLogger->Infof(__FILE__, __LINE__, "%x became leader at term %llu", m_pConfig->m_nRaftID, m_u64Term);
}

const char* CRaft::GetCampaignString(CampaignType typeCampaign)
{
    const char* pstrCampaigns[3] = {
        "campaignPreElection",
        "campaignElection",
        "campaignTransfer"
    };
    const char* pstrCampaign = "unknown campaign type";
    if (typeCampaign >= campaignPreElection && typeCampaign <= campaignTransfer)
        pstrCampaign = pstrCampaigns[typeCampaign - campaignPreElection];
    return pstrCampaign;
}

void CRaft::Campaign(CampaignType typeCampaign)
{
    uint64_t u64Term;
    MessageType voteMsg;
    if (typeCampaign == campaignPreElection)
    {
        //将当前节点切换成PreCandidate状态，不增加Term值,防止离群节点Term值无效增加干扰集群的问题
        BecomePreCandidate();
        //预选RPCs发送的是下一个Term编号，而自己的真实Term并不增加
        u64Term = m_u64Term + 1;
        voteMsg = MsgPreVote;
    }
    else
    {
        //将当前节点切换成Candidate状态，该方法会增加Term值
        BecomeCandidate();
        u64Term = m_u64Term;
        voteMsg = MsgVote;
    }

    //选自己一票，并且统计当前节点收到同意的选票张数
    //为单节点模式优化
    //如统计其得票数是否超过半数，则切换到正式选举或者转换为Leader    
    if (GetQuorum() == Poll(m_pConfig->m_nRaftID, VoteRespMsgType(voteMsg), true))
    {
        // We won the election after voting for ourselves (which must mean that
        // this is a single-node cluster). Advance to the next state.
        if (typeCampaign == campaignPreElection)
            Campaign(campaignElection);
        else
            BecomeLeader();
        return;
    }
    //在进行Leader节点选举时，MsgPreVote或MsgVote消息会在Context字段中设置该特殊标识    
    string strContext;
    if (typeCampaign == campaignTransfer)
        strContext = kCampaignTransfer;
    
    for (auto iter : m_mapProgress)
    {
        uint32_t nRaftID = iter.first;
        //注：上面Poll方法中已经为当前节点投赞成票
        if (m_pConfig->m_nRaftID != nRaftID)
        {
            m_pLogger->Infof(__FILE__, __LINE__, "%x [logterm: %llu, index: %llu] sent %s request to %x at term %llu",
                m_pConfig->m_nRaftID, m_pRaftLog->GetLastTerm(), m_pRaftLog->GetLastIndex(), GetCampaignString(typeCampaign), nRaftID, m_u64Term);

            Message *pMsg = new Message();
            pMsg->set_term(u64Term);                      //候选人的任期号,如果是预选就是下一任编号
            pMsg->set_to(nRaftID);                        //目标
            pMsg->set_type(voteMsg);                      //请选（或预选）我
            pMsg->set_index(m_pRaftLog->GetLastIndex());  //候选人最新日志条目的索引号
            pMsg->set_logterm(m_pRaftLog->GetLastTerm()); //候选人最新日志条目对应的任期号
            pMsg->set_context(strContext);
            SendMsg(pMsg);
        }
    }
}

int CRaft::Poll(uint32_t nRaftID, MessageType typeMsg, bool bAccepted)
{
    if (bAccepted)
        m_pLogger->Infof(__FILE__, __LINE__, "%x received %s from %x at term %llu", m_pConfig->m_nRaftID, CRaftUtil::MsgType2String(typeMsg), nRaftID, m_u64Term);
    else
        m_pLogger->Infof(__FILE__, __LINE__, "%x received %s rejection from %x at term %llu", m_pConfig->m_nRaftID, CRaftUtil::MsgType2String(typeMsg), nRaftID, m_u64Term);

    if (m_mapVotes.find(nRaftID) == m_mapVotes.end())
        m_mapVotes[nRaftID] = bAccepted;

    int nGranted = 0;
    for (auto iter: m_mapVotes)
    {
        if (iter.second)
            nGranted++;
    }
    return nGranted;
}

int CRaft::Step(const Message& msg)
{
    m_pLogger->Debugf(__FILE__, __LINE__, "msg %s %llu -> %llu, term:%llu",
        CRaftUtil::MsgType2String(msg.type()), msg.from(), msg.to(), m_u64Term);
    // Handle the message term, which may result in our stepping down to a follower.
    Message *respMsg;
    uint64_t u64Term = msg.term();
    int typeMsg = msg.type();
    uint32_t nFromID = msg.from();

    if (u64Term == 0)
    {
        //local message
    }
    else if (u64Term > m_u64Term)
    {
        //当消息中Term大于本地Term说明是新一轮选举
        uint32_t leader = nFromID;
        if (typeMsg == MsgVote || typeMsg == MsgPreVote)
        {
            bool bForce = (msg.context() == kCampaignTransfer);
            bool inLease = (m_pConfig->m_bCheckQuorum 
                && m_nLeaderID != None 
                && m_nTicksElectionElapsed < m_pConfig->m_nTicksElection);
            if (!bForce && inLease)
            {
                // If a server receives a RequestVote request within the minimum election timeout
                // of hearing from a current leader, it does not update its term or grant its vote
                m_pLogger->Infof(__FILE__, __LINE__, "%x [logterm: %llu, index: %llu, vote: %x] ignored %s from %x [logterm: %llu, index: %llu] at term %llu: lease is not expired (remaining ticks: %d)",
                    m_pConfig->m_nRaftID, m_pRaftLog->GetLastTerm(), m_pRaftLog->GetLastIndex(), m_nVoteID, CRaftUtil::MsgType2String(typeMsg), nFromID,
                    msg.logterm(), msg.index(), u64Term, m_pConfig->m_nTicksElection - m_nTicksElectionElapsed);
                return OK;
            }
            leader = None;
        }
        if (typeMsg == MsgPreVote)
        {
            // Never change our term in response to a PreVote
        }
        else if (typeMsg == MsgPreVoteResp && !msg.reject())
        {
            // We send pre-vote requests with a term in our future. If the
            // pre-vote is granted, we will increment our term when we get a
            // quorum. If it is not, the term comes from the node that
            // rejected our vote so we should become a follower at the new
            // term.
        }
        else
        {
            m_pLogger->Infof(__FILE__, __LINE__, "%x [term: %llu] received a %s message with higher term from %x [term: %llu]",
                m_pConfig->m_nRaftID, m_u64Term, CRaftUtil::MsgType2String(typeMsg), nFromID, u64Term);
            if (typeMsg == MsgApp || typeMsg == MsgHeartbeat || typeMsg == MsgSnap)
                BecomeFollower(u64Term, nFromID);
            else
                BecomeFollower(u64Term, None);
        }
    }
    else if (u64Term < m_u64Term)
    {
        if (m_pConfig->m_bCheckQuorum && (typeMsg == MsgHeartbeat || typeMsg == MsgApp))
        {
            // We have received messages from a leader at a lower term. It is possible
            // that these messages were simply delayed in the network, but this could
            // also mean that this node has advanced its term number during a network
            // partition, and it is now unable to either win an election or to rejoin
            // the majority on the old term. If checkQuorum is false, this will be
            // handled by incrementing term numbers in response to MsgVote with a
            // higher term, but if checkQuorum is true we may not advance the term on
            // MsgVote and must generate other messages to advance the term. The net
            // result of these two features is to minimize the disruption caused by
            // nodes that have been removed from the cluster's configuration: a
            // removed node will send MsgVotes (or MsgPreVotes) which will be ignored,
            // but it will not receive MsgApp or MsgHeartbeat, so it will not create
            // disruptive term increases
            respMsg = new Message();
            respMsg->set_to(nFromID);
            respMsg->set_type(MsgAppResp);
            SendMsg(respMsg);
        }
        else
        {
            // ignore other cases
            m_pLogger->Infof(__FILE__, __LINE__, "%x [term: %llu] ignored a %s message with lower term from %x [term: %llu]",
                m_pConfig->m_nRaftID, m_u64Term, CRaftUtil::MsgType2String(typeMsg), nFromID, u64Term);
        }
        return OK;
    }

    OnMsg(msg);
    return OK;
}

void CRaft::OnMsg(const Message& msg)
{
    MessageType typeMsg = msg.type();
    if(MsgHup == typeMsg)
        OnMsgHup(msg);
    else if(MsgVote == typeMsg || MsgPreVote == typeMsg)
        OnMsgVote(msg);
    else
    {
        if (eStateFollower == m_stateRaft)
            StepByFollower(msg);
        else if ((eStatePreCandidate == m_stateRaft) || eStateCandidate == m_stateRaft)
            StepByCandidate(msg);
        else if (eStateLeader == m_stateRaft)
            StepByLeader(msg);
        else
            assert(false);
    }
}

//选举计数器超时，Follower发送Hup消息，开启预选或者选举操作，向集群中其他节点发送MsgPreVote或者MsgVote消息
int CRaft::OnMsgHup(const Message& msg)
{
    if (m_stateRaft != eStateLeader)
    {
        //获取raftLog中已提交但未应用的Entry记录
        EntryVec entries;
        int err = m_pRaftLog->GetSliceEntries(m_pRaftLog->GetApplied() + 1, m_pRaftLog->GetCommitted() + 1, noLimit, entries);
        if (!SUCCESS(err))
            m_pLogger->Fatalf(__FILE__, __LINE__, "unexpected error getting unapplied entries (%s)", GetErrorString(err));
        //检测是否有未应用的EntryConfChange记录，如果有就放弃发起选举的机会       
        int nPendingConf = GetNumOfPendingConf(entries);
        if (nPendingConf != 0 && m_pRaftLog->GetCommitted() > m_pRaftLog->GetApplied())
        {
            m_pLogger->Warningf(__FILE__, __LINE__, "%x cannot campaign at term %llu since there are still %llu pending configuration changes to apply",
                m_pConfig->m_nRaftID, m_u64Term, nPendingConf);
            return OK;
        }

        m_pLogger->Infof(__FILE__, __LINE__, "%x is starting a new election at term %llu", m_pConfig->m_nRaftID, m_u64Term);
        //检测当前集群是否开启了PreVote模式，如果开启了则发起预选（PreElection），没有开启则发起选举（Election）
        if (m_pConfig->m_bPreVote)
            Campaign(campaignPreElection);
        else
            Campaign(campaignElection);
    }
    else //如果当前节点是Leader，则忽略MsgHup类型的消息       
        m_pLogger->Debugf(__FILE__, __LINE__, "%x ignoring MsgHup because already leader", m_pConfig->m_nRaftID);
    return OK;
}

int CRaft::OnMsgVote(const Message &msg)
{
    Message * respMsg;
    uint64_t term = msg.term();
    MessageType type = msg.type();
    uint32_t from = msg.from();
    // The m.Term > r.Term clause is for MsgPreVote. For MsgVote m.Term should always equal r.Term.
    bool bCanVote = (m_nVoteID == None || term > m_u64Term || m_nVoteID == from);
    if (bCanVote && m_pRaftLog->IsUpToDate(msg.index(), msg.logterm()))
    {
        m_pLogger->Infof(__FILE__, __LINE__, "%x [logterm: %llu, index: %llu, vote: %x] cast %s for %x [logterm: %llu, index: %llu] at term %llu",
            m_pConfig->m_nRaftID, m_pRaftLog->GetLastTerm(), m_pRaftLog->GetLastIndex(), m_nVoteID, CRaftUtil::MsgType2String(type), from, msg.logterm(), msg.index(), m_u64Term);
        respMsg = new Message();
        respMsg->set_to(from);
        respMsg->set_term(term);
        respMsg->set_type(VoteRespMsgType(type));
        SendMsg(respMsg);
        if (type == MsgVote)
        {
            m_nTicksElectionElapsed = 0;
            m_nVoteID = from;
        }
    }
    else
    {
        m_pLogger->Infof(__FILE__, __LINE__,
            "%x [logterm: %llu, index: %llu, vote: %x] rejected %s from %x [logterm: %llu, index: %llu] at term %llu",
            m_pConfig->m_nRaftID, m_pRaftLog->GetLastTerm(), m_pRaftLog->GetLastIndex(), m_nVoteID, CRaftUtil::MsgType2String(type), from, msg.logterm(), msg.index(), m_u64Term);
        respMsg = new Message();
        respMsg->set_to(from);
        respMsg->set_term(m_u64Term);
        respMsg->set_type(VoteRespMsgType(type));
        respMsg->set_reject(true);
        SendMsg(respMsg);
    }
    return OK;
}

void CRaft::StepByLeader(const Message& msg)
{
    switch (msg.type())
    {
    case MsgBeat: //本地消息
        BcastHeartbeat();
        break;
    case MsgCheckQuorum: //本地消息
        OnMsgCheckQuorum(msg);
        break;
    case MsgProp:
        OnMsgProp(msg);
        break;
    case MsgReadIndex:
        OnMsgReadIndex(msg);
        break;
    default:
        OnMsgProgress(msg);
        break;
    }
}

// All other message types require a progress for m.From (pr).
void CRaft::OnMsgProgress(const Message &msg)
{
    auto iter = m_mapProgress.find(msg.from());
    if (iter != m_mapProgress.end())
    {
        CProgress *pProgress = iter->second;
        switch (msg.type())
        {
        case MsgAppResp:
            OnAppResp(msg, pProgress);
            break;
        case MsgHeartbeatResp:
            OnHeartbeatResp(msg, pProgress);
            break;
        case MsgSnapStatus:
            OnMsgSnapStatus(msg, pProgress);
            break;
        case MsgUnreachable:
            OnMsgUnreachable(msg, pProgress);
            break;
        case MsgTransferLeader:
            OnMsgTransferLeader(msg, pProgress);
            break;
        default:
            m_pLogger->Infof(__FILE__, __LINE__, "%x [term: %llu] received and ignored a %s message with term from %x [term: %llu]",
                m_pConfig->m_nRaftID, m_u64Term, CRaftUtil::MsgType2String(msg.type()), msg.from(), msg.term());
            break;
        }
    }
    else
        m_pLogger->Debugf(__FILE__, __LINE__, "%x no progress available for %x", m_pConfig->m_nRaftID,msg.from());
}

void CRaft::OnMsgReadIndex(const Message &msg)
{
    if (GetQuorum() > 1)
    {
        uint64_t term;
        int err = m_pRaftLog->GetTerm(m_pRaftLog->GetCommitted(), term);
        if (m_pRaftLog->ZeroTermOnErrCompacted(term, err) != m_u64Term)
        {
            //如果在本任期尚未提交日志，说明还在日志同步阶段，不能对外提供读服务
            return;
        }
        // thinking: use an interally defined context instead of the user given context.
        // We can express this in terms of the term and index instead of a user-supplied value.
        // This would allow multiple reads to piggyback on the same message.
        if (m_pReadOnly->m_optMode == ReadOnlySafe)
        {
            Message *pNewMsg = CloneMessage(msg);
            m_pReadOnly->AddRequest(m_pRaftLog->GetCommitted(), pNewMsg);
            BcastHeartbeatWithCtx(pNewMsg->entries(0).data());
            return;
        }
        else if (m_pReadOnly->m_optMode == ReadOnlyLeaseBased)
        {
            uint64_t ri = 0;
            if (m_pConfig->m_bCheckQuorum)
            {
                ri = m_pRaftLog->GetCommitted();
            }
            //本地模式直接应答
            if (msg.from() == None || msg.from() == m_pConfig->m_nRaftID)
                SendReadReady(new CReadState(m_pRaftLog->GetCommitted(), msg.entries(0).data()));
            else
            {
                //代理模式，先传到代理节点
                Message *pNewMsg = CloneMessage(msg);
                pNewMsg->set_to(msg.from());
                pNewMsg->set_type(MsgReadIndexResp);
                pNewMsg->set_index(ri);
                SendMsg(pNewMsg);
            }
        }
    }
    else //单节点模式
    {
        SendReadReady(new CReadState(m_pRaftLog->GetCommitted(), msg.entries(0).data()));
    }
}

void CRaft::OnMsgCheckQuorum(const Message &)
{
    if (!CheckQuorumActive())
    {
        m_pLogger->Warningf(__FILE__, __LINE__, "%x stepped down to follower since quorum is not active", m_pConfig->m_nRaftID);
        BecomeFollower(m_u64Term, None);
    }
}

void CRaft::OnMsgProp(const Message &msg)
{
    if (msg.entries_size() == 0)
        m_pLogger->Fatalf(__FILE__, __LINE__, "%x stepped empty MsgProp", m_pConfig->m_nRaftID);
    if (msg.entries_size() > 1)
        m_pLogger->Fatalf(__FILE__, __LINE__, "%x stepped more then one entry of MsgProp", m_pConfig->m_nRaftID);
    if (m_mapProgress.find(m_pConfig->m_nRaftID) == m_mapProgress.end())
    {
        // If we are not currently a member of the range (i.e. this node
        // was removed from the configuration while serving as leader),
        // drop any new proposals.
        return;
    }
    if (m_nLeaderTransfereeID != None)
    {
        m_pLogger->Debugf(__FILE__, __LINE__,
            "%x [term %d] transfer leadership to %x is in progress; dropping proposal",
            m_pConfig->m_nRaftID, m_u64Term, m_nLeaderTransfereeID);
        return;
    }

    Message *pNewMsg = CloneMessage(msg);
    for (int i = 0; i < int(pNewMsg->entries_size()); ++i)
    {
        Entry *entry = pNewMsg->mutable_entries(i);
        if (entry->type() != EntryConfChange)
            continue;
        if (m_bPendingConf)
        {
            m_pLogger->Infof(__FILE__, __LINE__,
                "propose conf %s ignored since pending unapplied configuration",
                entryString(*entry).c_str());
            Entry tmp;
            tmp.set_type(EntryNormal);
            entry->CopyFrom(tmp);
        }
        m_bPendingConf = true;
    }
    EntryVec entries;
    copyEntries(*pNewMsg, entries);
    m_listWrite.push_back(pNewMsg);
    AppendEntry(entries);
    BcastAppend();
}

void CRaft::OnMsgSnapStatus(const Message &msg, CProgress * pProgress)
{
    uint32_t from = msg.from();
    if (pProgress->state_ != ProgressStateSnapshot)
    {
        return;
    }
    if (!msg.reject())
    {
        pProgress->BecomeProbe();
        m_pLogger->Debugf(__FILE__, __LINE__, "%x snapshot succeeded, resumed sending replication messages to %x [%s]",
            m_pConfig->m_nRaftID, from, pProgress->GetInfoText().c_str());
    }
    else
    {
        pProgress->snapshotFailure();
        pProgress->BecomeProbe();
        m_pLogger->Debugf(__FILE__, __LINE__, "%x snapshot failed, resumed sending replication messages to %x [%s]",
            m_pConfig->m_nRaftID, from, pProgress->GetInfoText().c_str());
    }
    // If snapshot finish, wait for the msgAppResp from the remote node before sending
    // out the next msgApp.
    // If snapshot failure, wait for a heartbeat interval before next try
    pProgress->Pause();
}

void CRaft::OnMsgUnreachable(const Message &msg, CProgress * pProgress)
{
    // During optimistic replication, if the remote becomes unreachable,
    // there is huge probability that a MsgApp is lost.
    if (pProgress->state_ == ProgressStateReplicate)
        pProgress->BecomeProbe();
    m_pLogger->Debugf(__FILE__, __LINE__, "%x failed to send message to %x because it is unreachable [%s]",
        m_pConfig->m_nRaftID, msg.from(), pProgress->GetInfoText().c_str());
}

void CRaft::OnMsgTransferLeader(const Message &msg,CProgress * pProgress)
{
    uint32_t leadTransferee = msg.from();
    uint32_t lastLeadTransferee = m_nLeaderTransfereeID;
    if (lastLeadTransferee != None)
    {
        if (lastLeadTransferee == leadTransferee)
        {
            m_pLogger->Infof(__FILE__, __LINE__,
                "%x [term %llu] transfer leadership to %x is in progress, ignores request to same node %x",
                m_pConfig->m_nRaftID, m_u64Term, leadTransferee, leadTransferee);
            return;
        }
        AbortLeaderTransfer();
        m_pLogger->Infof(__FILE__, __LINE__,
            "%x [term %d] abort previous transferring leadership to %x",
            m_pConfig->m_nRaftID, m_u64Term, leadTransferee);
    }
    if (leadTransferee == m_pConfig->m_nRaftID)
    {
        m_pLogger->Debugf(__FILE__, __LINE__,
            "%x is already leader. Ignored transferring leadership to self",
            m_pConfig->m_nRaftID);
        return;
    }
    // Transfer leadership to third party.
    m_pLogger->Infof(__FILE__, __LINE__,
        "%x [term %llu] starts to transfer leadership to %x",
        m_pConfig->m_nRaftID, m_u64Term, leadTransferee);
    // Transfer leadership should be finished in one electionTimeout, so reset r.electionElapsed.
    m_nTicksElectionElapsed = 0;
    m_nLeaderTransfereeID = leadTransferee;
    if (pProgress->m_u64MatchLogIndex == m_pRaftLog->GetLastIndex())
    {
        //日志追平后，立刻触发目标的选举流程
        SendTimeoutNow(leadTransferee);
        m_pLogger->Infof(__FILE__, __LINE__,
            "%x sends MsgTimeoutNow to %x immediately as %x already has up-to-date log",
            m_pConfig->m_nRaftID, leadTransferee, leadTransferee);
    }
    else //继续追日志
        SendAppend(leadTransferee);
}

void CRaft::OnAppResp(const Message& msg, CProgress *pProgress)
{
    uint32_t from = msg.from();
    uint64_t u64Index = msg.index();
    pProgress->m_bRecentActive = true;
    if (msg.reject())
    {
        //如果拒绝，就说明follower可能漏掉了很多日志条目，需要更新leader存储的对应的该节点的Next值，然后重新向follower发AppendEntries
        m_pLogger->Debugf(__FILE__, __LINE__, "%x received msgApp rejection(lastindex: %llu) from %x for index %llu",
            m_pConfig->m_nRaftID, msg.rejecthint(), from, u64Index);
        if (pProgress->maybeDecrTo(u64Index, msg.rejecthint()))
        {
            m_pLogger->Debugf(__FILE__, __LINE__, "%x decreased progress of %x to [%s]",
                m_pConfig->m_nRaftID, from, pProgress->GetInfoText().c_str());
            if (pProgress->state_ == ProgressStateReplicate)
                pProgress->BecomeProbe();
            SendAppend(from);
        }
    }
    else
    {
        bool oldPaused = pProgress->IsPaused();
        if (pProgress->MaybeUpdate(u64Index))
        {
            if (pProgress->state_ == ProgressStateProbe)
                pProgress->BecomeReplicate();
            else if (pProgress->state_ == ProgressStateSnapshot && pProgress->needSnapshotAbort())
            {
                m_pLogger->Debugf(__FILE__, __LINE__, "%x snapshot aborted, resumed sending replication messages to %x [%s]",
                    m_pConfig->m_nRaftID, from, pProgress->GetInfoText().c_str());
                pProgress->BecomeProbe();
            }
            else if (pProgress->state_ == ProgressStateReplicate)
                pProgress->ins_.freeTo(u64Index);
            //尝试提交日志
            if (MaybeCommit())
                BcastAppend();
            else if (oldPaused)
            {
                // update() reset the wait state on this node. If we had delayed sending
                // an update before, send it now.
                SendAppend(from);
            }
            // Transfer leadership is in progress.
            if (msg.from() == m_nLeaderTransfereeID 
                && pProgress->m_u64MatchLogIndex == m_pRaftLog->GetLastIndex())
            {
                m_pLogger->Infof(__FILE__, __LINE__,
                    "%x sent MsgTimeoutNow to %x after received MsgAppResp", m_pConfig->m_nRaftID, msg.from());
                //日志追平后，立刻触发目标的选举流程
                SendTimeoutNow(msg.from());
            }
        }
    }
}

//Leader状态时处理心跳回应消息
void CRaft::OnHeartbeatResp(const Message& msg, CProgress *pProgress)
{
    uint32_t from = msg.from();   
    pProgress->m_bRecentActive = true;
    pProgress->Resume();

    // free one slot for the full inflights window to allow progress.
    if (pProgress->state_ == ProgressStateReplicate && pProgress->ins_.full())
        pProgress->ins_.freeFirstOne();
    if (pProgress->m_u64MatchLogIndex < m_pRaftLog->GetLastIndex())
        SendAppend(from);

    if (m_pReadOnly->m_optMode != ReadOnlySafe || msg.context().empty())
        return;

    int nAck = m_pReadOnly->RecvAck(msg);
    if (nAck < GetQuorum())
        return;

    vector<CReadIndexStatus*> rss;
    m_pReadOnly->Advance(msg, &rss);
    for (size_t i = 0; i < rss.size(); ++i)
    {
        Message * pReadIndexMsg = rss[i]->m_pReadIndexMsg;
        if (pReadIndexMsg->from() == None || pReadIndexMsg->from() == m_pConfig->m_nRaftID)
        {
            //请求来自本地或者Client直接请求Leader
            SendReadReady(new CReadState(rss[i]->m_u64CommitIndex, pReadIndexMsg->entries(0).data()));
        }
        else
        {
            //请求由Follower代理而来
            Message * respMsg = new Message();
            respMsg->set_type(MsgReadIndexResp);
            respMsg->set_to(pReadIndexMsg->from());
            respMsg->set_index(rss[i]->m_u64CommitIndex);
            respMsg->mutable_entries()->CopyFrom(pReadIndexMsg->entries());
            SendMsg(respMsg);
        }
    }
}

// stepCandidate is shared by StateCandidate and StatePreCandidate; the difference is
// whether they respond to MsgVoteResp or MsgPreVoteResp.
void CRaft::StepByCandidate(const Message& msg)
{
    // Only handle vote responses corresponding to our candidacy (while in
    // StateCandidate, we may get stale MsgPreVoteResp messages in this term from
    // our pre-candidate state).
    MessageType voteRespType;
    if (m_stateRaft == eStatePreCandidate)
        voteRespType = MsgPreVoteResp;
    else
        voteRespType = MsgVoteResp;

    MessageType typeMsg = msg.type();

    if (typeMsg == voteRespType)
    {
        //记录投票并统计结果
        int nGranted = Poll(msg.from(), msg.type(), !msg.reject());
        m_pLogger->Infof(__FILE__, __LINE__, "%x [quorum:%llu] has received %d %s votes and %llu vote rejections",
            m_pConfig->m_nRaftID, GetQuorum(), nGranted, CRaftUtil::MsgType2String(typeMsg), m_mapVotes.size() - nGranted);
        if (nGranted == GetQuorum())
        {
            if (m_stateRaft == eStatePreCandidate)
            {
                //如果是PreCandidate，在预选中收到半数以上的赞成票之后，会发起正式的选举
                Campaign(campaignElection);
            }
            else 
            {
                //如果是Candidate，在选举中收到半数以上的赞成票之后，会切换为Leader
                //然后向集群中其他节点广播MsgApp消息
                BecomeLeader();
                BcastAppend();
            }
        }
        else if (GetQuorum() == m_mapVotes.size() - nGranted)
        {
            //获取了半数以上的反对票，当前节点切换成跟随者Follower状态，等待下一轮的选举超时
            BecomeFollower(m_u64Term, None);
        }
        return;
    }

    switch (typeMsg)
    {
    case MsgProp:
        m_pLogger->Infof(__FILE__, __LINE__, "%x no leader at term %llu; dropping proposal", m_pConfig->m_nRaftID, m_u64Term);
        break;
    case MsgApp:
        BecomeFollower(m_u64Term, msg.from());
        OnAppendEntries(msg);
        break;
    case MsgHeartbeat:
        BecomeFollower(m_u64Term, msg.from());
        OnHeartbeat(msg);
        break;
    case MsgSnap:
        BecomeFollower(msg.term(), msg.from());
        OnSnapshot(msg);
        break;
    case MsgTimeoutNow:
        m_pLogger->Debugf(__FILE__, __LINE__, "%x [term %llu state candidate] ignored MsgTimeoutNow from %x",
            m_pConfig->m_nRaftID, m_u64Term, msg.from());
        break;
    default:
        m_pLogger->Infof(__FILE__, __LINE__, "%x [term: %llu] received and ignored a %s message with term from %x [term: %llu]",
            m_pConfig->m_nRaftID, m_u64Term, CRaftUtil::MsgType2String(msg.type()), msg.from(), msg.term());
        break;
    }
}

void CRaft::StepByFollower(const Message& msg)
{
    switch (msg.type())
    {
    case MsgProp:
        OnProxyMsgProp(msg);
        break;
    case MsgApp:
        m_nTicksElectionElapsed = 0; //重置选举计时器，防止当前Follower发起新一轮选举
        m_nLeaderID = msg.from();    //保存当前的Leader节点ID
        OnAppendEntries(msg);        //将MsgApp消息中携带的Entry记录追加到raftLog中，并且向Leader节点发送MsgAppResp消息
        break;
    case MsgHeartbeat:
        m_nTicksElectionElapsed = 0;
        m_nLeaderID = msg.from();
        OnHeartbeat(msg);
        break;
    case MsgSnap:
        m_nTicksElectionElapsed = 0;
        m_nLeaderID = msg.from();
        OnSnapshot(msg);
        break;
    case MsgTransferLeader:
        if (m_nLeaderID == None)
        {
            m_pLogger->Infof(__FILE__, __LINE__,
                "%x no leader at term %llu; dropping leader transfer msg",
                m_pConfig->m_nRaftID, m_u64Term);
        }
        else
        {
            Message *pMsg = CloneMessage(msg);
            pMsg->set_to(m_nLeaderID);
            SendMsg(pMsg);
        }
        break;
    case MsgTimeoutNow:
        OnMsgTimeoutNow(msg);
        break;
    case MsgReadIndex:
        OnProxyMsgReadIndex(msg);
        break;
    case MsgReadIndexResp:
        OnMsgReadIndexResp(msg);
        break;
    default:
        m_pLogger->Infof(__FILE__, __LINE__, "%x [term: %llu] received and ignored a %s message with term from %x [term: %llu]",
            m_pConfig->m_nRaftID, m_u64Term, CRaftUtil::MsgType2String(msg.type()), msg.from(), msg.term());
        break;
    }
}

void CRaft::OnMsgTimeoutNow(const Message &msg)
{
    if (IsPromotable())
    {
        m_pLogger->Infof(__FILE__, __LINE__,
            "%x [term %llu] received MsgTimeoutNow from %x and starts an election to get leadership.",
            m_pConfig->m_nRaftID, m_u64Term, msg.from());
        // Leadership transfers never use pre-vote even if r.preVote is true; we
        // know we are not recovering from a partition so there is no need for the
        // extra round trip.
        Campaign(campaignTransfer);
    }
    else
    {
        m_pLogger->Infof(__FILE__, __LINE__,
            "%x received MsgTimeoutNow from %x but is not promotable",
            m_pConfig->m_nRaftID, msg.from());
    }
}

void CRaft::OnProxyMsgProp(const Message& msg)
{
    if (m_nLeaderID != None)
    {
        Message *pWriteMsg = CloneMessage(msg);
        m_listWrite.push_back(pWriteMsg);
        Message *pMsg = CloneMessage(msg);
        pMsg->set_to(m_nLeaderID);
        SendMsg(pMsg);
    }
}

void CRaft::OnProxyMsgReadIndex(const Message& msg)
{
    if (m_nLeaderID == None)
        m_pLogger->Infof(__FILE__, __LINE__, "%x no leader at term %llu; dropping index reading msg", m_pConfig->m_nRaftID, m_u64Term);
    else
    {
        Message *pMsg = CloneMessage(msg);
        pMsg->set_to(m_nLeaderID);
        SendMsg(pMsg);
    }
}

void CRaft::OnMsgReadIndexResp(const Message &msg)
{
    if (msg.entries_size() != 1)
        m_pLogger->Errorf(__FILE__, __LINE__, "%x invalid format of MsgReadIndexResp from %x, entries count: %llu",
            m_pConfig->m_nRaftID, msg.from(), msg.entries_size());
    else
        SendReadReady(new CReadState(msg.index(), msg.entries(0).data()));
}

// restore recovers the state machine from a snapshot. It restores the log and the
// configuration of state machine.
bool CRaft::Restore(const Snapshot& snapshot)
{
    if (snapshot.metadata().index() <= m_pRaftLog->GetCommitted())
    {
        return false;
    }

    if (m_pRaftLog->IsMatchTerm(snapshot.metadata().index(), snapshot.metadata().term()))
    {
        m_pLogger->Infof(__FILE__, __LINE__, "%x [commit: %llu, lastindex: %llu, lastterm: %llu] fast-forwarded commit to snapshot [index: %llu, term: %llu]",
            m_pConfig->m_nRaftID, m_pRaftLog->GetCommitted(), m_pRaftLog->GetLastIndex(), m_pRaftLog->GetLastTerm(),
            snapshot.metadata().index(), snapshot.metadata().term());
        m_pRaftLog->CommitTo(snapshot.metadata().index());
        return false;
    }
    m_pLogger->Infof(__FILE__, __LINE__, "%x [commit: %llu, lastindex: %llu, lastterm: %llu] starts to restore snapshot [index: %llu, term: %llu]",
        m_pConfig->m_nRaftID, m_pRaftLog->GetCommitted(), m_pRaftLog->GetLastIndex(), m_pRaftLog->GetLastTerm(),
        snapshot.metadata().index(), snapshot.metadata().term());
    m_pRaftLog->Restore(snapshot);
    m_mapProgress.clear();

    for (int i = 0; i < snapshot.metadata().conf_state().nodes_size(); ++i)
    {
        uint32_t node = snapshot.metadata().conf_state().nodes(i);
        uint64_t match = 0;
        uint64_t next = m_pRaftLog->GetLastIndex() + 1;
        if (node == m_pConfig->m_nRaftID)
        {
            match = next - 1;
        }
        ResetProgress(node, match, next);
        m_pLogger->Infof(__FILE__, __LINE__, "%lu restored progress of %x [%s]", m_pConfig->m_nRaftID, node, m_mapProgress[node]->GetInfoText().c_str());
    }
    return true;
}

void CRaft::OnSnapshot(const Message& msg)
{
    uint64_t sindex = msg.snapshot().metadata().index();
    uint64_t sterm = msg.snapshot().metadata().term();
    Message *resp = new Message;

    resp->set_to(msg.from());
    resp->set_type(MsgAppResp);
    if (Restore(msg.snapshot()))
    {
        m_pLogger->Infof(__FILE__, __LINE__, "%x [commit: %d] restored snapshot [index: %d, term: %d]",
            m_pConfig->m_nRaftID, m_pRaftLog->GetCommitted(), sindex, sterm);
        resp->set_index(m_pRaftLog->GetLastIndex());
    }
    else
    {
        m_pLogger->Infof(__FILE__, __LINE__, "%x [commit: %d] ignored snapshot [index: %d, term: %d]",
            m_pConfig->m_nRaftID, m_pRaftLog->GetCommitted(), sindex, sterm);
        resp->set_index(m_pRaftLog->GetCommitted());
    }
    SendMsg(resp);
}

void CRaft::OnHeartbeat(const Message& msg)
{
    //同步Leader日志提交号
    bool bCommited = m_pRaftLog->CommitTo(msg.commit());
    if (bCommited)
        CommitWrite(m_pRaftLog->GetCommitted());

    Message *resp = new Message();
    resp->set_to(msg.from());
    resp->set_type(MsgHeartbeatResp);
    resp->set_context(msg.context());
    SendMsg(resp);
}

//1.消息的索引值小于当前节点已提交的值，则返回MsgAppResp消息类型，并返回已提交的位置
//2.如果消息追加成功，则返回MsgAppResp消息类型，并返回最后一条记录的索引值
//3.如果追加失败，则Reject设为true，并返回raftLog中最后一条记录的索引
void CRaft::OnAppendEntries(const Message& msg)
{
    Message *pRespMsg = NULL;
    //原则：已经提交的日志不能被覆盖
    if (msg.index() < m_pRaftLog->GetCommitted())
    {
        pRespMsg = new Message();
        pRespMsg->set_to(msg.from());
        pRespMsg->set_type(MsgAppResp);
        pRespMsg->set_index(m_pRaftLog->GetCommitted());
    }
    else
    {
        EntryVec entries;
        copyEntries(msg, entries);
        uint64_t u64LastCommitted = m_pRaftLog->GetCommitted();
        uint64_t u64LastIndex;
        bool ret = m_pRaftLog->MaybeAppend(msg.index(), msg.logterm(), msg.commit(), entries, u64LastIndex);
        if (ret)
        {
            //追加Leader日志时产生了提交动作
            if (u64LastCommitted != m_pRaftLog->GetCommitted())
                CommitWrite(m_pRaftLog->GetCommitted());

            pRespMsg = new Message();
            pRespMsg->set_to(msg.from());
            pRespMsg->set_type(MsgAppResp);
            pRespMsg->set_index(u64LastIndex);
        }
        else
        {
            //日志不匹配说明Follower落后Leader节点，将当前LastIndex发给Leader，希望重发
            uint64_t term;
            int err = m_pRaftLog->GetTerm(msg.index(), term);
            m_pLogger->Debugf(__FILE__, __LINE__,
                "%x [logterm: %llu, index: %llu] rejected msgApp [logterm: %llu, index: %llu] from %x",
                m_pConfig->m_nRaftID, m_pRaftLog->ZeroTermOnErrCompacted(term, err), msg.index(), msg.logterm(), msg.index(), msg.from());
            pRespMsg = new Message();
            pRespMsg->set_to(msg.from());
            pRespMsg->set_type(MsgAppResp);
            pRespMsg->set_index(msg.index());
            pRespMsg->set_reject(true);
            pRespMsg->set_rejecthint(m_pRaftLog->GetLastIndex());
        }
    }
    assert(pRespMsg != NULL);
    SendMsg(pRespMsg);
}

void CRaft::ResetProgress(uint32_t nRaftID, uint64_t u64Match, uint64_t u64Next)
{
    if (m_mapProgress.find(nRaftID) != m_mapProgress.end())
        delete m_mapProgress[nRaftID];
    m_mapProgress[nRaftID] = new CProgress(u64Next, m_pConfig->m_nMaxInfilght, m_pLogger);
    m_mapProgress[nRaftID]->m_u64MatchLogIndex = u64Match;
}

void CRaft::DelProgress(uint32_t nRaftID)
{
    if (m_mapProgress.find(nRaftID) != m_mapProgress.end())
    {
        delete m_mapProgress[nRaftID];
        m_mapProgress.erase(nRaftID);
    }
}

void CRaft::AbortLeaderTransfer()
{
    m_nLeaderTransfereeID = None;
}

void CRaft::AddNode(uint32_t nRaftID)
{
    m_bPendingConf = false;
    if (m_mapProgress.find(nRaftID) == m_mapProgress.end())
        ResetProgress(nRaftID, 0, m_pRaftLog->GetLastIndex() + 1);
}

void CRaft::RemoveNode(uint32_t nRaftID)
{
    DelProgress(nRaftID);
    m_bPendingConf = false;

    // do not try to commit or abort transferring if there is no nodes in the cluster.
    if (m_mapProgress.empty())
    {
        return;
    }

    // The quorum size is now smaller, so see if any pending entries can
    // be committed.
    if (MaybeCommit())
        BcastAppend();

    // If the removed node is the leadTransferee, then abort the leadership transferring.
    if (m_stateRaft == eStateLeader && m_nLeaderTransfereeID == nRaftID)
        AbortLeaderTransfer();
}

void CRaft::SendTimeoutNow(uint32_t nRaftID)
{
    Message *pMsg = new Message();
    pMsg->set_to(nRaftID);
    pMsg->set_type(MsgTimeoutNow);
    SendMsg(pMsg);
}

void CRaft::ResetPendingConf(void)
{
    m_bPendingConf = false;
}

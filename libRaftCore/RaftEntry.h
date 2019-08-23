#pragma once
#include "libRaftCore.h"

///\brief Raft协议中的一条日志
class LIBRAFTCORE_API CRaftEntry
{
public:
    ///\brief 日志类型
    enum EType
    {
        eNormal = 0,     ///< 普通日志
        eConfChange = 1  ///< 组员变化日志
    };
public:
    CRaftEntry(void);

    CRaftEntry(const CRaftEntry & entry);

    CRaftEntry& operator =(const CRaftEntry& str);

    ~CRaftEntry(void);

    void Copy(const CRaftEntry & entry);

    inline uint64_t term(void) const
    {
        return m_u64Term;
    }

    inline void set_term(uint64_t u64Term)
    {
        m_u64Term = u64Term;
    }

    inline uint64_t index(void) const
    {
        return m_u64Index;
    }
    inline void set_index(uint64_t u64Index)
    {
        m_u64Index = u64Index;
    }

    inline CRaftEntry::EType type(void) const
    {
        return m_eType;
    }

    inline void set_type(CRaftEntry::EType eType)
    {
        m_eType = eType;
    }

    std::string data(void) const
    {
        return m_strData ;
    }

    void set_data(const char *pstrData)
    {
        m_strData = pstrData;
    }
    
    void set_data(const std::string &strData)
    {
        m_strData = strData;
    }

public:
    uint64_t m_u64Term;         ///< 任期号
    uint64_t m_u64Index;        ///< 日志索引号
    EType m_eType;              ///< 类型，是日志还是配置变更
    std::string m_strData;      ///< 数据缓冲区
};

class LIBRAFTCORE_API CConfState
{
public:
    CConfState(void)
    {
        m_nNum = 0;
        m_pnNodes = NULL;
    }

    CConfState(const CConfState &stateConf)
    {
        m_nNum = 0;
        m_pnNodes = NULL;
        Copy(stateConf);
    }

    CConfState& operator = (const CConfState &stateConf)
    {
        Copy(stateConf);
        return *this;
    }

    ~CConfState(void);

    void Copy(const CConfState & stateConf);

    inline uint32_t nodes_size(void) const
    {
        return m_nNum;
    }

    inline uint32_t nodes(int nIndex) const
    {
        assert(nIndex >= 0 && nIndex < m_nNum);
        return m_pnNodes[nIndex];
    }

    void add_nodes(uint32_t nNodeID);

public:
    unsigned char m_nNum; ///< 节点个数
    uint32_t *m_pnNodes; ///< 节点标识缓冲
};

class LIBRAFTCORE_API CSnapshotMetadata
{
public:
    CSnapshotMetadata(void)
    {
        m_u64Term = 0;
        m_u64Index =0;
    }

    void Copy(const CSnapshotMetadata &metaDat)
    {
        m_u64Index = metaDat.m_u64Index;
        m_u64Term = metaDat.m_u64Term;
        m_stateConf.Copy(metaDat.m_stateConf);
    };

    const CConfState &conf_state(void) const
    {
        return m_stateConf;
    };
    
    CConfState *mutable_conf_state(void)
    {
        return &m_stateConf;
    }
    
    inline uint64_t term(void) const
    {
        return m_u64Term;
    }

    inline void set_term(uint64_t u64Term)
    {
        m_u64Term = u64Term;
    }
    
    inline uint64_t index(void) const
    {
        return m_u64Index;
    }

    inline void set_index(uint64_t u64Index)
    {
        m_u64Index = u64Index;
    }
public:
    uint64_t m_u64Term;    ///< 任期号
    uint64_t m_u64Index;   ///< 日志索引号
    CConfState m_stateConf;
};

class LIBRAFTCORE_API CSnapshot
{
public:
    CSnapshot(void)
    {
        m_u32DatLen = 0;
        m_pData = NULL;
    }
    
    ~CSnapshot(void);

    const CSnapshotMetadata & metadata(void) const
    {
        return m_metaDat;
    }

    CSnapshotMetadata * mutable_metadata(void)
    {
        return &m_metaDat;
    }

    void CopyFrom(const CSnapshot& snapshot);
    
    void set_data(const std::string &data);
    
public:
    CSnapshotMetadata m_metaDat;
    uint32_t m_u32DatLen;
    unsigned char *m_pData;
};

class LIBRAFTCORE_API CMessage
{
public:
    enum EMessageType
    {
        MsgHup = 0,             //当Follower节点的选举计时器超时，会发送MsgHup消息
        MsgBeat = 1,            //Leader发送心跳，主要作用是探活，Follower接收到MsgBeat会重置选举计时器，防止Follower发起新一轮选举
        MsgProp = 2,            //客户端发往到集群的写请求是通过MsgProp消息表示的
        MsgApp = 3,             //当一个节点通过选举成为Leader时，会向Follower发送MsgApp消息同步日志
        MsgAppResp = 4,         //MsgApp的响应消息
        MsgVote = 5,            //当PreCandidate状态节点收到半数以上的投票之后，会发起新一轮的选举，即向集群中的其他节点发送MsgVote消息
        MsgVoteResp = 6,        //MsgVote选举消息响应的消息
        MsgSnap = 7,            //Leader向Follower发送快照信息
        MsgHeartbeat = 8,       //Leader发送的心跳消息
        MsgHeartbeatResp = 9,   //Follower处理心跳回复返回的消息类型
        MsgUnreachable = 10,    //Follower消息不可达
        MsgSnapStatus = 11,     //如果Leader发送MsgSnap消息时出现异常，则会调用Raft接口发送MsgUnreachable和MsgSnapStatus消息
        MsgCheckQuorum = 12,    //Leader检测是否保持半数以上的连接
        MsgTransferLeader = 13, //Leader节点转移时使用
        MsgTimeoutNow = 14,     //Leader节点转移超时，会发该类型的消息，使Follower的选举计时器立即过期，并发起新一轮的选举
        MsgReadIndex = 15,      //客户端发往集群的只读消息使用MsgReadIndex消息（只读的两种模式：ReadOnlySafe和ReadOnlyLeaseBased）
        MsgReadIndexResp = 16,  //MsgReadIndex消息的响应消息
        MsgPreVote = 17,        //PreCandidate状态下的节点发送的消息
        MsgPreVoteResp = 18,     //预选节点收到的响应消息
        MsgNoUsed
    };
public:
    CMessage(void);

    CMessage(const CMessage &msg);

    void Copy(const CMessage &msg);

    CMessage& operator =(const CMessage &msg);

    ~CMessage(void);

    inline uint32_t from(void) const
    {
        return m_nFromID;
    }

    inline void set_from(uint32_t nFromID)
    {
        m_nFromID = nFromID;
    }

    inline uint32_t to(void) const
    {
        return m_nToID;
    }
    
    inline void set_to(uint32_t nToID)
    {
        m_nToID = nToID;
    }

    inline uint64_t term(void) const
    {
        return m_u64Term;
    }
    
    inline void set_term(uint64_t u64Term)
    {
        m_u64Term = u64Term;
    }

    inline uint64_t index(void) const
    {
        return m_u64Index;
    }
    
    inline void set_index(uint64_t u64Index)
    {
        m_u64Index = u64Index;
    }
    
    inline uint64_t commit(void) const
    {
        return m_u64Committed;
    }

    inline void set_commit(uint64_t u64Committed)
    {
        m_u64Committed = u64Committed;
    }

    inline uint64_t logterm(void) const
    {
        return m_u64LogTerm;
    }

    inline void set_logterm(uint64_t u64LogTerm)
    {
        m_u64LogTerm = u64LogTerm;
    }

    inline EMessageType type(void) const
    {
        return m_typeMsg ;
    }
           
    inline void set_type(EMessageType eType)
    {
        m_typeMsg = eType;
    }

    inline bool reject(void) const
    {
        return m_bReject;
    }

    inline void set_reject(bool bReject)
    {
        m_bReject = bReject;
    }

    inline uint64_t rejecthint(void) const
    {
        return m_u64RejectHint;
    }

    inline void set_rejecthint(uint64_t u64RejectHint)
    {
        m_u64RejectHint = u64RejectHint;
    }

    inline const CSnapshot & snapshot(void) const
    {
        return m_snapshotLog;
    }

    inline CSnapshot *mutable_snapshot(void)
    {
        return &m_snapshotLog;
    }

    inline const std::string &context(void) const
    {
        return m_strContext;
    }

    inline void set_context(const std::string &strContext)
    {
        m_strContext = strContext;
    }

    void set_entries(std::vector<CRaftEntry> &entries);
    void set_entries_from_msg(const CMessage &msg);

    CRaftEntry *entries(uint16_t nIndex) const;

    inline uint16_t entries_size(void)const
    {
        return m_nEntryNum;
    }

    CRaftEntry *add_entries(void);
public:
    uint32_t m_nToID;           ///< 消息的目标ID
    uint32_t m_nFromID;         ///< 消息的来源ID
    uint64_t m_u64Term;         ///< 任期号
    uint64_t m_u64Index;        ///< 日志索引号
    uint64_t m_u64LogTerm;      ///< 日志任期号
    uint64_t m_u64Committed;    ///< 已经提交的日志号
    uint64_t m_u64RejectHint;   ///< 如果投反对票，同时返回最后的日志索引号
    CRaftEntry *m_pEntry;       ///< 用数组式指针存储日志对象
    uint16_t m_nEntryNum;       ///< 消息包含的日志数目
    EMessageType m_typeMsg;     ///< 消息类型
    bool        m_bReject;      ///< 是否投反对票
    CSnapshot   m_snapshotLog;  ///< 
    std::string m_strContext;   ///< 
};

class LIBRAFTCORE_API CHardState
{
public:
    CHardState(void)
    {
        m_u64Committed = 0;
        m_u64Term = 0;
        m_u32VoteID = 0;
    }

    inline uint64_t term(void) const
    {
        return m_u64Term;
    }
    inline uint64_t commit(void) const
    {
        return m_u64Committed;
    }
    inline uint32_t vote(void) const
    {
        return m_u32VoteID;
    }
    inline void set_term(uint64_t u64Term)
    {
        m_u64Term = u64Term;
    }
    inline void set_commit(uint64_t u64Committed)
    {
        m_u64Committed = u64Committed;
    }
    inline void set_vote(uint32_t u32VoteID)
    {
        m_u32VoteID = u32VoteID;
    }
public:
    uint64_t m_u64Term;      ///< 任期号
    uint64_t m_u64Committed; ///< 已经提交的日志号
    uint32_t m_u32VoteID;    ///< 投票目标的ID
};

class LIBRAFTCORE_API CConfChange
{
public:
    enum EConfChangeType
    {
        eConfChangeAddNode = 0,
        eConfChangeRemoveNode = 1,
        eConfChangeUpdateNode = 2
    };

    inline void set_type(EConfChangeType typeChange)
    {
        m_typeChange = typeChange;
    }

    inline void set_nodeid(uint32_t nRaftID)
    {
        m_nRaftID = nRaftID;
    }

    void SerializeToString(std::string &strData);
    
    uint64_t        m_nID;          ///< 唯一标识
    EConfChangeType m_typeChange;   ///< 类型
    uint32_t   m_nRaftID;           ///< 节点ID
    uint32_t m_u32Len;
    unsigned char *m_pContext;
};

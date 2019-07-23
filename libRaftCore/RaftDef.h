#pragma once

#include <cstdint>
#include <climits>
#include <string>
#include <vector>
#include "raft.pb.h"

using namespace std;
using namespace raftpb;

const static uint32_t None = 0;
const static uint64_t noLimit = ULONG_MAX;

enum ErrorCode
{
    OK = 0,
    ErrCompacted = 1,       ///< 企图读取已经被压缩后的数据
    ErrSnapOutOfDate = 2, 
    ErrUnavailable = 3,     ///< 企图读取索引号还没有过的数据
    ErrSnapshotTemporarilyUnavailable = 4,
    ErrSeriaFail = 5
};

inline bool SUCCESS(int nErrorNo)
{
    return nErrorNo == OK;
}

///\brief Raft状态机枚举值的定义
enum EStateType
{
    eStateFollower = 0,
    eStateCandidate = 1,
    eStateLeader = 2,
    eStatePreCandidate = 3,
    numStates
};

///\brief 节点"软"状态，无需持久化
class CSoftState
{
public:
    ///\brief 构造函数，Leader缺省为无，状态确认为追随者
    CSoftState(void)
        : m_nLeaderID(None),
        m_stateRaft(eStateFollower)
    {
    }
public:
    uint32_t m_nLeaderID;  ///< 当前领导者ID
    EStateType m_stateRaft; ///< 当前状态（领导者，追随者，竞选者，备选者） 
};

// ReadState provides state for read only query.
// It's caller's responsibility to call ReadIndex first before getting
// this state from ready, It's also caller's duty to differentiate if this
// state is what it requests through RequestCtx, eg. given a unique id as
// RequestCtx
class CReadState
{
public:
    uint64_t m_u64Index;
    string   m_strRequestCtx;
    CReadState(uint64_t u64Index, const string &strRequestCtx)
        : m_u64Index(u64Index),
        m_strRequestCtx(strRequestCtx)
    {
    }
};

///\brief 日志操作 用于修改状态机，应用日志，以及读操作
class CLogOperation
{
public:
    ///\brief 构造函数，缺省为无效的读操作
    CLogOperation(void)
    {
        m_nType = 0;
        m_u64ApplyTo = 0;
        m_pOperation = NULL;
    }

    ~CLogOperation(void)
    {
        if (NULL != m_pOperation)
            delete m_pOperation;
    }
public:
    uint32_t m_nType;        ///< 操作类型 0: 读操作; 1:写操作，将执行本请求中的写操作且修改应用日志号; 2: 应用日志，读取日志，修改状态机，修改应用日志号
    CReadState *m_pOperation; ///< 读操作 或者写操作
    uint64_t m_u64ApplyTo;   ///< 即将应用到的日志号
};

typedef vector<Entry> EntryVec;

#include "RaftLogger.h"
#include "RaftStorage.h"
#include "RaftConfig.h"

#pragma once

#include <cstdint>
#include <climits>
#include <string>
#include <vector>
using namespace std;

const static uint32_t None = 0;
const static uint64_t noLimit = ULONG_MAX;

class CRaftErrNo
{
public:
    ///\brief 日志操作的错误号 
    enum ErrorCode
    {
        eOK = 0,                 ///< 成功
        ErrCompacted = 1,        ///< 企图读取已经被压缩后的数据
        ErrSnapOutOfDate = 2,    ///< 企图读取过期数据
        eErrUnavailable = 3,     ///< 企图读取索引号还没有过来的数据
        eErrSnapshotTemporarilyUnavailable = 4,
        eErrSeriaFail = 5
    };
    ///\brief 检查错误号是否为成功
    ///\param nErrorNo 错误号
    ///\return 成功标志 true 成功；false 失败
    static bool CRaftErrNo::Success(int nErrorNo)
    {
        return nErrorNo == CRaftErrNo::eOK;
    }

    ///\brief 根据错误号取得对应的错误信息
    ///\param nErrNo 错误号，有效值域是ErrorCode
    ///\return 对应的错误信息
    static const char* CRaftErrNo::GetErrorString(int nErrNo);
};

///\brief Raft状态机枚举值的定义
enum EStateType
{
    eStateFollower = 0,     ///< 追随者
    eStateCandidate = 1,    ///< 备选
    eStateLeader = 2,       ///< 领导者
    eStatePreCandidate = 3, ///< 预选者
    numStates               ///< 边界值
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
    uint32_t m_nLeaderID;   ///< 当前领导者ID
    EStateType m_stateRaft; ///< 当前状态（领导者，追随者，竞选者，备选者） 
};

///\brief 提供IO操作的请求，用于ReadOnly队列和写操作队列，有唯一标识标识读写操作
class CReadState
{
public:
    uint64_t m_u64Index;      ///< 对应请求时刻的提交日志号
    string   m_strRequestCtx; ///< 字符串类型的请求对象，也作为唯一标识
    ///\brief 构造函数
    ///\param u64Index 对应请求时刻的提交日志号
    ///\param strRequestCtx  字符串类型的请求对象，也作为唯一标识
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

    ///\brief 析构函数
    ~CLogOperation(void)
    {
        if (NULL != m_pOperation)
        {
            delete m_pOperation;
            m_pOperation = NULL;
        }
    }
public:
    uint32_t m_nType;         ///< 操作类型 0: 读操作; 1:写操作，将执行本请求中的写操作且修改应用日志号; 2: 应用日志，读取日志，修改状态机，修改应用日志号
    CReadState *m_pOperation; ///< 读操作 或者写操作
    uint64_t m_u64ApplyTo;    ///< 即将应用到的日志号
};
#include "RaftEntry.h"

typedef vector<CRaftEntry> EntryVec;

#include "RaftLogger.h"
#include "RaftStorage.h"
#include "RaftConfig.h"

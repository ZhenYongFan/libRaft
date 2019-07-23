#pragma once

#include "Raft.h"

using namespace std;

///\brief 一次读请求
class CReadIndexStatus
{
public:
    ///\brief 构造函数，记录读消息和当时的对应的日志提交号
    CReadIndexStatus(uint64_t u64CommitIndex, Message *pReadIndexMsg)
        : m_u64CommitIndex(u64CommitIndex),
        m_pReadIndexMsg(pReadIndexMsg)
    {
    }

    Message *m_pReadIndexMsg;      ///< 记录了对应的MsgReadIndex请求
    uint64_t m_u64CommitIndex;     ///< 该MsgReadIndex请求到达时，对应的已提交位置
    map<uint64_t, bool> m_mapAck;  ///< 记录了该MsgReadIndex相关的MsgHeartbeatResp响应的信息
};

class CReadOnly 
{
public:
  CReadOnly(ReadOnlyOption option, CLogger *pLogger);

  ~CReadOnly(void);
  
  void AddRequest(uint64_t u64Committed, Message *pMsgRead);
  
  int RecvAck(const Message& msgRead);
  
  void Advance(const Message& msgRead, vector<CReadIndexStatus*>* rss);
  
  string LastPendingRequestCtx();
public:
    ///\brief 当前只读请求的处理模式，ReadOnlySafe 或者 ReadOnlyLeaseBased
    ReadOnlyOption m_optMode; 

    ///\brief 服务端收到MsgReadIndex消息时，会为其创建一个唯一的消息ID，并作为MsgReadIndex消息的第一条Entry记录。
    ///\n     维护了消息ID与对应请求CReadIndexStatus实例的映射
    map<string, CReadIndexStatus*> m_mapPendingReadIndex;

    ///\brief 记录了MsgReadIndex请求对应的消息ID，这样可以保证MsgReadIndex的顺序
    list<string> m_listRead;

    CLogger *m_pLogger;
};

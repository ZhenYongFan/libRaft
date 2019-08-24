#pragma once
#include "libRaftCore.h"
using namespace std;

class CMessage;
class CLogger;

///\brief 一次读请求
class LIBRAFTCORE_API CReadIndexStatus
{
public:
    ///\brief 构造函数，记录读消息和当时的对应的日志提交号
    ///\param u64CommitIndex 读请求时的日志提交号
    ///\param pReadIndexMsg 读请求
    CReadIndexStatus(uint64_t u64CommitIndex, CMessage *pReadIndexMsg)       
    {
        m_u64CommitIndex = u64CommitIndex;
        m_pReadIndexMsg = pReadIndexMsg;
    }

    ///\brief 析构函数
    ~CReadIndexStatus(void);

    CMessage *m_pReadIndexMsg;    ///< 记录了对应的MsgReadIndex请求
    uint64_t m_u64CommitIndex;    ///< 该MsgReadIndex请求到达时，对应的已提交位置
    map<uint64_t, bool> m_mapAck; ///< 记录了该MsgReadIndex相关的MsgHeartbeatResp响应的信息
};

///\brief 记录读请求序列
class LIBRAFTCORE_API CReadOnly
{
public:
  ///\brief 构造函数
  CReadOnly(ReadOnlyOption option, CLogger *pLogger);

  ///\brief 析构函数
  ~CReadOnly(void);
  
  ///\brief记录读请求，保存已提交的位置以及MsgReadIndex消息
  ///\param u64Committed 已提交的位置
  ///\param pMsgRead 类型为MsgReadIndex的消息
  ///\remark 处理流程如下：\n
  /// 1.获取消息ID，在ReadIndex消息的第一个记录中记录了消息ID\n
  /// 2.判断该消息是否已经记录在pendingReadIndex中，如果已存在则直接返回\n
  /// 3.如果不存在，则维护到pendingReadIndex中，index是当前Leader已提交的位置，m是请求的消息\n
  /// 4.并将消息ID追加到readIndexQueue队列中
  void AddRequest(uint64_t u64Committed, CMessage *pMsgRead);
  
  int RecvAck(const CMessage& msgRead);
  
  void Advance(const CMessage& msgRead, vector<CReadIndexStatus*>* rss);

  ///\brief 取得队列中最后一个消息
  std::string LastPendingRequestCtx(void);
public:
    ///\brief 当前只读请求的处理模式，ReadOnlySafe 或者 ReadOnlyLeaseBased
    ReadOnlyOption m_optMode; 

    ///\brief 服务端收到MsgReadIndex消息时，会为其创建一个唯一的消息ID，并作为MsgReadIndex消息的第一条Entry记录。
    ///\n     维护了消息ID与对应请求CReadIndexStatus实例的映射
    std::map<std::string, CReadIndexStatus*> m_mapPendingReadIndex;

    ///\brief 记录了MsgReadIndex请求对应的消息ID，这样可以保证MsgReadIndex的顺序
    std::list<std::string> m_listRead;

    CLogger *m_pLogger;
};

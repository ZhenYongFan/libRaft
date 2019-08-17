#pragma once
#include "libRaftCore.h"

#include "ListenEventBase.h"
#include "RaftConfig.h"
#include <unordered_map>
class CRaft;
class CRaftLog;
class CRaftQueue;
class CLogger;
class CKvService;
class CLogOperation;

namespace google
{
    namespace protobuf
    {
        class MessageLite;
    };
};

namespace raftpb
{
    class Message;
}

class LIBRAFTCORE_API CRaftServer : public CListenEventBase
{
public:
    CRaftServer();

    virtual ~CRaftServer();

    void SetRaftNode(CRaft *pRaftNode);

    void SetMsgQueue(CRaftQueue *pQueue);

    void SetIoQueue(CRaftQueue *pQueue);

    void SetLogger(CLogger *pLogger);

    void SetKvService(CKvService *pKvService);

    void SetRaftLog(CRaftLog *pRaftLog);

    virtual bool Init(void);

    virtual void Uninit(void);

    virtual int Start(void);

    virtual void NotifyStop(void);

    virtual void Stop(void);

protected:
    bool InitSignal(void);
    bool InitTimer(void);

    static void SignalShell(evutil_socket_t tSignal, short nEvents, void *pData);
    virtual void SignalFunc(evutil_socket_t tSignal, short nEvents);

    static void TimerShell(evutil_socket_t tTimer, short nEvents, void *pData);
    virtual void TimerFunc(evutil_socket_t tSignal, short nEvents);

    static void SendMsgShell(void *pContext);
    virtual void SendMsgFunc(void);

    static void KvIoShell(void *pContext);
    virtual void KvIoFunc(void);

    int DoLogopt(CLogOperation * pLogOperation);

    virtual bool TrySendRaftMsg(uint32_t uRaftID, google::protobuf::MessageLite * pMsg);

    ///\brief 接到停止通知后，抛弃队列里的所有Message
    virtual void DiscardMessages(void);
    virtual void DiscardIoTask(void);

protected:
    virtual CIoEventBase* CreateIoBase(void);
    virtual void DestroyIoBase(CIoEventBase* pIoBase);

    virtual void ListenEventFunc(struct evconnlistener *pListener, evutil_socket_t fdSocket,
        struct sockaddr *pSockAddr, int nSockLen);
    virtual uint32_t CreateSessionID(void);
protected:
    struct event *m_pSignalEvent;  ///< 信号事件
    struct event *m_pTimerEvent;   ///< 定时器事件
    CRaft *m_pRaftNode;            ///< Raft状态机
    CRaftQueue *m_pMsgQueue;       ///< 消息队列
    CRaftQueue *m_pIoQueue;        ///< 消息队列
    CLogger *m_pLogger;            ///< 日志对象
    CKvService *m_pKvService;      ///< KV服务
    CRaftLog *m_pRaftLog;          ///< Raft日志
    std::thread *m_pThreadMsg;     ///< 处理消息的线程
    std::thread *m_pThreadIo;      ///< 处理IO的线程
    int m_nTryTicks;
    std::mutex m_mutexSession;
    std::unordered_map<uint32_t, CEventSession *> m_mapNodes;
public:
    CRaftConfig m_cfgSelf;        ///< 配置信息
};


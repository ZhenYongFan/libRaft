#pragma once
#include "libRaftExt.h"

#include "ListenEventBase.h"
#include "RaftConfig.h"

class CRaftFrame;
class CLogOperation;
class CMessage;


class LIBRAFTEXT_API CRaftServer : public CListenEventBase
{
public:
    CRaftServer();

    virtual ~CRaftServer();

    virtual bool Init(CRaftFrame *pRaftFrame);

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

    virtual bool TrySendRaftMsg(uint32_t uRaftID,std::string &strMsg);

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
    
    CRaftFrame *m_pRaftFrame;      ///< Raft框架

    std::thread *m_pThreadMsg;     ///< 处理消息的线程
    std::thread *m_pThreadIo;      ///< 处理IO的线程
    int m_nTryTicks;
    std::mutex m_mutexSession;
    std::unordered_map<uint32_t, CEventSession *> m_mapNodes;
};


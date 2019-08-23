#pragma once
#include "libRaftExt.h"

#include "RaftIoBase.h"
#include "RaftConfig.h"
#include "RaftSemaphore.h"
class CLogger;
class CRaftClient;

namespace raftpb
{
    class Message;
};

namespace raftserverpb
{
    class ResponseOp;
    class RequestOp;
};

class LIBRAFTEXT_API CRaftClientPool:public CRaftIoBase
{
public:
    CRaftClientPool(CSequenceID *pSessionSeqID);

    virtual ~CRaftClientPool(void);

    virtual bool Init(void);

    virtual void Uninit(void);

    virtual int Start(void);

    virtual void NotifyStop(void);

    virtual void Stop(void);

    uint32_t RegClient(CRaftClient *pClient);

    bool UnRegClient(CRaftClient *pClient);
    
    raftserverpb::ResponseOp * SendReq(raftserverpb::RequestOp *pRequest,uint32_t dwTimeoutMs);

    bool Wake(raftserverpb::ResponseOp *pResponse);

    void SetLogger(CLogger *pLogger);

protected:
    bool InitSubSession(void);

    bool InitSignal(void);
    bool InitTimer(void);

    static void SignalShell(evutil_socket_t tSignal, short nEvents, void *pData);
    virtual void SignalFunc(evutil_socket_t tSignal, short nEvents);

    static void TimerShell(evutil_socket_t tTimer, short nEvents, void *pData);
    virtual void TimerFunc(evutil_socket_t tSignal, short nEvents);

    virtual CEventSession *CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);
    CEventSession *GetActiveSession(void);

    struct event *m_pSignalEvent;  ///< 信号事件
    struct event *m_pTimerEvent;   ///< 定时器事件

    CLogger *m_pLogger;            ///< 日志对象
    int m_nTryTicks;
    int m_nActiveNode;
    std::mutex m_mutexSession;
    std::unordered_map<uint32_t, CEventSession *> m_mapNodes;

    int m_nMaxSubSessions;
    int m_nSubSessionSeq;
    CRaftSemaphore m_semSubSessions;
    std::mutex m_mutexSubSession;
    std::unordered_map<uint32_t, raftserverpb::ResponseOp *> m_mapResponses;
    std::unordered_map<uint32_t, CRaftSemaphore *> m_mapSubSession;
    std::list<CRaftSemaphore *> m_listSem;
public:
    CRaftConfig m_cfgSelf;        ///< 配置信息
};


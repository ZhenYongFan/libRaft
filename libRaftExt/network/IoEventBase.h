#pragma once
#include "libRaftExt.h"
#include "EventBase.h"

class CSequenceID;
class CEventSession;

///\brief 负责IO操作的EventBase
class LIBRAFTEXT_API CIoEventBase:public CEventBase
{
public:
    ///\brief 构造函数
    ///\param pSessionSeqID 流水号管理器
    CIoEventBase(CSequenceID *pSessionSeqID);

    ///\brief 析构函数
    virtual ~CIoEventBase(void);

    ///\brief 释放资源
    virtual void Uninit(void);
    
    virtual CEventSession * Connect(const std::string &strHost, int nPort, uint32_t nSessionID);

    virtual CEventSession *CreateServiceSession(struct bufferevent *pBufferEvent);

    virtual CEventSession *CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);

    virtual bool DestroySession(CEventSession *pSession);

protected:

    CSequenceID *m_pSessionSeqID; ///< 流水号管理器

    std::unordered_map<uint32_t,CEventSession *> m_mapSession; ///< Session集合

    std::mutex m_mutexSession;    ///< Session集合的多线程锁
};

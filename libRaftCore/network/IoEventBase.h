#pragma once
#include "libRaftCore.h"
#include "EventBase.h"

class CSequenceID;
class CEventSession;

///\brief 负责IO操作的EventBase
class LIBRAFTCORE_API CIoEventBase:public CEventBase
{
public:
    ///\brief 构造函数
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

    CSequenceID *m_pSessionSeqID;

    std::mutex m_mutexSession;

    std::unordered_map<uint32_t,CEventSession *> m_mapSession;
};

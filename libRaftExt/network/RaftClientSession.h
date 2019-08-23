#pragma once
#include "RaftSession.h"
class CRaftClientPool;

class CRaftClientSession :public CRaftSession
{
public:
    CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID);
    CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);

    virtual ~CRaftClientSession();
    void SetPool(CRaftClientPool *pClientPool);

protected:
    virtual bool PushMessage(std::string &strMsgData);
protected:
    CRaftClientPool *m_pClientPool;
};

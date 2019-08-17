#pragma once
#include "RaftSession.h"
class CRaftClientPool;

class CRaftClientSession :public CRaftSession
{
public:
    CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent);
    CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort);

    virtual ~CRaftClientSession();
    void SetPool(CRaftClientPool *pClientPool);

protected:
    virtual bool PushMessage(std::string &strMsgData);
protected:
    CRaftClientPool *m_pClientPool;
};

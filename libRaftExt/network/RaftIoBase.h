#pragma once
#include "libRaftCore.h"

#include "IoEventBase.h"
class CRaftQueue;

namespace google
{
    namespace protobuf
    {
        class MessageLite;
    };
};

class LIBRAFTCORE_API CRaftIoBase : public CIoEventBase
{
public:
    CRaftIoBase(CSequenceID *pSessionSeqID);
    
    virtual ~CRaftIoBase();
    
    void SetMsgQueue(CRaftQueue *pQueue);

    bool TrySendRaftMsg(uint32_t uRaftID, google::protobuf::MessageLite * pMsg);
protected:
    virtual CEventSession *CreateServiceSession(struct bufferevent *pBufferEvent);

    virtual CEventSession *CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);

protected:
    CRaftQueue *m_pMsgQueue; ///消息队列
};

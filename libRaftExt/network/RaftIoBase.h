#pragma once
#include "libRaftExt.h"
#include "IoEventBase.h"

class CRaftQueue;
class CMessage;
class CRaftSerializer;

class LIBRAFTEXT_API CRaftIoBase : public CIoEventBase
{
public:
    CRaftIoBase(CSequenceID *pSessionSeqID);
    
    virtual ~CRaftIoBase();
    
    void SetMsgQueue(CRaftQueue *pQueue);

    void SetSerializer(CRaftSerializer *pSerializer)
    {
        m_pSerializer = pSerializer;
    }

    CRaftSerializer *GetSerializer(void)
    {
        return m_pSerializer;
    }

    int TrySendRaftMsg(uint32_t uRaftID, std::string &strMsg);
protected:
    virtual CEventSession *CreateServiceSession(struct bufferevent *pBufferEvent);

    virtual CEventSession *CreateClientSession(struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);

protected:
    CRaftQueue *m_pMsgQueue;        ///< 消息队列
    CRaftSerializer *m_pSerializer; ///< 串行化对象
};

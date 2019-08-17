#pragma once
#include "libRaftCore.h"
#include "EventSession.h"

class CRaftQueue;

class LIBRAFTCORE_API CRaftSession : public CEventSession
{
public:
    CRaftSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID);
    CRaftSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID);
    virtual ~CRaftSession(void);

    ///\attention 从Raft算法角度，允许发送失败
    int SendMsg(std::string &strMsg);
    
    ///\brief 尝试重新连接
    virtual int RetryConnect(void);
    
    void SetMsgQueue(CRaftQueue *pQueue);
    
    void SetTypeID(uint32_t nTypeID);
    
    uint32_t GetTypeID(void)
    {
        return m_nTypeID;
    };
protected:
    virtual void ReadEventFunc(struct bufferevent *pBev);

    bool ReadLen(struct evbuffer * pInput);
    bool ReadCtx(struct evbuffer * pInput);

    virtual bool PushMessage(std::string &strMsgData);

    virtual void WriteEventFunc(struct bufferevent *pBev);

    virtual void OnEventConnected(struct bufferevent *pBev);
    virtual void OnEventEof(struct bufferevent *pBev);
    virtual void OnEventError(struct bufferevent *pBev);
    virtual void OnEventOther(struct bufferevent *pBev, short nWhat);
    virtual void OnEventDisconnected(struct bufferevent *pBev);
protected:
    //一个std::string的读取
    int m_nReadLenOffset;
    unsigned char m_byLens[4];
    int m_nReadOffset;
    std::string m_strReadCtx;
    CRaftQueue *m_pMsgQueue;
    uint32_t m_nTypeID; //0:和Raft节点通讯的Session；1: 和客户端通信的Session；2: 和服务端通信的Session
};


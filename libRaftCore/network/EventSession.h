#pragma once
#include "libRaftCore.h"

class CIoEventBase;

///\brief 通过libevent建立的会话
class LIBRAFTCORE_API CEventSession
{
public:
    ///\brief 会话当前状态
    enum EState
    {
        eConnecting = 1,    ///< 正在连接中
        eConnected = 2,     ///< 连接成功
        eDisConnected = 3,  ///< 连接断开
        eEventError = 4,    ///< 连接发生严重错误
    };
public:
    ///\brief 构造函数
    CEventSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID);

    CEventSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent,const std::string &strHost, int nPort, uint32_t nSessionID);
    
    virtual ~CEventSession(void);

    CIoEventBase *GetIoBase(void)
    {
        return m_pIoBase;
    }

    struct bufferevent *GetBufferEvent(void)
    {
        return m_pBufferEvent;
    }
    
    EState GetState(void)
    {
        return m_eState;
    }

    bool IsClient(void)
    {
        return !m_strHost.empty();
    }

    uint32_t GetSessionID(void)
    {
        return m_nSessionID;
    }

    bool IsConnected(void)
    {
        return (eConnected == m_eState);
    };

    virtual int Connect(const std::string &strHost, int nPort);
    
    virtual int RetryConnect(void);

    static void ReadEventShell(struct bufferevent *pBev, void *pContext);
    static void WriteEventShell(struct bufferevent *pBev, void *pContext);
    static void EventShell(struct bufferevent *pBev, short what, void *pContext);

    virtual void ReadEventFunc(struct bufferevent *pBev);
    virtual void WriteEventFunc(struct bufferevent *pBev);
    virtual void EventFunc(struct bufferevent *pBev, short nWhat);

protected:
    virtual void OnReadTimeout(struct bufferevent *pBev);
    virtual void OnWriteTimeout(struct bufferevent *pBev);
    virtual void OnEventTimeout(struct bufferevent *pBev);
    virtual void OnEventConnected(struct bufferevent *pBev);
    virtual void OnEventDisconnected(struct bufferevent *pBev);
    virtual void OnEventEof(struct bufferevent *pBev);
    virtual void OnEventError(struct bufferevent *pBev);
    virtual void OnEventOther(struct bufferevent *pBev, short nWhat);
protected:
    CIoEventBase *m_pIoBase;            ///< IO管理者
    struct bufferevent *m_pBufferEvent; ///< libevent 句柄
    EState m_eState;                    ///< 当前状态
    std::string m_strHost;              ///< 目标服务器域名或者IPV4或者IPV6
    int m_nPort;                        ///< 端口
    uint32_t m_nSessionID;              ///< 唯一编号
};


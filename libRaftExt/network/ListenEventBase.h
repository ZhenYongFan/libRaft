#pragma once
#include "libRaftExt.h"
#include "EventBase.h"
#include "SequenceID.h"

class CIoEventBase;
class CEventSession;

class LIBRAFTEXT_API CListenEventBase :public CEventBase
{
public:
    CListenEventBase();

    virtual ~CListenEventBase();

    bool AddListenCfg(const std::string &strHost, int nPort);

    bool DelListenCfg(const std::string &strHost, int nPort);

    void ClearListenCfg(void);

    virtual bool Init(void);

    virtual void Uninit(void);

    virtual int Start(void);

    virtual void NotifyStop(void);

    virtual void Stop(void);

protected:
    static void ListenEventShell(struct evconnlistener *pListener, evutil_socket_t fdSocket,
        struct sockaddr *pSockAddr, int nSockLen, void *pUserData);

    static void ListenErrorShell(struct evconnlistener *pListener, void *pUserData);

    virtual void ListenEventFunc(struct evconnlistener *pListener, evutil_socket_t fdSocket,
        struct sockaddr *pSockAddr, int nSockLen);

    virtual void ListenErrorFunc(struct evconnlistener *pListener);

    virtual CEventSession * CreateEvent(CIoEventBase *pIoBase, struct evconnlistener *pListener,
        evutil_socket_t fdSocket, struct sockaddr *pSockAddr, int nSockLen);

    struct evconnlistener * InitListener(std::string &strHost, int nPort);

    virtual CIoEventBase* CreateIoBase(void);

    virtual void DestroyIoBase(CIoEventBase* pIoBase);

    int FindHost(const std::string & strHost, int nPort);

protected:
    std::vector<struct evconnlistener *> m_apListener; ///< 监听对象数组
    std::vector<std::string> m_astrHost;               ///< 监听域名数组
    std::vector<int> m_anPort;                         ///< 监听端口数组
    CIoEventBase *m_pIoBase; ///< 将来支持多个IO对象
    CSequenceID m_seqIDs;    ///< 序列号生成器
};


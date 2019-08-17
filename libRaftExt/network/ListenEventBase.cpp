#include "stdafx.h"
#include "ListenEventBase.h"
#include "IoEventBase.h"

CListenEventBase::CListenEventBase()
    :m_seqIDs(1024)
{
    m_pIoBase = NULL;
}

CListenEventBase::~CListenEventBase()
{
}

bool CListenEventBase::AddListenCfg(const std::string &strHost, int nPort)
{
    int nFound = FindHost(strHost, nPort);
    if(nFound == -1)
    {
        m_astrHost.push_back(strHost);
        m_anPort.push_back(nPort);
    }
    return (nFound == -1);
}

bool CListenEventBase::DelListenCfg(const std::string &strHost, int nPort)
{
    int nFound = FindHost(strHost, nPort);
    if (nFound != -1)
    {
        m_astrHost.erase(m_astrHost.begin() + nFound);
        m_anPort.erase(m_anPort.begin() + nFound);
    }
    return (nFound != -1);
}

void CListenEventBase::ClearListenCfg(void)
{
    m_astrHost.clear();
    m_anPort.clear();
}

int CListenEventBase::FindHost(const std::string & strHost, int nPort)
{
    int nFound = -1;
    for (int nIndex = 0; nIndex < int(m_astrHost.size()); nIndex++)
    {
        if (m_astrHost[nIndex] == strHost && m_anPort[nIndex] == nPort)
        {
            nFound = nIndex;
            break;
        }
    }    
    return nFound;
}

bool CListenEventBase::Init(void)
{
    bool bInit = false;
    if (CEventBase::Init())
    {
        m_pIoBase = CreateIoBase();
        if (m_pIoBase->Init())
        {
            bInit = true;
            for (size_t nIndex = 0; bInit && nIndex < m_astrHost.size(); nIndex++)
            {
                std::string strHost = m_astrHost[nIndex];
                int nPort = m_anPort[nIndex];
                struct evconnlistener *pListener = InitListener(strHost, nPort);
                if (NULL != pListener)
                    m_apListener.push_back(pListener);
                else
                    bInit = false;
            }
        }
        else
        {
            DestroyIoBase(m_pIoBase);
            m_pIoBase = NULL;
        }
    }
    if (!bInit)
        Uninit();
    return bInit;
}

CIoEventBase* CListenEventBase::CreateIoBase(void)
{
    CIoEventBase* pIoEvent = new CIoEventBase(&m_seqIDs);
    return pIoEvent;
}

void CListenEventBase::DestroyIoBase(CIoEventBase* pIoBase)
{
    delete pIoBase;
}

struct evconnlistener *  CListenEventBase::InitListener(std::string &strHost, int nPort)
{
    struct evconnlistener *pListener = NULL;
    char pstrPort[16];
    snprintf(pstrPort, sizeof(pstrPort), "%d", nPort);
    struct evutil_addrinfo addrInfo;
    struct evutil_addrinfo *pAddrRs;
    memset(&addrInfo, 0, sizeof(struct addrinfo));
    addrInfo.ai_family = AF_UNSPEC;         /* Allow IPv4 or IPv6 */
    addrInfo.ai_socktype = SOCK_STREAM;     /* Datagram socket */
    addrInfo.ai_flags = AI_PASSIVE;         /* For wildcard IP address */
    addrInfo.ai_protocol = 0;               /* Any protocol */
    addrInfo.ai_canonname = NULL;
    addrInfo.ai_addr = NULL;
    addrInfo.ai_next = NULL;
    int nGetAddr = evutil_getaddrinfo(strHost.c_str(),pstrPort, &addrInfo, &pAddrRs);
    if (0 == nGetAddr)
    {
        struct evutil_addrinfo *pAddr = NULL;
        // Pick the ipv6 address first since ipv4 addresses can be mapped
        // into ipv6 space.
        for (pAddr = pAddrRs; NULL != pAddr; pAddr = pAddr->ai_next)
        {
            if (pAddr->ai_family == AF_INET6 || pAddr->ai_next == NULL)
                break;
        }
        if (NULL != pAddr)
        {
            pListener = evconnlistener_new_bind(m_pBase, ListenEventShell, (void *)this,
                LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                pAddr->ai_addr, int(pAddr->ai_addrlen));
            if(NULL != pListener)
                evconnlistener_set_error_cb(pListener, ListenErrorShell);
        }
        evutil_freeaddrinfo(pAddrRs);
    }
    return pListener;
}

void CListenEventBase::Uninit(void)
{
    for(size_t nIndex = 0 ; nIndex < m_apListener.size() ; nIndex ++)
    {
        struct evconnlistener *pListener = m_apListener[nIndex];
        evconnlistener_free(pListener);
    }
    m_apListener.clear();
    if (NULL != m_pIoBase)
    {
        m_pIoBase->Uninit();
        delete m_pIoBase;
        m_pIoBase = NULL;
    }
    CEventBase::Uninit();
}

int CListenEventBase::Start(void)
{
    int nStart = m_pIoBase->Start();
    if (0 == nStart)
        nStart = CEventBase::Start();
    return nStart;
}

void CListenEventBase::NotifyStop(void)
{
    CEventBase::NotifyStop();
    m_pIoBase->NotifyStop();
}

void CListenEventBase::Stop(void)
{
    CEventBase::Stop();
    m_pIoBase->Stop();
}

void CListenEventBase::ListenEventShell(struct evconnlistener *pListener, evutil_socket_t fdSocket,
    struct sockaddr *pSockAddr, int nSockLen, void *pUserData)
{
    CListenEventBase *pBase = (CListenEventBase*)pUserData;
    pBase->ListenEventFunc(pListener, fdSocket, pSockAddr, nSockLen);
}

void CListenEventBase::ListenErrorShell(struct evconnlistener *pListener, void *pUserData)
{
    CListenEventBase *pBase = (CListenEventBase*)pUserData;
    pBase->ListenErrorFunc(pListener);
}

void CListenEventBase::ListenEventFunc(struct evconnlistener *pListener, evutil_socket_t fdSocket,
    struct sockaddr *pSockAddr, int nSockLen)
{
    CEventSession *pSession = CreateEvent(m_pIoBase, pListener, fdSocket, pSockAddr, nSockLen);
    if(NULL == pSession)
    {
        //出现异常，在合适的时机，由主线程停止子线程
    }
}

void CListenEventBase::ListenErrorFunc(struct evconnlistener *pListener)
{

}

CEventSession * CListenEventBase::CreateEvent(CIoEventBase *pIoBase,struct evconnlistener *pListener,
    evutil_socket_t fdSocket,struct sockaddr *pSockAddr, int nSockLen)
{
    CEventSession *pSession = NULL;
    struct event_base *pBase = pIoBase->GetBase();
    struct bufferevent *pBev = bufferevent_socket_new(pBase, fdSocket, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    if (NULL != pBev)
    {
        pSession = pIoBase->CreateServiceSession(pBev);
        if(NULL != pSession)
            bufferevent_enable(pBev, EV_READ | EV_WRITE);
    }
    return pSession;
}

#include "stdafx.h"
#include "EventSession.h"
#include "IoEventBase.h"

CEventSession::CEventSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID)
    :m_pIoBase(pIoBase),m_pBufferEvent(pBufferEvent),m_nSessionID(nSessionID)
{
    bufferevent_setcb(pBufferEvent, ReadEventShell, WriteEventShell, EventShell, this);
    m_eState = eConnected;
    m_nSessionID = 0;
}

CEventSession::CEventSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent,const std::string &strHost, int nPort, uint32_t nSessionID)
    :m_pIoBase(pIoBase), m_pBufferEvent(pBufferEvent),m_nSessionID(nSessionID)
{
    m_nSessionID = 0;
    m_strHost = strHost;
    m_nPort = nPort;
    m_eState = eEventError;
    bufferevent_setcb(pBufferEvent, ReadEventShell, WriteEventShell, EventShell, this);
}

CEventSession::~CEventSession()
{
}

int CEventSession::Connect(const std::string &strHost, int nPort)
{
    int nConnect = -1;
    if (IsClient() && eEventError == m_eState)
    {
        nConnect = bufferevent_socket_connect_hostname(m_pBufferEvent, NULL, AF_UNSPEC, strHost.c_str(), nPort);
        if (0 == nConnect)
        {
            m_eState = eConnecting;
            m_strHost = strHost;
            m_nPort = nPort;
        }
    }
    return nConnect;
}

int CEventSession::RetryConnect(void)
{
    int nConnect = -1;
    if (IsClient() && eEventError == m_eState)
    {
        nConnect = bufferevent_socket_connect_hostname(m_pBufferEvent, NULL, AF_UNSPEC, m_strHost.c_str(), m_nPort);
        if (0 == nConnect)
            m_eState = eConnecting;
    }
    return nConnect;
}

void CEventSession::ReadEventShell(struct bufferevent *pBev, void *pContext)
{
    CEventSession *pSession = (CEventSession *)pContext;
    pSession->ReadEventFunc(pBev);
}

void CEventSession::WriteEventShell(struct bufferevent *pBev, void *pContext)
{
    CEventSession *pSession = (CEventSession *)pContext;
    pSession->WriteEventFunc(pBev);
}

void CEventSession::EventShell(struct bufferevent *pBev, short nWhat, void *pContext)
{
    CEventSession *pSession = (CEventSession *)pContext;
    pSession->EventFunc(pBev, nWhat);
}

void CEventSession::ReadEventFunc(struct bufferevent *pBev)
{

}

void CEventSession::WriteEventFunc(struct bufferevent *pBev)
{

}

void CEventSession::OnReadTimeout(struct bufferevent *pBev)
{

}

void CEventSession::OnWriteTimeout(struct bufferevent *pBev)
{

}

void CEventSession::OnEventDisconnected(struct bufferevent *pBev)
{
    m_pIoBase->DestroySession(this);
}

void CEventSession::OnEventEof(struct bufferevent *pBev)
{
    m_eState = eDisConnected;
    OnEventDisconnected(pBev);
}

void CEventSession::OnEventError(struct bufferevent *pBev)
{
    m_eState = eEventError;
    OnEventDisconnected(pBev);
}

void CEventSession::OnEventOther(struct bufferevent *pBev, short nWhat)
{
    m_eState = eEventError;
    OnEventDisconnected(pBev);
}

void CEventSession::OnEventTimeout(struct bufferevent *pBev)
{
}

void CEventSession::OnEventConnected(struct bufferevent *pBev)
{
    bufferevent_enable(pBev, EV_READ | EV_WRITE);
    m_eState = eConnected;
}

void CEventSession::EventFunc(struct bufferevent *pBev, short nWhat)
{
    if (nWhat & BEV_EVENT_TIMEOUT)
    {
        if (nWhat & BEV_EVENT_READING)
            OnReadTimeout(pBev);
        else if (nWhat & BEV_EVENT_WRITING)
            OnWriteTimeout(pBev);
        else
            OnEventTimeout(pBev);
    }
    else if (nWhat & BEV_EVENT_CONNECTED)
        OnEventConnected(pBev);
    else if (nWhat & BEV_EVENT_EOF)
        OnEventEof(pBev);
    else if (nWhat & BEV_EVENT_ERROR)
        OnEventError(pBev);
    else
        OnEventOther(pBev, nWhat);
}

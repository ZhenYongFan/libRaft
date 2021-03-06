#include "stdafx.h"
#include "RaftSession.h"
#include "RaftIoBase.h"
#include "RaftEntry.h"
#include <event2/util.h>
#include <event2/buffer.h>
#include <string.h>

#include "RequestOp.h"
#include "RaftQueue.h"
#include "protobuffer/RaftProtoBufferSerializer.h"
#include "RequestOp.h"

CRaftSession::CRaftSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID)
    :CEventSession(pIoBase,pBufferEvent, nSessionID)
{
    m_nTypeID = 0;
    m_pMsgQueue = NULL;
    m_nReadOffset = 0;
    m_nReadLenOffset = 0;
}

CRaftSession::CRaftSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID)
    : CEventSession(pIoBase, pBufferEvent, strHost,nPort, nSessionID)
{
    m_nTypeID = 0;
    m_pMsgQueue = NULL;
    m_nReadOffset = 0;
    m_nReadLenOffset = 0;
}

CRaftSession::~CRaftSession()
{
}

void CRaftSession::SetMsgQueue(CRaftQueue *pQueue)
{
    m_pMsgQueue = pQueue;
}

void CRaftSession::SetTypeID(uint32_t nTypeID)
{
    m_nTypeID = nTypeID;
}

int CRaftSession::RetryConnect(void)
{
    int nConnect = 1;
    if (IsClient() && (eEventError == m_eState || eDisConnected == m_eState))
    {
        nConnect = 2;
        struct event_base *pBase = m_pIoBase->GetBase();
        m_pBufferEvent = bufferevent_socket_new(pBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
        if (NULL != m_pBufferEvent)
        {
            bufferevent_setcb(m_pBufferEvent, ReadEventShell, WriteEventShell, EventShell, this);
            m_eState = eConnecting;
            if (0 == bufferevent_socket_connect_hostname(m_pBufferEvent, NULL, AF_UNSPEC, m_strHost.c_str(), m_nPort))               
                nConnect = 0;
            else
                m_eState = eEventError;
        }
    }
    return nConnect;
}

int CRaftSession::SendMsg(std::string &strMsg)
{
    int nSend = 1;
    if (m_eState == CEventSession::eConnected)
    {
        int nLen = int(strMsg.length());
        if (0 == bufferevent_write(m_pBufferEvent, &nLen, sizeof(nLen)))
        {
            if (0 == bufferevent_write(m_pBufferEvent, strMsg.c_str(), strMsg.length()))
                nSend = 0;
        }
    }
    return nSend;
}

void CRaftSession::OnEventConnected(struct bufferevent *pBev)
{
    CEventSession::OnEventConnected(pBev);
    printf("connected \n");
}

void CRaftSession::OnEventEof(struct bufferevent *pBev)
{
    CEventSession::OnEventEof(pBev);
    printf("eof \n");
}

void CRaftSession::OnEventError(struct bufferevent *pBev)
{
    evutil_socket_t socketBuf = bufferevent_getfd(pBev);
    int nErrorNo = evutil_socket_geterror(socketBuf);
    const char *pstrErrMsg = evutil_socket_error_to_string(nErrorNo);
    
    printf("error: %s \n", pstrErrMsg);
    CEventSession::OnEventError(pBev);
}

void CRaftSession::OnEventOther(struct bufferevent *pBev, short nWhat)
{
    CEventSession::OnEventOther(pBev, nWhat);
    printf("other error\n");
}

void CRaftSession::OnEventDisconnected(struct bufferevent *pBev)
{
    if (!IsClient())
        m_pIoBase->DestroySession(this);
    else
    {
        bufferevent_free(m_pBufferEvent);
        m_pBufferEvent = NULL;
    }
}

void CRaftSession::ReadEventFunc(struct bufferevent *pBev)
{
    int nRead = 0;
    bool bMore = false;
    struct evbuffer *pInput = bufferevent_get_input(pBev);
    do
    {
        if (m_strReadCtx.size() == 0)
        {
            bMore = ReadLen(pInput);
            if (bMore)
            {
                int nDataLen;
                memcpy(&nDataLen, m_byLens, sizeof(int));
                m_strReadCtx.resize(nDataLen);
                bMore = ReadCtx(pInput);
            }
        }
        else
        {
            bMore = ReadCtx(pInput);
        }
        if (bMore)
        {
            if (PushMessage(m_strReadCtx))
            {
                nRead++;
                m_strReadCtx.clear();
                m_nReadOffset = 0;
                m_nReadLenOffset = 0;
            }
            else
            {
                nRead = -1;
                break;
            }
        }
    } while (bMore);
}

bool CRaftSession::ReadLen(struct evbuffer * pInput)
{
    bool bMore = false;
    size_t nLen = evbuffer_get_length(pInput);
    size_t nWantRead = sizeof(int) - m_nReadLenOffset;
    if (nLen >= nWantRead)
    {
        evbuffer_copyout(pInput, m_byLens + m_nReadLenOffset, nWantRead);
        evbuffer_drain(pInput, nWantRead);
        bMore = true;
    }
    else if(nLen > 0)
    {
        evbuffer_copyout(pInput, m_byLens + m_nReadLenOffset, nLen);
        m_nReadLenOffset += int(nLen);
        evbuffer_drain(pInput, nLen);
    }
    return bMore;
}

bool CRaftSession::ReadCtx(struct evbuffer * pInput)
{
    bool bMore = false;
    size_t nLen = evbuffer_get_length(pInput);
    size_t nWantRead = m_strReadCtx.size() - m_nReadOffset;
    if (nLen >= nWantRead)
    {
        evbuffer_copyout(pInput, &m_strReadCtx[m_nReadOffset], nWantRead);
        evbuffer_drain(pInput, nWantRead);
        bMore = true;
    }
    else if(nLen > 0)
    {
        evbuffer_copyout(pInput, &m_strReadCtx[m_nReadOffset], nLen);
        m_nReadOffset += int(nLen);
        evbuffer_drain(pInput, nLen);
    }        
    return bMore;
}

bool CRaftSession::PushMessage(std::string &strMsgData)
{
    bool bPush = false;
    CRaftIoBase *pRaftBase = dynamic_cast<CRaftIoBase *>(GetIoBase());
    CRaftProtoBufferSerializer *pSerializer = dynamic_cast<CRaftProtoBufferSerializer *>(pRaftBase->GetSerializer());
    if (0 == m_nTypeID)
    {
        CMessage *pMsg = new CMessage();
        if (pSerializer->ParseMessage(*pMsg, strMsgData))
        {
            m_pMsgQueue->Push(pMsg);
            bPush = true;
        }
        else
            delete pMsg;
    }
    else if(1 == m_nTypeID)
    {
        CRequestOp *pRequest = new CRequestOp();
        if (pSerializer->ParseRequestOp(*pRequest,strMsgData))
        {
            pRequest->set_clientid(m_nSessionID);
            CRequestOp::ERequestCase typeRequest = pRequest->request_case();
            CMessage::EMessageType typeMsg = CMessage::MsgNoUsed;
            if (typeRequest == CRequestOp::kRequestPut || typeRequest == CRequestOp::kRequestDeleteRange)
                typeMsg = CMessage::MsgProp;
            else if (typeRequest == CRequestOp::kRequestRange)
                typeMsg = CMessage::MsgReadIndex;
            pSerializer->SerializeRequestOp(*pRequest, strMsgData);

            CMessage *pMsg = new CMessage();
            pMsg->set_to(0);
            pMsg->set_from(0);
            pMsg->set_term(0);
            pMsg->set_logterm(0);
            pMsg->set_index(0);
            pMsg->set_type(typeMsg);
            CRaftEntry *pEntry = pMsg->add_entries();
            pEntry->set_data(strMsgData);
            m_pMsgQueue->Push(pMsg);
            bPush = true;
        }
        delete pRequest;
    }
    else
    {

    }
    return bPush;
}

void CRaftSession::WriteEventFunc(struct bufferevent *pBev)
{

}

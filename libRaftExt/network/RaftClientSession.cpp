#include "stdafx.h"
#include "RaftClientSession.h"
#include "RaftClientPool.h"
#include "rpc.pb.h"
using namespace raftserverpb;

CRaftClientSession::CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent)
    :CRaftSession(pIoBase,pBufferEvent)
{

}
CRaftClientSession::CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort)
    : CRaftSession(pIoBase,pBufferEvent,strHost,nPort)
{

}

CRaftClientSession::~CRaftClientSession()
{
}

void CRaftClientSession::SetPool(CRaftClientPool *pClientPool)
{
    m_pClientPool = pClientPool;
}

bool CRaftClientSession::PushMessage(std::string &strMsgData)
{
    ResponseOp *pResponse = new ResponseOp();
    pResponse->ParseFromString(strMsgData);
    bool bWake = m_pClientPool->Wake(pResponse);
    if (!bWake)
        delete pResponse;
    return bWake;
}

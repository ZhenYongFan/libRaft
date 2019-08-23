#include "stdafx.h"
#include "RaftClientSession.h"
#include "RaftClientPool.h"
#include "protobuffer/rpc.pb.h"
using namespace raftserverpb;

CRaftClientSession::CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, uint32_t nSessionID)
    :CRaftSession(pIoBase,pBufferEvent,nSessionID)
{

}
CRaftClientSession::CRaftClientSession(CIoEventBase *pIoBase, struct bufferevent *pBufferEvent, const std::string &strHost, int nPort, uint32_t nSessionID)
    : CRaftSession(pIoBase,pBufferEvent,strHost,nPort,nSessionID)
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

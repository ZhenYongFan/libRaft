#include "stdafx.h"
#include "RaftAdminClient.h"
#include "RaftClientPool.h"
#include "rpc.pb.h"
using namespace raftserverpb;

CRaftClient::CRaftClient()
{
    m_uSubSessionID = 0;
    m_pClientPool = NULL;
}

CRaftClient::~CRaftClient()
{
}

bool CRaftClient::Init(CRaftClientPool *pClientPool)
{
    bool bInit = false;
    if (NULL != pClientPool && NULL == m_pClientPool)
    {
        m_pClientPool = pClientPool;
        m_uSubSessionID = m_pClientPool->RegClient(this);
        bInit = (0 != m_uSubSessionID);
    }
    return bInit;
}

void CRaftClient::Uninit(void)
{
    if (NULL != m_pClientPool)
    {
        m_pClientPool->UnRegClient(this);
        m_pClientPool = NULL;
    }
}

void CRaftClient::SetSubSessionID(uint32_t uSubSessionID)
{
    m_uSubSessionID = uSubSessionID;
}

uint32_t CRaftClient::GetSubSessionID(void)
{
    return m_uSubSessionID;
}

int CRaftClient::Put(const std::string &strKey, const std::string &strValue)
{
    int nPut = 0;
    RequestOp *pRequest = new RequestOp();
    pRequest->set_subsessionid(m_uSubSessionID);
    PutRequest *pPutRequest = pRequest->mutable_request_put();
    pPutRequest->set_key(strKey);
    pPutRequest->set_value(strValue);
    uint32_t dwTimeoutMs = INFINITE;
    ResponseOp *pResponse = m_pClientPool->SendReq(pRequest, dwTimeoutMs);
    if (NULL != pResponse)
    {
        const PutResponse& resPut = pResponse->response_put();
        nPut = pResponse->errorno();
        delete pResponse;
    }
    else
        nPut = 1;
    delete pRequest;
    return nPut;
}

int CRaftClient::Get(const std::string &strKey, std::string &strValue)
{
    int nGet = 0;
    RequestOp *pRequest = new RequestOp();
    pRequest->set_clientid(m_uSubSessionID);
    RangeRequest *pRangeRequest = pRequest->mutable_request_range();
    pRangeRequest->set_key(strKey);
    uint32_t dwTimeoutMs = INFINITE;
    ResponseOp *pResponse = m_pClientPool->SendReq(pRequest, dwTimeoutMs);
    if (NULL != pResponse)
    {
        const RangeResponse& resRange = pResponse->response_range();
        nGet = pResponse->errorno();
        if (0 == nGet)
        {
            const google::protobuf::RepeatedPtrField<::raftserverpb::KeyValue>&rsFound = resRange.kvs();
            if (!rsFound.empty())
                strValue = (*rsFound.begin()).value();
            else
                nGet = 1;
        }
        delete pResponse;
    }
    else
        nGet = 1;
    delete pRequest;
    return nGet;
}

int CRaftClient::Delete(const std::string &strKey)
{
    int nDelete = 0;
    RequestOp *pRequest = new RequestOp();
    pRequest->set_clientid(m_uSubSessionID);
    DeleteRangeRequest *pRangeRequest = pRequest->mutable_request_delete_range();
    pRangeRequest->set_key(strKey);
    uint32_t dwTimeoutMs = INFINITE;
    ResponseOp *pResponse = m_pClientPool->SendReq(pRequest, dwTimeoutMs);
    if (NULL != pResponse)
    {
        const DeleteRangeResponse& resRange = pResponse->response_delete_range();
        nDelete = pResponse->errorno();
        if (0 == nDelete)
        {
            std::string strValue;
            const google::protobuf::RepeatedPtrField<::raftserverpb::KeyValue>&rsFound = resRange.prev_kvs();
            if (!rsFound.empty())
                strValue = (*rsFound.begin()).value();
            else
                nDelete = 1;
        }
        delete pResponse;
    }
    else
        nDelete = 1;
    delete pRequest;
    return nDelete;
}

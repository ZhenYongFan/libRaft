#include "stdafx.h"
#include "RequestOp.h"

void CKeyValue::set_key(const std::string &strKey)
{
    m_strKey = strKey;
}

void CKeyValue::set_value(const std::string &strValue)
{
    m_strValue = strValue;
}

const std::string & CRangeRequest::key(void) const
{
    return m_strKey;
}

const std::string & CPutRequest::key(void) const
{
    return m_strKey;
}

const std::string & CPutRequest::value(void) const
{
    return m_strValue;
}

const std::string & CDeleteRangeRequest::key(void) const
{
    return m_strKey;
}

CRequestOp::CRequestOp()
{
}

CRequestOp::~CRequestOp()
{
}

void CRequestOp::set_clientid(uint32_t nClientID)
{
    m_nClientID = nClientID;
}

uint32_t CRequestOp::clientid(void)
{
    return m_nClientID;
}

void CRequestOp::set_subsessionid(uint32_t nSubSessionID)
{
    m_nSubSessionID = nSubSessionID;
}

uint32_t CRequestOp::subsessionid(void)
{
    return m_nSubSessionID;
}

CRequestOp::RequestCase CRequestOp::request_case(void)
{
    return m_typeRequest;
}

CRangeRequest * CRequestOp::mutable_request_range(void)
{
    return &m_rangeRequest;
}

const CRangeRequest & CRequestOp::request_range(void)
{
    return m_rangeRequest;
}

CPutRequest * CRequestOp::mutable_request_put(void)
{
    return &m_putRequest;
}

const CPutRequest & CRequestOp::request_put(void)
{
    return m_putRequest;
}

const CDeleteRangeRequest &CRequestOp::request_delete_range(void)
{
    return m_deleteRequest;
}

CRangeResponse::~CRangeResponse(void)
{
    for (auto pKeyValue:m_listKv)
    {
        delete pKeyValue;
    }
}

CKeyValue *CRangeResponse::add_kvs(void)
{
    CKeyValue *pKeyValue = new CKeyValue();
    m_listKv.push_back(pKeyValue);
    return pKeyValue;
}

CResponseOp::CResponseOp(void)
{

}

CResponseOp::~CResponseOp(void)
{

}

void CResponseOp::set_subsessionid(uint32_t nSubSessionID)
{
    m_nSubSessionID = nSubSessionID;
}

void CResponseOp::set_errorno(uint32_t nErrNo)
{
    m_nErrNo = nErrNo;
}

CRangeResponse *CResponseOp::mutable_response_range(void)
{
    return &m_rangeResponse;
}

const CRangeResponse &CResponseOp::response_range(void)
{
    return m_rangeResponse;
}

CPutResponse * CResponseOp::mutable_response_put(void)
{
    return &m_putResponse;
}

const CPutResponse & CResponseOp::response_put(void)
{
    return m_putResponse;
}

CDeleteRangeResponse* CResponseOp::mutable_response_delete_range(void)
{
    return &m_deleteResponse;
}

const CDeleteRangeResponse &CResponseOp::response_delete_range(void)
{
    return m_deleteResponse;
}

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
const std::string & CKeyValue::key(void) const
{
    return m_strKey;
}
const std::string & CKeyValue::value(void) const
{
    return m_strValue;
}

const std::string & CRangeRequest::key(void) const
{
    return m_strKey;
}

void CRangeRequest::set_key(const std::string &strKey)
{
    m_strKey = strKey;
}

const std::string & CPutRequest::key(void) const
{
    return m_strKey;
}

void CPutRequest::set_key(const std::string &strKey)
{
    m_strKey = strKey;
}

const std::string & CPutRequest::value(void) const
{
    return m_strValue;
}

void CPutRequest::set_value(const std::string &strValue)
{
    m_strValue = strValue;
}

const std::string & CDeleteRangeRequest::key(void) const
{
    return m_strKey;
}
void CDeleteRangeRequest::set_key(const std::string &strKey)
{
    m_strKey = strKey;
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

uint32_t CRequestOp::clientid(void) const
{
    return m_nClientID;
}

void CRequestOp::set_subsessionid(uint32_t nSubSessionID)
{
    m_nSubSessionID = nSubSessionID;
}

uint32_t CRequestOp::subsessionid(void) const
{
    return m_nSubSessionID;
}

bool CRequestOp::has_request_delete_range(void) const
{
    return (kRequestDeleteRange == m_typeRequest);
}

bool CRequestOp::has_request_put(void)  const
{
    return (kRequestPut == m_typeRequest);
}

bool CRequestOp::has_request_range(void)  const
{
    return (kRequestRange == m_typeRequest);
}

CRequestOp::ERequestCase CRequestOp::request_case(void) const
{
    return m_typeRequest;
}

CDeleteRangeRequest * CRequestOp::mutable_request_delete_range(void)
{
    m_typeRequest = kRequestDeleteRange;
    return &m_deleteRequest;
}

CRangeRequest * CRequestOp::mutable_request_range(void)
{
    m_typeRequest = kRequestRange;
    return &m_rangeRequest;
}

CPutRequest * CRequestOp::mutable_request_put(void)
{
    m_typeRequest = kRequestPut;
    return &m_putRequest;
}

const CRangeRequest & CRequestOp::request_range(void) const
{
    return m_rangeRequest;
}

const CPutRequest & CRequestOp::request_put(void) const
{
    return m_putRequest;
}

const CDeleteRangeRequest &CRequestOp::request_delete_range(void) const
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

const std::list<CKeyValue *> &CRangeResponse::GetKeyValues(void) const
{
    return m_listKv;
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

uint32_t CResponseOp::subsessionid(void) const
{
    return m_nSubSessionID;
}

void CResponseOp::set_errorno(uint32_t nErrNo)
{
    m_nErrNo = nErrNo;
}

uint32_t CResponseOp::errorno(void) const
{
    return m_nErrNo;
}

CResponseOp::EResponseCase CResponseOp::response_case(void) const
{
    return m_typeResponse;
}
bool CResponseOp::has_response_delete_range(void) const
{
    return (kResponseDeleteRange == m_typeResponse);
}
bool CResponseOp::has_response_put(void) const
{
    return (kResponsePut == m_typeResponse);
}
bool CResponseOp::has_response_range(void) const
{
    return (kResponseRange == m_typeResponse);
}

CRangeResponse *CResponseOp::mutable_response_range(void)
{
    m_typeResponse = kResponseRange;
    return &m_rangeResponse;
}

const CRangeResponse &CResponseOp::response_range(void) const
{
    return m_rangeResponse;
}

CPutResponse * CResponseOp::mutable_response_put(void)
{
    m_typeResponse = kResponsePut;
    return &m_putResponse;
}

const CPutResponse & CResponseOp::response_put(void) const
{
    return m_putResponse;
}

CDeleteRangeResponse* CResponseOp::mutable_response_delete_range(void)
{
    m_typeResponse = kResponseDeleteRange;
    return &m_deleteResponse;
}

const CDeleteRangeResponse &CResponseOp::response_delete_range(void) const
{
    return m_deleteResponse;
}

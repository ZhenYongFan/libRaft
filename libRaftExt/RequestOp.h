#pragma once

class CKeyValue
{
public:
    void set_key(const std::string &strKey);
    void set_value(const std::string &strValue);
protected:
    std::string m_strKey;
    std::string m_strValue;
};

class CRangeRequest
{
public:
    const std::string & key(void) const;
protected:
    std::string m_strKey;
};

class CPutRequest
{
public:
    const std::string & key(void) const;
    const std::string & value(void) const;
protected:
    std::string m_strKey;
    std::string m_strValue;
};

class CDeleteRangeRequest
{
public:
    const std::string & key(void) const;
protected:
    std::string m_strKey;
};

class CRequestOp
{
public:
    enum RequestCase
    {
        kRequestPut,
        kRequestDeleteRange,
        kRequestRange
    };
    CRequestOp();

    ~CRequestOp();

    void set_clientid(uint32_t nClientID);
    uint32_t clientid(void);

    void set_subsessionid(uint32_t nSubSessionID);
    uint32_t subsessionid(void);

    CRequestOp::RequestCase request_case(void);

    CRangeRequest * mutable_request_range(void);

    const CRangeRequest & request_range(void);

    CPutRequest * mutable_request_put(void);

    const CPutRequest & request_put(void);

    const CDeleteRangeRequest &request_delete_range(void);
protected:
    uint32_t m_nClientID;
    uint32_t m_nSubSessionID;
    RequestCase m_typeRequest;
    CRangeRequest m_rangeRequest;
    CPutRequest m_putRequest;
    CDeleteRangeRequest m_deleteRequest;
};

class CRangeResponse
{
public:
    ~CRangeResponse(void);
    CKeyValue *add_kvs(void);
protected:
    std::list<CKeyValue *> m_listKv;
};

class CPutResponse
{
public:
};

class CDeleteRangeResponse
{
public:
};

class CResponseOp
{
public:
    CResponseOp(void);

    ~CResponseOp(void);

    void set_subsessionid(uint32_t nSubSessionID);

    void set_errorno(uint32_t nErrNo);

    CRangeResponse *mutable_response_range();
    const CRangeResponse &response_range();

    CPutResponse * mutable_response_put(void);
    const CPutResponse & response_put(void);
    
    CDeleteRangeResponse* mutable_response_delete_range(void);
    const CDeleteRangeResponse &response_delete_range(void);
protected:
    uint32_t m_nErrNo;
    uint32_t m_nSubSessionID;
    CRangeResponse m_rangeResponse;
    CPutResponse m_putResponse;
    CDeleteRangeResponse m_deleteResponse;
};

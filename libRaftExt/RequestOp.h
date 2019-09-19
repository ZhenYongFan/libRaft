#pragma once

class CKeyValue
{
public:
    void set_key(const std::string &strKey);
    void set_value(const std::string &strValue);
    const std::string & key(void) const;
    const std::string & value(void) const;
protected:
    std::string m_strKey;
    std::string m_strValue;
};

class CRangeRequest
{
public:
    const std::string & key(void) const;
    void set_key(const std::string &strKey);
protected:
    std::string m_strKey;
};

class CPutRequest
{
public:
    const std::string & key(void) const;
    const std::string & value(void) const;
    void set_key(const std::string &strKey);
    void set_value(const std::string &strValue);
protected:
    std::string m_strKey;
    std::string m_strValue;
};

class CDeleteRangeRequest
{
public:
    const std::string & key(void) const;
    void set_key(const std::string &strKey);
protected:
    std::string m_strKey;
};

class CRequestOp
{
public:
    enum ERequestCase
    {
        kRequestPut,
        kRequestDeleteRange,
        kRequestRange
    };
    CRequestOp();

    ~CRequestOp();

    void set_clientid(uint32_t nClientID);
    uint32_t clientid(void) const;

    void set_subsessionid(uint32_t nSubSessionID);
    uint32_t subsessionid(void) const;

    bool has_request_delete_range(void) const;
    bool has_request_put(void) const;
    bool has_request_range(void) const;

    CRequestOp::ERequestCase request_case(void) const;

    CDeleteRangeRequest * mutable_request_delete_range(void);

    CRangeRequest * mutable_request_range(void);

    CPutRequest * mutable_request_put(void);

    const CRangeRequest & request_range(void) const;

    const CPutRequest & request_put(void) const;

    const CDeleteRangeRequest &request_delete_range(void) const;
protected:
    uint32_t m_nClientID;
    uint32_t m_nSubSessionID;
    ERequestCase m_typeRequest;
    CRangeRequest m_rangeRequest;
    CPutRequest m_putRequest;
    CDeleteRangeRequest m_deleteRequest;
};

class CRangeResponse
{
public:
    ~CRangeResponse(void);
    CKeyValue *add_kvs(void);
    const std::list<CKeyValue *> &GetKeyValues(void) const;
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
    enum EResponseCase
    {
        kResponsePut,
        kResponseDeleteRange,
        kResponseRange
    };

    CResponseOp(void);

    ~CResponseOp(void);

    void set_subsessionid(uint32_t nSubSessionID);

    uint32_t subsessionid( void) const;

    void set_errorno(uint32_t nErrNo);

    uint32_t errorno(void) const;

    CResponseOp::EResponseCase response_case(void) const;
    bool has_response_delete_range(void) const;
    bool has_response_put(void) const;
    bool has_response_range(void) const;

    CRangeResponse *mutable_response_range();
    const CRangeResponse &response_range() const;

    CPutResponse * mutable_response_put(void);
    const CPutResponse & response_put(void) const;
    
    CDeleteRangeResponse* mutable_response_delete_range(void);
    const CDeleteRangeResponse &response_delete_range(void) const;
protected:
    uint32_t m_nErrNo;
    uint32_t m_nSubSessionID;
    EResponseCase m_typeResponse;
    CRangeResponse m_rangeResponse;
    CPutResponse m_putResponse;
    CDeleteRangeResponse m_deleteResponse;
};

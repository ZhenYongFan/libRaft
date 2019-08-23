#pragma once
#include "RaftSerializer.h"
class CRequestOp;
class CResponseOp;

class CRaftProtoBufferSerializer : public CRaftSerializer
{
public:
    CRaftProtoBufferSerializer();
    virtual ~CRaftProtoBufferSerializer();

    void SerializeEntry(const CRaftEntry &entry, std::string &strValue);

    bool ParseEntry(CRaftEntry &entry, const std::string &strValue);

    void SerializeMessage(const CMessage &msg, std::string &strValue);

    bool ParseMessage(CMessage &msg, const std::string &strValue);

    void SerializeRequestOp(const CRequestOp &opRequest, std::string &strValue);

    bool ParseRequestOp(CRequestOp &opRequest, const std::string &strValue);

    void SerializeResponseOp(const CResponseOp &opResponse, std::string &strValue);

    bool ParseResponseOp(CResponseOp &opResponse, const std::string &strValue);
};


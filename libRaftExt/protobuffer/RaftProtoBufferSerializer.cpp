#include "stdafx.h"
#include "RaftProtoBufferSerializer.h"
#include "raft.pb.h"
#include "rpc.pb.h"

CRaftProtoBufferSerializer::CRaftProtoBufferSerializer()
{
}

CRaftProtoBufferSerializer::~CRaftProtoBufferSerializer()
{
}

void CRaftProtoBufferSerializer::SerializeEntry(const CRaftEntry &entry, std::string &strValue)
{
    raftpb::Entry entryPb;
    strValue = entryPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseEntry(CRaftEntry &entry, const std::string &strValue)
{
    raftpb::Entry entryPb;
    bool bParse = entryPb.ParseFromString(strValue);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeMessage(const CMessage &msg, std::string &strValue)
{
    raftpb::Message msgPb;
    strValue = msgPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseMessage(CMessage &msg, const std::string &strValue)
{
    raftpb::Message msgPb;
    bool bParse = msgPb.ParseFromString(strValue);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeRequestOp(const CRequestOp &opRequest, std::string &strValue)
{
    raftserverpb::RequestOp opRequestPb;
    strValue = opRequestPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseRequestOp(CRequestOp &opRequest, const std::string &strValue)
{
    raftserverpb::RequestOp opRequestPb;
    bool bParse = opRequestPb.ParseFromString(strValue);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeResponseOp(const CResponseOp &opResponse, std::string &strValue)
{
    raftserverpb::ResponseOp opResponsePb;
    strValue = opResponsePb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseResponseOp(CResponseOp &opResponse, const std::string &strValue)
{
    raftserverpb::ResponseOp opResponsePb;
    bool bParse = opResponsePb.ParseFromString(strValue);
    return bParse;
}

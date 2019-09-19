#include "stdafx.h"
#include "RaftProtoBufferSerializer.h"
#include "raft.pb.h"
#include "rpc.pb.h"
#include <RaftEntry.h>
#include <RequestOp.h>

void LocalToPb(const CRaftEntry &entry, raftpb::Entry &entryPb)
{
    entryPb.set_term(entry.m_u64Term);
    entryPb.set_index(entry.m_u64Index);
    entryPb.set_type(raftpb::EntryType(entry.m_eType));
    entryPb.set_data(entry.m_strData);
}

void LocalToPb(const CConfState &stateConf, raftpb::ConfState &stateConfPb)
{
    for (int nIndex = 0; nIndex < stateConf.m_nNum; nIndex++)
        stateConfPb.add_nodes(stateConf.m_pnNodes[nIndex]);   
}

void LocalToPb(const CSnapshotMetadata &metaDat, raftpb::SnapshotMetadata &metaDatPb)
{
    metaDatPb.set_index(metaDat.m_u64Index);
    metaDatPb.set_term(metaDat.m_u64Term);
    raftpb::ConfState* pConfStatePb = metaDatPb.mutable_conf_state();
    LocalToPb(metaDat.m_stateConf, *pConfStatePb);
}

void LocalToPb(const CSnapshot &snapshot, raftpb::Snapshot &snapshotPb)
{
    if (snapshot.m_u32DatLen > 0)
        snapshotPb.set_data(std::string((char *)snapshot.m_pData, snapshot.m_u32DatLen));
    raftpb::SnapshotMetadata * pMetaData = snapshotPb.mutable_metadata();
    LocalToPb(snapshot.m_metaDat, *pMetaData);
}

void LocalToPb(const CMessage &msg, raftpb::Message &msgPb)
{
    msgPb.set_index(msg.m_u64Index);
    msgPb.set_commit(msg.m_u64Committed);
    msgPb.set_context(msg.m_strContext);
    msgPb.set_from(msg.m_nFromID);
    msgPb.set_logterm(msg.m_u64LogTerm);
    msgPb.set_reject(msg.m_bReject);
    msgPb.set_rejecthint(msg.m_u64RejectHint);
    msgPb.set_term(msg.m_u64Term);
    msgPb.set_to(msg.m_nToID);
    msgPb.set_type(raftpb::MessageType(msg.m_typeMsg));
    raftpb::Snapshot *pSnapshot = msgPb.mutable_snapshot();
    if (NULL != pSnapshot)
        LocalToPb(msg.m_snapshotLog, *pSnapshot);
    for (int nIndex = 0 ; nIndex < msg.m_nEntryNum ; nIndex ++)
    {
        raftpb::Entry *pEntry = msgPb.add_entries();
        LocalToPb(msg.m_pEntry[nIndex], *pEntry);
    }
}

void LocalToPb(const CConfChange &conf, raftpb::ConfChange &confPb)
{
    confPb.set_id(conf.m_nID);
    confPb.set_nodeid(conf.m_nRaftID);
    confPb.set_type(raftpb::ConfChangeType(conf.m_typeChange));
    if (conf.m_u32Len > 0)
        confPb.set_context(std::string((char *)conf.m_pContext, conf.m_u32Len));
}

void PbToLocal(const raftpb::Entry &entryPb, CRaftEntry &entry)
{
    entry.set_term(entryPb.term());
    entry.set_index(entryPb.index());
    entry.set_type(CRaftEntry::EType(entryPb.type()));
    entry.set_data(entryPb.data());
}

void PbToLocal(const raftpb::ConfState &stateConfPb,CConfState &stateConf)
{
    for (int nIndex = 0; nIndex < stateConfPb.nodes_size(); nIndex++)
        stateConf.add_nodes(stateConfPb.nodes(nIndex));
}

void PbToLocal(const raftpb::SnapshotMetadata &metaDatPb, CSnapshotMetadata &metaDat)
{
    metaDat.set_index(metaDatPb.index());
    metaDat.set_term(metaDatPb.term());
    if(metaDatPb.has_conf_state())
        PbToLocal(metaDatPb.conf_state(), metaDat.m_stateConf);
}

void PbToLocal(const raftpb::Snapshot &snapshotPb, CSnapshot &snapshot)
{
    snapshot.set_data(snapshotPb.data());
    if (snapshotPb.has_metadata())
        PbToLocal(snapshotPb.metadata(), snapshot.m_metaDat);
}

void PbToLocal(const raftpb::Message &msgPb, CMessage &msg)
{
    msg.set_index(msgPb.index());
    msg.set_commit(msgPb.commit());
    msg.set_context(msgPb.context());
    msg.set_from(msgPb.from());
    msg.set_logterm(msgPb.logterm());
    msg.set_reject(msgPb.reject());
    msg.set_rejecthint(msgPb.rejecthint());
    msg.set_term(msgPb.term());
    msg.set_to(msgPb.to());

    msg.set_type(CMessage::EMessageType(msgPb.type()));
    if (msgPb.has_snapshot())
        PbToLocal(msgPb.snapshot(), msg.m_snapshotLog);
    if (msgPb.entries_size() > 0)
    {
        msg.m_nEntryNum = msgPb.entries_size();
        msg.m_pEntry = new CRaftEntry[msg.m_nEntryNum];
        for (int nIndex = 0; nIndex < msgPb.entries_size(); nIndex++)
            PbToLocal(msgPb.entries(nIndex), msg.m_pEntry[nIndex]);
    }
}

void PbToLocal(const raftpb::ConfChange &confPb, CConfChange &conf)
{
    conf.set_id(confPb.id());
    conf.set_nodeid(confPb.nodeid());
    conf.set_type(CConfChange::EConfChangeType(confPb.type()));
    conf.set_context(confPb.context());
}

void RequestOpLocalToPb(const CRequestOp &opRequest, raftserverpb::RequestOp &opRequestPb)
{
    opRequestPb.set_clientid(opRequest.clientid());
    opRequestPb.set_subsessionid(opRequest.subsessionid());
    if (opRequest.has_request_delete_range())
    {
        raftserverpb::DeleteRangeRequest* pRequest = opRequestPb.mutable_request_delete_range();
        const CDeleteRangeRequest &request = opRequest.request_delete_range();
        pRequest->set_key(request.key());
    }
    else if (opRequest.has_request_put())
    {
        raftserverpb::PutRequest* pRequest = opRequestPb.mutable_request_put();
        const CPutRequest &request = opRequest.request_put();
        pRequest->set_key(request.key());
        pRequest->set_value(request.value());
    }
    else if (opRequest.has_request_range())
    {
        raftserverpb::RangeRequest* pRequest = opRequestPb.mutable_request_range();
        const CRangeRequest &request = opRequest.request_range();
        pRequest->set_key(request.key());
    }
}

void RequestOpPbToLocal(const raftserverpb::RequestOp &opRequestPb, CRequestOp &opRequest)
{
    opRequest.set_clientid(opRequestPb.clientid());
    opRequest.set_subsessionid(opRequestPb.subsessionid());
    if (opRequestPb.has_request_delete_range())
    {
        CDeleteRangeRequest* pRequest = opRequest.mutable_request_delete_range();
        const raftserverpb::DeleteRangeRequest &request = opRequestPb.request_delete_range();
        pRequest->set_key(request.key());
    }
    else if (opRequestPb.has_request_put())
    {
        CPutRequest* pRequest = opRequest.mutable_request_put();
        const raftserverpb::PutRequest &request = opRequestPb.request_put();
        pRequest->set_key(request.key());
        pRequest->set_value(request.value());
    }
    else if (opRequestPb.has_request_range())
    {
        CRangeRequest* pRequest = opRequest.mutable_request_range();
        const raftserverpb::RangeRequest &request = opRequestPb.request_range();
        pRequest->set_key(request.key());
    }
}

void ResponseOpLocalToPb(const CResponseOp &opResponse, raftserverpb::ResponseOp &opResponsePb)
{
    opResponsePb.set_errorno(opResponse.errorno());
    opResponsePb.set_subsessionid(opResponse.subsessionid());
    if (opResponse.has_response_delete_range())
    {
        raftserverpb::DeleteRangeResponse* pResponse = opResponsePb.mutable_response_delete_range();
        const CDeleteRangeResponse &response = opResponse.response_delete_range();
    }
    else if (opResponse.has_response_put())
    {
        raftserverpb::PutResponse* pResponse = opResponsePb.mutable_response_put();
        const CPutResponse &response = opResponse.response_put();
    }
    else if (opResponse.has_response_range())
    {
        raftserverpb::RangeResponse* pResponse = opResponsePb.mutable_response_range();
        const CRangeResponse &response = opResponse.response_range();
        const std::list<CKeyValue *> &listKeyvalues = response.GetKeyValues();
        for (auto pKeyValue : listKeyvalues)
        {
            raftserverpb::KeyValue* pPbKeyValue = pResponse->add_kvs();
            pPbKeyValue->set_key(pKeyValue->key());
            pPbKeyValue->set_value(pKeyValue->value());
        }
    }
}

void ResponseOpPbToLocal(const raftserverpb::ResponseOp &opResponsePb, CResponseOp &opResponse)
{
    opResponse.set_errorno(opResponsePb.errorno());
    opResponse.set_subsessionid(opResponsePb.subsessionid());
    if (opResponsePb.has_response_delete_range())
    {
        CDeleteRangeResponse* pResponse = opResponse.mutable_response_delete_range();
        const raftserverpb::DeleteRangeResponse &response = opResponsePb.response_delete_range();
    }
    else if (opResponsePb.has_response_put())
    {
        CPutResponse* pResponse = opResponse.mutable_response_put();
        const raftserverpb::PutResponse &response = opResponsePb.response_put();
    }
    else if (opResponsePb.has_response_range())
    {
        CRangeResponse* pResponse = opResponse.mutable_response_range();
        const raftserverpb::RangeResponse &response = opResponsePb.response_range();
        for (int nIndex = 0 ; nIndex < response.kvs_size(); nIndex ++)
        {
            const raftserverpb::KeyValue &keyValue = response.kvs(nIndex);
            CKeyValue* pKeyValue = pResponse->add_kvs();
            pKeyValue->set_key(keyValue.key());
            pKeyValue->set_value(keyValue.value());
        }
    }
}

CRaftProtoBufferSerializer::CRaftProtoBufferSerializer()
{
}

CRaftProtoBufferSerializer::~CRaftProtoBufferSerializer()
{
}

size_t CRaftProtoBufferSerializer::ByteSize(const CRaftEntry &entry) const
{
    raftpb::Entry entryPb;
    LocalToPb(entry, entryPb);
    return entryPb.ByteSize();
}

void CRaftProtoBufferSerializer::SerializeEntry(const CRaftEntry &entry, std::string &strValue)
{
    raftpb::Entry entryPb;
    LocalToPb(entry, entryPb);
    strValue = entryPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseEntry(CRaftEntry &entry, const std::string &strValue)
{
    raftpb::Entry entryPb;
    bool bParse = entryPb.ParseFromString(strValue);
    if (bParse)
        PbToLocal(entryPb, entry);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeMessage(const CMessage &msg, std::string &strValue)
{
    raftpb::Message msgPb;
    LocalToPb(msg, msgPb);
    strValue = msgPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseMessage(CMessage &msg, const std::string &strValue)
{
    raftpb::Message msgPb;
    bool bParse = msgPb.ParseFromString(strValue);
    if (bParse)
        PbToLocal(msgPb, msg);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeConfChangee(const CConfChange &conf, std::string &strValue)
{
    raftpb::ConfChange confPb;
    LocalToPb(conf, confPb);
    strValue = confPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseConfChange(CConfChange &conf, const std::string &strValue)
{
    raftpb::ConfChange confPb;
    bool bParse = confPb.ParseFromString(strValue);
    if (bParse)
        PbToLocal(confPb, conf);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeRequestOp(const CRequestOp &opRequest, std::string &strValue)
{
    raftserverpb::RequestOp opRequestPb;
    RequestOpLocalToPb(opRequest, opRequestPb);
    strValue = opRequestPb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseRequestOp(CRequestOp &opRequest, const std::string &strValue)
{
    raftserverpb::RequestOp opRequestPb;
    bool bParse = opRequestPb.ParseFromString(strValue);
    if (bParse)
        RequestOpPbToLocal(opRequestPb, opRequest);
    return bParse;
}

void CRaftProtoBufferSerializer::SerializeResponseOp(const CResponseOp &opResponse, std::string &strValue)
{
    raftserverpb::ResponseOp opResponsePb;
    ResponseOpLocalToPb(opResponse, opResponsePb);
    strValue = opResponsePb.SerializeAsString();
}

bool CRaftProtoBufferSerializer::ParseResponseOp(CResponseOp &opResponse, const std::string &strValue)
{
    raftserverpb::ResponseOp opResponsePb;
    bool bParse = opResponsePb.ParseFromString(strValue);
    if (bParse)
        ResponseOpPbToLocal(opResponsePb, opResponse);
    return bParse;
}

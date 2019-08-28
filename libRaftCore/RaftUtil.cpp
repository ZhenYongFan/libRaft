#include "stdafx.h"
#include "RaftUtil.h"
#include "RaftMemLog.h"
#include "RaftSerializer.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

bool isDeepEqualSnapshot(const CSnapshot *s1, const CSnapshot *s2) {
  if (s1 == NULL || s2 == NULL) {
    return false;
  }

  if (s1->metadata().index() != s2->metadata().index()) {
    return false;
  }
  if (s1->metadata().term() != s2->metadata().term()) {
    return false;
  }
  if (s1->m_u32DatLen != s2->m_u32DatLen) {
    return false;
  }
  if (0 != memcmp(s1->m_pData,s2->m_pData,s1->m_u32DatLen)){
      return false;
  }

  return true;
}

bool isDeepEqualEntry(const CRaftEntry& ent1, const CRaftEntry& ent2) {
  if (ent1.type() != ent2.type()) {
    return false;
  }
  if (ent1.term() != ent2.term()) {
    return false;
  }
  if (ent1.index() != ent2.index()) {
    return false;
  }
  if (ent1.data() != ent2.data()) {
    return false;
  }
  return true;
}

bool isDeepEqualEntries(const EntryVec& ents1, const EntryVec& ents2) {
  if (ents1.size() != ents2.size()) {
    return false;
  }
  int i;
  for (i = 0; i < ents1.size(); ++i) {
    if (!isDeepEqualEntry(ents1[i], ents2[i])) {
      return false;
    }
  }
  return true;
}

bool isDeepEqualNodes(const vector<uint32_t>& ns1, const vector<uint32_t>& ns2) {
  if (ns1.size() != ns2.size()) {
    return false;
  }

  int i;
  for (i = 0; i < ns1.size(); ++i) {
    if (ns1[i] != ns2[i]) {
      return false;
    }
  }
  return true;
}

void limitSize(uint64_t maxSize, EntryVec &entries, CRaftSerializer &serializer) {
  if (entries.empty()) {
    return;
  }

  int limit;
  int num = static_cast<int>(entries.size());
  uint64_t size = serializer.ByteSize(entries[0]);
  for (limit = 1; limit < num; ++limit) {
    size += serializer.ByteSize(entries[limit]);
    if (size > maxSize) {
      break;
    }
  }

  entries.erase(entries.begin() + limit, entries.end());
}

bool IsLocalMessage(const CMessage::EMessageType typeMsg)
{
    return (typeMsg == CMessage::MsgHup
        || typeMsg == CMessage::MsgBeat
        || typeMsg == CMessage::MsgUnreachable
        || typeMsg == CMessage::MsgSnapStatus
        || typeMsg == CMessage::MsgCheckQuorum);
}

bool IsResponseMessage(const CMessage::EMessageType typeMsg)
{
    return (typeMsg == CMessage::MsgAppResp
        || typeMsg == CMessage::MsgVoteResp
        || typeMsg == CMessage::MsgHeartbeatResp
        || typeMsg == CMessage::MsgUnreachable
        || typeMsg == CMessage::MsgPreVoteResp);
}

bool isHardStateEqual(const CHardState& h1, const CHardState& h2) {
  return h1.term() == h2.term() &&
         h1.vote() == h2.vote() &&
         h1.commit() == h2.commit();
}

bool isSoftStateEqual(const CSoftState& s1, const CSoftState& s2) {
  if (s1.m_nLeaderID != s2.m_nLeaderID) {
    return false;
  }

  return s1.m_stateRaft == s2.m_stateRaft;
}

bool isEmptySnapshot(const CSnapshot* pSnapshot)
{
    if (pSnapshot == NULL)
    {
        return true;
    }
    return pSnapshot->metadata().index() == 0;
}

bool isDeepEqualReadStates(const vector<CReadState*>& rs1, const vector<CReadState*>& rs2) {
  if (rs1.size() != rs2.size()) {
    return false;
  }
  int i;
  for (i = 0; i < rs1.size(); ++i) {
    CReadState* r1 = rs1[i];
    CReadState* r2 = rs2[i];
    if (r1->m_u64Index != r2->m_u64Index) {
      return false;
    }
    if (r1->m_strRequestCtx != r2->m_strRequestCtx) {
      return false;
    }
  }

  return true;
}

bool isDeepEqualMessage(const CMessage& msg1, const CMessage& msg2) {
  if (msg1.from() != msg2.from()) {
    return false;
  }
  if (msg1.to() != msg2.to()) {
    return false;
  }
  if (msg1.type() != msg2.type()) {
    return false;
  }

  if (msg1.entries_size() != msg2.entries_size()) {
    return false;
  }
  
  int i;
  for (i = 0; i < msg1.entries_size(); ++i) {
    if (!isDeepEqualEntry(*msg1.entries(i), *msg2.entries(i))) {
      return false;
    }
  }
  return true;
}

int GetNumOfPendingConf(const EntryVec& entries)
{
    int nNum = 0;
    for (const CRaftEntry &entry : entries)
    {
        if (CRaftEntry::eConfChange == entry.type())
            nNum ++ ;
    }
    return nNum;
}

CMessage::EMessageType VoteRespMsgType(CMessage::EMessageType typeMsg)
{
    if (typeMsg == CMessage::MsgVote)
        return CMessage::MsgVoteResp;
    else
        return CMessage::MsgPreVoteResp;
}

void copyEntries(const CMessage& msg, EntryVec &entries)
{
    for (int i = 0; i < msg.entries_size(); ++i)
        entries.push_back(*msg.entries(i));
}

const char *pstrErrorString[] = {
    "OK",
    "Compacted",
    "ErrSnapOutOfDate",
    "ErrUnavailable",
    "ErrSnapshotTemporarilyUnavailable",
    "ErrSeriaFail",
    ""
};

const char* CRaftErrNo::GetErrorString(int nErrNo)
{
    const char* pstrErrMsg ;
    if (nErrNo >= 0 && nErrNo <= int(CRaftErrNo::eErrSeriaFail))
        pstrErrMsg = pstrErrorString[nErrNo];
    else
        pstrErrMsg = pstrErrorString[int(CRaftErrNo::eErrSeriaFail) + 1];
    return pstrErrMsg;
}

const char *pstrTypeStrings[] = {
    "MsgHup",
    "MsgBeat",
    "MsgProp",
    "MsgApp",
    "MsgAppResp",
    "MsgVote",
    "MsgVoteResp",
    "MsgSnap",
    "MsgHeartbeat",
    "MsgHeartbeatResp",
    "MsgUnreachable",
    "MsgSnapStatus",
    "MsgCheckQuorum",
    "MsgTransferLeader",
    "MsgTimeoutNow",
    "MsgReadIndex",
    "MsgReadIndexResp",
    "MsgPreVote",
    "MsgPreVoteResp",
    "unknown msg"
};

const char* CRaftUtil::MsgType2String(int typeMsg)
{
    const char* pstrTypeString;
    if (typeMsg >= 0 && typeMsg < CMessage::MsgNoUsed)
        pstrTypeString = pstrTypeStrings[typeMsg];
    else
        pstrTypeString = pstrTypeStrings[CMessage::MsgNoUsed];
    return pstrTypeString;
}

string CRaftUtil::EntryString(const CRaftEntry& entry)
{
    char tmp[100];
    snprintf(tmp, sizeof(tmp), "term:%llu, index:%llu, type:%d",
        entry.term(), entry.index(), entry.type());
    string str = tmp;
    str += ", data:" + entry.data() + "\n";
    return str;
}

string CRaftUtil::JoinStrings(const vector<string>& strPeers, const string &strSep)
{
    string strJoins ;
    for (auto strPeer :strPeers)
    {
        if(strJoins.empty())
            strJoins += strSep;
        strJoins += strPeer;
    }
    return strJoins;
}

CMessage* CRaftUtil::CloneMessage(const CMessage& msg)
{
    return new CMessage(msg);
}

// newLog returns log using the given storage. It recovers the log to the state
// that it just commits and applies the latest snapshot.
CRaftMemLog* newLog(CRaftStorage *pStorage, CLogger *pLogger)
{
    CRaftMemLog *pRaftLog = new CRaftMemLog(pStorage, pLogger);

    uint64_t u64FirstIndex, u64LastIndex;
    int nErrorNo = pStorage->FirstIndex(u64FirstIndex);
    if (!CRaftErrNo::Success(nErrorNo))
        pLogger->Fatalf(__FILE__, __LINE__, "get first index err:%s", CRaftErrNo::GetErrorString(nErrorNo));

    nErrorNo = pStorage->LastIndex(u64LastIndex);
    if (!CRaftErrNo::Success(nErrorNo))
        pLogger->Fatalf(__FILE__, __LINE__, "get last index err:%s", CRaftErrNo::GetErrorString(nErrorNo));

    pRaftLog->m_unstablePart.m_u64Offset = u64LastIndex + 1;
    pRaftLog->m_unstablePart.m_pLogger = pLogger;

    // Initialize our committed and applied pointers to the time of the last compaction.
    pRaftLog->m_pStorage->m_u64Committed = u64FirstIndex - 1;
    pRaftLog->m_pStorage->m_u64Applied = u64FirstIndex - 1;

    return pRaftLog;
}

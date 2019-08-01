#include "stdafx.h"
#include <string>
#include "RaftUtil.h"
#include "RaftMemLog.h"

using namespace std;

const char* GetErrorString(int err) {
  return "";
}

bool isDeepEqualSnapshot(const Snapshot *s1, const Snapshot *s2) {
  if (s1 == NULL || s2 == NULL) {
    return false;
  }

  if (s1->metadata().index() != s2->metadata().index()) {
    return false;
  }
  if (s1->metadata().term() != s2->metadata().term()) {
    return false;
  }
  if (s1->data() != s2->data()) {
    return false;
  }

  return true;
}

bool isDeepEqualEntry(const Entry& ent1, const Entry& ent2) {
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

bool isDeepEqualNodes(const vector<uint64_t>& ns1, const vector<uint64_t>& ns2) {
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

void limitSize(uint64_t maxSize, EntryVec &entries) {
  if (entries.empty()) {
    return;
  }

  int limit;
  int num = static_cast<int>(entries.size());
  uint64_t size = entries[0].ByteSize();
  for (limit = 1; limit < num; ++limit) {
    size += entries[limit].ByteSize();
    if (size > maxSize) {
      break;
    }
  }

  entries.erase(entries.begin() + limit, entries.end());
}

bool IsLocalMessage(const MessageType typeMsg)
{
    return (typeMsg == MsgHup
        || typeMsg == MsgBeat
        || typeMsg == MsgUnreachable
        || typeMsg == MsgSnapStatus
        || typeMsg == MsgCheckQuorum);
}

bool IsResponseMessage(const MessageType typeMsg)
{
    return (typeMsg == MsgAppResp
        || typeMsg == MsgVoteResp
        || typeMsg == MsgHeartbeatResp
        || typeMsg == MsgUnreachable
        || typeMsg == MsgPreVoteResp);
}

bool isHardStateEqual(const HardState& h1, const HardState& h2) {
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

bool isEmptySnapshot(const Snapshot* snapshot) {
  if (snapshot == NULL) {
    return true;
  }
  return snapshot->metadata().index() == 0;
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

bool isDeepEqualMessage(const Message& msg1, const Message& msg2) {
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
    if (!isDeepEqualEntry(msg1.entries(i), msg2.entries(i))) {
      return false;
    }
  }
  return true;
}

int GetNumOfPendingConf(const EntryVec& entries)
{
    int nNum = 0;
    for (const Entry &entry : entries)
    {
        if (EntryConfChange == entry.type())
            nNum ++ ;
    }
    return nNum;
}

MessageType VoteRespMsgType(MessageType typeMsg)
{
    if (typeMsg == MsgVote)
        return MsgVoteResp;
    else
        return MsgPreVoteResp;
}

string joinStrings(const vector<string>& strs, const string &sep) {
  string ret = "";
  size_t i;
  for (i = 0; i < strs.size(); ++i) {
    if (ret.length() > 0) {
      ret += sep;
    }
    ret += strs[i];
  }

  return ret;
}

const char *pstrTypeString[] = {
    "MsgHup",
    "MsgBeat",
    "MsgProp",      //Propose 提议，建议
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
    "MsgPreVoteResp"
};

string entryString(const Entry& entry)
{
    char tmp[100];
    snprintf(tmp, sizeof(tmp), "term:%llu, index:%llu, type:%d",
        entry.term(), entry.index(), entry.type());
    string str = tmp;
    str += ", data:" + entry.data() + "\n";
    return str;
}

void copyEntries(const Message& msg, EntryVec &entries)
{
    for (int i = 0; i < msg.entries_size(); ++i)
        entries.push_back(msg.entries(i));
}

const char* CRaftUtil::MsgType2String(int typeMsg)
{
    if (typeMsg >= 0 && typeMsg <= 18)
        return pstrTypeString[typeMsg];
    else
        return "unknown msg";
}

// newLog returns log using the given storage. It recovers the log to the state
// that it just commits and applies the latest snapshot.
CRaftMemLog* newLog(CRaftStorage *pStorage, CLogger *pLogger)
{
    CRaftMemLog *log = new CRaftMemLog(pStorage, pLogger);

    uint64_t firstIndex, lastIndex;
    int err = pStorage->FirstIndex(firstIndex);
    if (!SUCCESS(err))
        pLogger->Fatalf(__FILE__, __LINE__, "get first index err:%s", GetErrorString(err));

    err = pStorage->LastIndex(lastIndex);
    if (!SUCCESS(err))
        pLogger->Fatalf(__FILE__, __LINE__, "get last index err:%s", GetErrorString(err));

    log->m_unstablePart.m_u64Offset = lastIndex + 1;
    log->m_unstablePart.m_pLogger = pLogger;

    // Initialize our committed and applied pointers to the time of the last compaction.
    log->m_pStorage->m_u64Committed = firstIndex - 1;
    log->m_pStorage->m_u64Applied = firstIndex - 1;

    return log;
}

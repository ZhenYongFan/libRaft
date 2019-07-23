#pragma once

#include "libRaftCore.h"
#include "RaftDef.h"

void limitSize(uint64_t maxSize, EntryVec &entries);

bool isDeepEqualNodes(const vector<uint64_t>& ns1, const vector<uint64_t>& ns2);
bool isDeepEqualSnapshot(const Snapshot *s1, const Snapshot *s2);
bool isDeepEqualEntries(const EntryVec& ents1, const EntryVec& ents2);
bool isDeepEqualReadStates(const vector<CReadState*>& rs1, const vector<CReadState*>& rs2);
bool isDeepEqualMessage(const Message& msg1, const Message& msg2);
bool isHardStateEqual(const HardState& h1, const HardState& h2);
bool isSoftStateEqual(const CSoftState& s1, const CSoftState& s2);
bool isEmptySnapshot(const Snapshot* snapshot);
int GetNumOfPendingConf(const EntryVec& entries);
MessageType VoteRespMsgType(MessageType typeMsg);

bool IsLocalMessage(const MessageType typeMsg);
bool IsResponseMessage(const MessageType typeMsg);

// string util
string joinStrings(const vector<string>& strs, const string &sep);

const char* GetErrorString(int nErrNo);
string entryString(const Entry& entry);

void copyEntries(const Message& msg, EntryVec &entries);

class LIBRAFTCORE_API CRaftUtil
{
public:
    static const char* MsgType2String(int typeMsg);
private:
    ///\brief 缺省构造
    CRaftUtil(void)
    {
    }

    ///\brief 禁用的复制型构造函数
    CRaftUtil(const CRaftUtil &)
    {
    }

    ///\brief 禁用的赋值运算符
    const CRaftUtil &operator = (const CRaftUtil &)
    {
        return *this;
    }
};

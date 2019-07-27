#pragma once

#include "libRaftCore.h"
#include "RaftDef.h"

BEGIN_C_DECLS
LIBRAFTCORE_API void limitSize(uint64_t maxSize, EntryVec &entries);

LIBRAFTCORE_API bool isDeepEqualNodes(const vector<uint64_t>& ns1, const vector<uint64_t>& ns2);
LIBRAFTCORE_API bool isDeepEqualSnapshot(const Snapshot *s1, const Snapshot *s2);
LIBRAFTCORE_API bool isDeepEqualEntries(const EntryVec& ents1, const EntryVec& ents2);
LIBRAFTCORE_API bool isDeepEqualReadStates(const vector<CReadState*>& rs1, const vector<CReadState*>& rs2);
LIBRAFTCORE_API bool isDeepEqualMessage(const Message& msg1, const Message& msg2);
LIBRAFTCORE_API bool isHardStateEqual(const HardState& h1, const HardState& h2);
LIBRAFTCORE_API bool isSoftStateEqual(const CSoftState& s1, const CSoftState& s2);
LIBRAFTCORE_API bool isEmptySnapshot(const Snapshot* snapshot);
LIBRAFTCORE_API int GetNumOfPendingConf(const EntryVec& entries);
LIBRAFTCORE_API MessageType VoteRespMsgType(MessageType typeMsg);

LIBRAFTCORE_API bool IsLocalMessage(const MessageType typeMsg);
LIBRAFTCORE_API bool IsResponseMessage(const MessageType typeMsg);

// string util
LIBRAFTCORE_API string  joinStrings(const vector<string>& strs, const string &sep);

LIBRAFTCORE_API const char* GetErrorString(int nErrNo);
LIBRAFTCORE_API string  entryString(const Entry& entry);

LIBRAFTCORE_API void  copyEntries(const Message& msg, EntryVec &entries);
END_C_DECLS

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

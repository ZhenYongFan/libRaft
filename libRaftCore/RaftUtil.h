#pragma once

#include "libRaftCore.h"
#include "RaftDef.h"

class CRaftMemLog;
class CRaftStorage;
class CLogger;
class CRaftSerializer;

BEGIN_C_DECLS
LIBRAFTCORE_API void limitSize(uint64_t maxSize, EntryVec &entries,CRaftSerializer &serializer);

LIBRAFTCORE_API bool isDeepEqualNodes(const vector<uint32_t>& ns1, const vector<uint32_t>& ns2);
LIBRAFTCORE_API bool isDeepEqualSnapshot(const CSnapshot *s1, const CSnapshot *s2);
LIBRAFTCORE_API bool isDeepEqualEntries(const EntryVec& ents1, const EntryVec& ents2);
LIBRAFTCORE_API bool isDeepEqualReadStates(const vector<CReadState*>& rs1, const vector<CReadState*>& rs2);
LIBRAFTCORE_API bool isDeepEqualMessage(const CMessage& msg1, const CMessage& msg2);
LIBRAFTCORE_API bool isHardStateEqual(const CHardState& h1, const CHardState& h2);
LIBRAFTCORE_API bool isSoftStateEqual(const CSoftState& s1, const CSoftState& s2);
LIBRAFTCORE_API bool isEmptySnapshot(const CSnapshot* snapshot);
LIBRAFTCORE_API int GetNumOfPendingConf(const EntryVec& entries);
LIBRAFTCORE_API CMessage::EMessageType VoteRespMsgType(CMessage::EMessageType typeMsg);

LIBRAFTCORE_API bool IsLocalMessage(const CMessage::EMessageType typeMsg);
LIBRAFTCORE_API bool IsResponseMessage(const CMessage::EMessageType typeMsg);

// string util

LIBRAFTCORE_API void  copyEntries(const CMessage& msg, EntryVec &entries);

LIBRAFTCORE_API CRaftMemLog* newLog(CRaftStorage *pStorage, CLogger *pLogger);

END_C_DECLS

///\brief 一组用于Raft算法的函数组成的工具类
class LIBRAFTCORE_API CRaftUtil
{
public:
    
    ///\brief 得到对应Message类型的Messge名称
    ///\param typeMsg Message类型
    ///\return Messge名称
    static const char* MsgType2String(int typeMsg);

    ///\brief 日志对象转化为可显示字符串，用于输出日志
    ///\param entry 日志对象
    ///\return 可显示字符串
    static std::string  EntryString(const CRaftEntry& entry);

    ///\brief 输出节点列表，用于输出日志
    ///\param strPeers 节点列表
    ///\param strSep 分隔符
    ///\return 用于输出日志的节点列表字符串
    static std::string  JoinStrings(const vector<string>& strPeers, const string &sep);

    ///\brief Clone一个Message
    ///\param msg Clone的对象
    ///\return Clone得到的Message对象指针，需要调用delete释放
    ///\attention 将来也许会用智能指针替代
    static CMessage* CloneMessage(const CMessage& msg);

private:
    ///\brief 禁用的缺省构造
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

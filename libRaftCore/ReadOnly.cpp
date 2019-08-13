#include "stdafx.h"
#include "raft.pb.h"
using namespace raftpb;

#include "RaftConfig.h"
#include "RaftLogger.h"
#include "ReadOnly.h"

CReadOnly::CReadOnly(ReadOnlyOption optMode, CLogger *pLogger)
    : m_optMode(optMode),
    m_pLogger(pLogger)
{
}

CReadOnly::~CReadOnly(void)
{
    for (auto iter : m_mapPendingReadIndex)
        delete iter.second;
    m_mapPendingReadIndex.clear();
}

void CReadOnly::AddRequest(uint64_t u64Committed, Message *pMsgRead)
{
    //读请求信息中加入Client、Session信息组成唯一标识
    string strContext = pMsgRead->entries(0).data();
    if (m_mapPendingReadIndex.find(strContext) == m_mapPendingReadIndex.end())
    {
        m_mapPendingReadIndex[strContext] = new CReadIndexStatus(u64Committed, pMsgRead);
        m_listRead.push_back(strContext);
    }
}

// RecvAck通知readonly结构，即raft状态机接受了对只读请求上下文附加的心跳的确认。
// 1.消息的Context即消息ID，根据消息id获取对应的readIndexStatus
// 2.如果获取不到则返回0
// 3.记录了该Follower节点返回的MsgHeartbeatResp响应的信息
// 4.返回Follower响应的数量
int CReadOnly::RecvAck(const Message& msgRead)
{
    int nAck = 0;
    auto iter = m_mapPendingReadIndex.find(msgRead.context());
    if (iter != m_mapPendingReadIndex.end())
    {
        CReadIndexStatus* rs = iter->second;
        rs->m_mapAck[msgRead.from()] = true;
        nAck = int(rs->m_mapAck.size() + 1);
    }
    return nAck;
}

//1.遍历readIndex队列，如果能找到该消息的Context，则返回该消息及之前的所有记录rss，
//并删除readIndex队列和pendingReadIndex中对应的记录
//2.如果没有Context对应的消息ID，则返回空结果集
void CReadOnly::Advance(const Message& msgRead, vector<CReadIndexStatus*> *rss)
{
    bool bFound = false;
    string strContext = msgRead.context();

    for (auto iter = m_listRead.begin(); iter != m_listRead.end(); ++ iter)
    {
        auto iterIndex = m_mapPendingReadIndex.find(*iter);
        if (iterIndex == m_mapPendingReadIndex.end())
            m_pLogger->Fatalf(__FILE__, __LINE__, "cannot find corresponding read state from pending map");
        rss->push_back(iterIndex->second);
        
        if (strContext == *iter)
        {
            bFound = true;
            m_mapPendingReadIndex.erase(m_mapPendingReadIndex.begin(),++iterIndex);
            m_listRead.erase(m_listRead.begin(), ++iter );
            break;
        }
    }
    if (!bFound)
        rss->clear();
}

// lastPendingRequestCtx returns the context of the last pending read only
// request in readonly struct.
//返回记录中最后一个消息ID
string CReadOnly::LastPendingRequestCtx(void)
{
    if (m_listRead.empty())
        return "";
    else
        return m_listRead.back();
}

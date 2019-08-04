#include "stdafx.h"
#include "RaftQueue.h"

CRaftQueue::CRaftQueue(void)
{
}

CRaftQueue::~CRaftQueue(void)
{
}

bool CRaftQueue::Init(int nMaxSpace, std::string &strErrMsg)
{
    m_semSpace.Create(nMaxSpace);
    m_semObj.Create(0);
    return true;
}

bool CRaftQueue::Push(void *pObj,uint32_t uTimeout)
{
    bool bPush = false;
    uint32_t nWait = m_semSpace.Wait(uTimeout);
    if (WAIT_OBJECT_0 == nWait)
    {
        std::lock_guard<std::mutex> queueGuard(m_mutxQueue);
        m_listMsg.push_back(pObj);
        m_semObj.Post(1);
        bPush = true;
    }
    return bPush;
}

void *CRaftQueue::Pop(uint32_t uTimeout)
{
    void *pObj = NULL;
    if (WAIT_OBJECT_0 == m_semObj.Wait(uTimeout))
    {
        std::lock_guard<std::mutex> queueGuard(m_mutxQueue);
        pObj = m_listMsg.front();
        m_listMsg.pop_front();
        m_semSpace.Post(1);
    }
    return pObj;
}

#pragma once
#include "libRaftCore.h"

class CRaftClientPool;

///\brief Raft客户端
class LIBRAFTCORE_API CRaftClient
{
public:
    CRaftClient(void);

    virtual ~CRaftClient(void);

    virtual bool Init(CRaftClientPool *pClientPool);

    virtual void Uninit(void);

    void SetSubSessionID(uint32_t nClientSeq);

    uint32_t GetSubSessionID(void);

    int Put(const std::string &strKey, const std::string &strValue);

    int Get(const std::string &strKey, std::string &strValue);

    int Delete(const std::string &strKey);
protected:
    CRaftClientPool *m_pClientPool;
    uint32_t m_uSubSessionID;
};
#if 0
///\brief 管理员客户端
class LIBRAFTCORE_API CRaftAdminClient
{
public:
    CRaftAdminClient();

    virtual ~CRaftAdminClient();

    virtual bool Init(CRaftClientPool *pClientPool);

    virtual void Uninit(void);
protected:
    CRaftClientPool *m_pClientPool;
};

#endif

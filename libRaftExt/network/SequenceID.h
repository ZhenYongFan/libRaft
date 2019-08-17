#pragma once
#include "libRaftExt.h"

///\brief 64K个流水号
class LIBRAFTEXT_API CSequenceID
{
public:
    ///\brief 构造函数
    ///\param u32BaseID 序列基值，君子协议：要求基值小于等于0xFFFF - 64K
    CSequenceID(uint32_t u32BaseID);

    ///\brief 析构函数
    ~CSequenceID(void);

    ///\brief 取得一个序列号
    ///\param nSeqID 如果成功返回的序列号
    ///\return 成功标志 true 成功；false 失败
    ///\attention 最终需要调用FreeSeqID释放序列号
    bool AllocSeqID(uint32_t &nSeqID);

    ///\brief 释放一个序列号，以便重新分配
    ///\param nSeqID 不再使用的序列号
    void FreeSeqID(uint32_t nSeqID);
protected:
    ///\brief 取得64位整数中第一个（从低端开始）为0的位
    ///\param u64Value 要判断的64位整数
    ///\return 第一个（从低端开始）为0的位
    int GetZeroBit(uint64_t u64Value);
public:
    uint64_t *m_pu64Mask; ///< 用64位整数表示64K位，只要1024个元素
    uint32_t m_u32BaseID; ///< 基值
    uint32_t m_nCurIdx;   ///< 当前分配索引，用于快速定位
    std::list<uint32_t> m_listFreeSeq; ///< 释放的Seq列表，用于快速重新分配
    std::mutex m_mutexSeq;             ///< 多线程保护
};


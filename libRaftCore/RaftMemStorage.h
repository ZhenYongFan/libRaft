#pragma once
#include "libRaftCore.h"
#include "RaftDef.h"
#include "RaftStorage.h"

///\brief 用内存保存Raft日志的稳定部分，对应CUnstableLog
class LIBRAFTCORE_API CRaftMemStorage :public CRaftStorage
{
public:
    ///\brief 构造函数
    ///\param pLogger 用于输出日志的对象
    ///\param pRaftSerializer 串行化对象
    CRaftMemStorage(CLogger *pLogger, CRaftSerializer *pRaftSerializer = NULL);

    ///\brief 析构函数
    virtual ~CRaftMemStorage(void);

    virtual int InitialState(CHardState &hs, CConfState &cs);

    virtual int SetHardState(const CHardState&);

    ///\brief 取得第一个日志的索引号
    ///\param u64Index 第一个日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int FirstIndex(uint64_t &u64Index);

    ///\brief 取得最后一个日志的索引号
    ///\param u64Index 最后一个日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int LastIndex(uint64_t &u64Index);

    ///\brief 设置提交日志的索引号
    ///\param u64Committed 提交日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int SetCommitted(uint64_t u64Committed);

    ///\brief 设置应用日志的索引号
    ///\param u64tApplied 应用日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int SetApplied(uint64_t u64tApplied);
    
    virtual int Append(const EntryVec& entries);

    ///\brief 根据日志索引号取得对应的任期号
    ///\param u64Index 日志索引号
    ///\param u64Term 取得的对应任期号
    ///\return 成功标志 OK 成功；其他失败
    virtual int Term(uint64_t u64Index, uint64_t &u64Term);

    ///\brief 按索引号范围（闭区间），在满足空间限制的前提下读取一组日志
    ///\param u64Low 起始索引号
    ///\param u64High 终止索引号
    ///\param u64MaxSize 日志占用内存大小
    ///\param vecEntries 返回的日志数组
    ///\attention 如果u64MaxSize ==0则返回第一条符合条件的日志
    virtual int Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<CRaftEntry> &entries);
    
    virtual int GetSnapshot(CSnapshot **pSnapshot);
    
    virtual int Compact(uint64_t compactIndex);
    
    virtual int ApplySnapshot(const CSnapshot& snapshot);
    
    virtual int CreateSnapshot(uint64_t i, CConfState *cs, const string& data, CSnapshot *ss);

private:
    ///\brief 内部函数，取得第一个日志的索引号
    uint64_t firstIndex(void);

    ///\brief 内部函数，取得最后一个日志的索引号
    uint64_t lastIndex(void);
public:
    CHardState hardState_;
    CSnapshot  *m_pSnapShot;
    // ents[i] has raft log position i+snapshot.Metadata.Index
    EntryVec entries_;

    std::mutex m_mutexStorage; ///< 多线程保护
    CLogger *m_pLogger;        ///< 日志输出器
};


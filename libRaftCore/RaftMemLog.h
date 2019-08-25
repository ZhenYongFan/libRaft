#pragma once

#include "libRaftCore.h"
#include "RaftLog.h"
#include "UnstableLog.h"
#include "RaftStorage.h"

///\brief 基于内存的日志管理
class LIBRAFTCORE_API CRaftMemLog :public CRaftLog
{
public:
    ///\brief 构造函数
    ///\param pStorage Raft日志的持久化管理器
    ///\param pLogger 调试用的日志输出器
    CRaftMemLog(CRaftStorage *pStorage, CLogger *pLogger);

    ///\brief 析构函数
    virtual ~CRaftMemLog(void);

    ///\brief 取得第一条日志的索引号
    ///\return 取得的索引号
    virtual uint64_t GetFirstIndex(void);

    ///\brief 取得最后一条日志的索引号
    ///\return 取得的索引号
    virtual uint64_t GetLastIndex(void);

    ///\brief 按索引号取得日志的任期值
    ///\param u64Index 索引号
    ///\param u64Term  取得的任期值
    ///\return 错误号 OK 成功；其他 失败
    virtual int GetTerm(uint64_t u64Index, uint64_t &u64Term);
    
    ///\brief 判断索引号和任期值是否匹配
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 匹配标志  true 匹配; false 不匹配
    virtual bool IsMatchTerm(uint64_t u64Index, uint64_t u64Term);

    ///\brief 取得最后一条日志对应的任期值
    ///\return 取得的任期值
    virtual uint64_t GetLastTerm(void);

    ///\brief 取得日志集合
    ///\param u64Index 起始索引号
    ///\param u64MaxSize 数据最大值
    ///\param entries 取得的日志集合
    ///\return 成功标志  OK 成功；其他 失败
    virtual int GetEntries(uint64_t u64Index, uint64_t u64MaxSize, EntryVec &entries);

    ///\brief 取得日志集合
    ///\param u64Low 起始索引号
    ///\param u64High 终止索引号
    ///\param u64MaxSize 数据最大值
    ///\param entries 取得的日志集合
    ///\return 成功标志  OK 成功；其他 失败
    virtual int GetSliceEntries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, EntryVec &entries);

    ///\brief 追加写日志集合
    ///\param entries 日志集合
    ///\return 最后一条日志的索引号
    virtual uint64_t Append(const EntryVec& entries);

    ///\brief 判断是否可以更新索引号和任期值，作为投赞成票的依据
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 是否可以更新  true 可以 任期比本节点最后任期晚或者相同任期但是索引号比本机最后索引号大; false 不可以
    virtual bool IsUpToDate(uint64_t u64Index, uint64_t u64Term);

    ///\brief 尝试提交
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 是否提交成功  true 成功 u64Index大于当前提交号并且对应任期值; false 失败
    virtual bool MaybeCommit(uint64_t u64Index, uint64_t u64Term);

    ///\brief 尝试追加
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\param u64Committed 提交号
    ///\param entries 日志集合
    ///\param u64LastIndex 追加成功返回的最后一条日志的索引号
    ///\return 是否追加成功  true 成功 ; false 失败
    virtual bool MaybeAppend(uint64_t u64Index, uint64_t u64LogTerm,
        uint64_t u64Committed, const EntryVec& entries, uint64_t &u64LastIndex);

    ///\brief 提交日志
    ///\param u64Committed 提交索引号
    ///\attention 成功条件：提交号大于现有提交号小于等于最后索引号
    virtual bool CommitTo(uint64_t u64Committed);

    ///\brief 取得已经提交的日志号
    virtual uint64_t GetCommitted(void);

    ///\brief 应用日志
    ///\param u64Committed 应用索引号
    ///\attention 成功条件：应用号大于现有应用号小于等于提交号
    virtual void AppliedTo(uint64_t u64Applied);

    ///\brief 取得已经应用的日志号
    virtual uint64_t GetApplied(void);

    ///\brief 根据错误号过滤任期号
    ///\param u64Term  任期
    ///\param nErrorNo 错误号
    ///\return 如果成功则返回任期号，如果错误号是ErrCompacted，返回0，其他则是严重错误
    virtual uint64_t ZeroTermOnErrCompacted(uint64_t u64Term, int nErrorNo);

    ///\brief 取得所有非持久化日志
    ///\param entries 取得的日志集合
    virtual void UnstableEntries(EntryVec &entries);

    virtual int snapshot(CSnapshot **snapshot);

    virtual void Restore(const CSnapshot& snapshot);

    virtual int InitialState(CHardState &hs, CConfState &cs);

    string String(void);

    uint64_t findConflict(const EntryVec& entries);

    void unstableEntries(EntryVec &entries);

    void nextEntries(EntryVec& entries);

    bool hasNextEntries(void);
    
    void StableTo(uint64_t u64Index, uint64_t u64Term);

    void stableSnapTo(uint64_t i);

    void allEntries(EntryVec &entries);
    
    ///\brief 检查日志边界的合法性
    int CheckOutOfBounds(uint64_t u64Low, uint64_t u64High);
public:
    CRaftStorage *m_pStorage; ///< 持久化日志存储，保存自上次快照之后的日志数据

    // unstable contains all unstable entries and snapshot.
    // they will be saved into storage.
    CUnstableLog m_unstablePart;

    CLogger *m_pLogger; ///< 输出日志的对象
};

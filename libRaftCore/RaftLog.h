#pragma once

#include "libRaftCore.h"
#include "RaftDef.h"

///\brief Raft算法中的日志，类似WAL
class LIBRAFTCORE_API CRaftLog
{
public:
    ///\brief 构造函数
    CRaftLog(void)
    {
    };

    ///\brief 析构函数
    virtual ~CRaftLog(void)
    {
    };

    ///\brief 取得第一条日志的索引号
    ///\return 取得的索引号
    virtual uint64_t GetFirstIndex(void) = 0;

    ///\brief 取得最后一条日志的索引号
    ///\return 取得的索引号
    virtual uint64_t GetLastIndex(void) = 0;

    ///\brief 按索引号取得日志的任期值
    ///\param u64Index 索引号
    ///\param u64Term  取得的任期值
    ///\return 错误号 OK 成功；其他 失败
    virtual int GetTerm(uint64_t u64Index, uint64_t &u64Term) = 0;

    ///\brief 判断索引号和任期值是否匹配
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 匹配标志  true 匹配; false 不匹配
    virtual bool IsMatchTerm(uint64_t u64Index, uint64_t u64Term) = 0;

    ///\brief 取得最后一条日志对应的任期值
    ///\return 取得的任期值
    virtual uint64_t GetLastTerm(void) = 0;

    ///\brief 取得日志集合
    ///\param u64Index 起始索引号
    ///\param u64MaxSize 数据最大值
    ///\param entries 取得的日志集合
    ///\return 成功标志  OK 成功；其他 失败
    virtual int GetEntries(uint64_t u64Index, uint64_t u64MaxSize, EntryVec &entries) = 0;

    ///\brief 取得日志集合
    ///\param u64Low 起始索引号
    ///\param u64High 终止索引号
    ///\param u64MaxSize 数据最大值
    ///\param entries 取得的日志集合
    ///\return 成功标志  OK 成功；其他 失败
    virtual int GetSliceEntries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, EntryVec &entries) = 0;

    ///\brief 追加写日志集合
    ///\param entries 日志集合
    ///\return 最后一条日志的索引号
    virtual uint64_t Append(const EntryVec& entries) = 0;

    ///\brief 判断是否可以更新索引号和任期值，作为投赞成票的依据
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 是否可以更新  true 可以 任期比本节点最后任期晚或者相同任期但是索引号比本机最后索引号大; false 不可以
    virtual bool IsUpToDate(uint64_t u64Index, uint64_t u64Term) = 0;

    ///\brief 尝试提交
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\return 是否提交成功  true 成功 u64Index大于当前提交号并且对应任期值; false 失败
    virtual bool MaybeCommit(uint64_t u64Index, uint64_t u64Term) = 0;

    ///\brief 尝试追加
    ///\param u64Index 索引号
    ///\param u64Term  任期值
    ///\param u64Committed 提交号
    ///\param entries 日志集合
    ///\param u64LastIndex 追加成功返回的最后一条日志的索引号
    ///\return 是否追加成功  true 成功 ; false 失败
    virtual bool MaybeAppend(uint64_t u64Index, uint64_t u64LogTerm,
        uint64_t u64Committed, const EntryVec& entries, uint64_t &u64LastIndex) = 0;

    ///\brief 取得已经提交的日志号
    virtual uint64_t GetCommitted(void) = 0;

    ///\brief 提交日志
    ///\param u64Committed 提交索引号
    ///\attention 成功条件：提交号大于现有提交号小于等于最后索引号
    virtual bool CommitTo(uint64_t u64Committed) = 0;

    ///\brief 取得已经应用的日志号
    virtual uint64_t GetApplied(void) = 0;

    ///\brief 应用日志
    ///\param u64Committed 应用索引号
    ///\attention 成功条件：应用号大于现有应用号小于等于提交号
    virtual void AppliedTo(uint64_t u64Applied) = 0;

    ///\brief 根据错误号过滤任期号
    ///\param u64Term  任期
    ///\param nErrorNo 错误号
    ///\return 如果成功则返回任期号，如果错误号是ErrCompacted，返回0，其他则是严重错误
    virtual uint64_t ZeroTermOnErrCompacted(uint64_t u64Term, int nErrorNo) = 0;

    ///\brief 取得所有非持久化日志
    ///\param entries 取得的日志集合
    virtual void UnstableEntries(EntryVec &entries) = 0;

    virtual int snapshot(CSnapshot **snapshot) = 0;

    virtual void Restore(const CSnapshot& snapshot) = 0;

    virtual int InitialState(CHardState &hs, CConfState &cs) = 0;
};

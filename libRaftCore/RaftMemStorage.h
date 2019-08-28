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

    ///\brief 取得初始化状态
    ///\param hs 返回的持久化状态
    ///\param cs 返回的节点信息
    ///\return 成功标志 OK 成功；其他 失败
    virtual int InitialState(CHardState &hs, CConfState &cs);

    ///\brief 设置持久性状态
    ///\param stateHard 持久性状态
    ///\return 成功标志 OK 成功；其他 失败
    virtual int SetHardState(const CHardState &stateHard);

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
    
    ///\brief 追加日志集合
    ///\param entries 要追加的日志集合
    ///\return 成功标志 OK 成功；其他失败      
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
       
    ///\brief 按指定的索引号压缩日志
    ///\param u64CompactIndex 索引号，放弃之前的全部日志
    ///\return 成功标志 OK 成功；其他失败
    ///\attention 千万不要压缩大于应用索引号之后的日志呀
    virtual int Compact(uint64_t u64CompactIndex);
   
    ///\brief 创建快照
    ///\param u64Index 日志索引号
    ///\param pConfState 如果不为空，则替代快照的节点ID集合
    ///\param strData  快照的数据
    ///\param pSnapshot 如果不为空，返回当前的快照.
    ///\return 成功标志 OK 成功；其他失败
    virtual int CreateSnapshot(uint64_t u64Index, CConfState *pConfState, const string& strData, CSnapshot *pSnapshot);

    ///\brief 取得当前快照
    ///\param snapshot 返回的快照对象
    ///\return 成功标志 OK 成功；其他失败
    virtual int GetSnapshot(CSnapshot & snapshot);

    ///\brief 应用快照
    ///\param snapshot 外部传入的快照
    ///\return  成功标志 OK 成功；其他失败
    virtual int ApplySnapshot(const CSnapshot& snapshot);

private:
    ///\brief 内部函数，取得第一个内存日志的索引号
    uint64_t GetFirstIdx(void);

    ///\brief 内部函数，取得最后一个内存日志的索引号
    uint64_t GetLastIdx(void);
public:
    CHardState m_stateHard;    ///< 可持久化状态
    CSnapshot  *m_pSnapShot;   ///< 快照
    EntryVec m_entries;        ///< 内存中的日志，索引号是数组序号+快照基值索引号

    std::mutex m_mutexStorage; ///< 多线程保护
    CLogger *m_pLogger;        ///< 日志输出器
};


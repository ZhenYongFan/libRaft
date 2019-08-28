#pragma once
#include "libRaftCore.h"
#include "RaftDef.h"
class CRaftSerializer;

///\brief 保存Raft日志的稳定部分，一般采用持久化存储模式，对应CUnstableLog
class LIBRAFTCORE_API CRaftStorage
{
public:
    ///\brief 构造函数
    ///\param pRaftSerializer 串行化对象
    CRaftStorage(CRaftSerializer *pRaftSerializer = NULL)
    {
        m_u64Committed = 0;
        m_u64Applied = 0;
        m_pRaftSerializer = pRaftSerializer;
    };

    ///\brief 析构函数
    virtual ~CRaftStorage(void)
    {
    }

    ///\brief 取得当前的串行化对象
    CRaftSerializer *GetSerializer(void)
    {
        return m_pRaftSerializer;
    }

    ///\brief 取得初始化状态
    ///\param hs 返回的持久化状态
    ///\param cs 返回的节点信息
    ///\return 成功标志 OK 成功；其他 失败
    virtual int InitialState(CHardState &hs, CConfState &cs) = 0;

    ///\brief 设置持久性状态
    ///\param stateHard 持久性状态
    ///\return 成功标志 OK 成功；其他 失败
    virtual int SetHardState(const CHardState &stateHard) = 0;

    ///\brief 取得第一个日志的索引号
    ///\param u64Index 第一个日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int FirstIndex(uint64_t &u64Index) = 0;

    ///\brief 取得最后一个日志的索引号
    ///\param u64Index 最后一个日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int LastIndex(uint64_t &u64Index) = 0;

    ///\brief 设置提交日志的索引号
    ///\param u64Committed 提交日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int SetCommitted(uint64_t u64Committed) = 0;

    ///\brief 设置应用日志的索引号
    ///\param u64tApplied 应用日志的索引号
    ///\return 成功标志 OK 成功；其他失败
    virtual int SetApplied(uint64_t u64tApplied) = 0;

    ///\brief 追加日志集合
    ///\param entries 要追加的日志集合
    ///\return 成功标志 OK 成功；其他失败      
    virtual int Append(const EntryVec& entries) = 0;

    ///\brief 根据日志索引号取得对应的任期号
    ///\param u64Index 日志索引号
    ///\param u64Term 取得的对应任期号
    ///\return 成功标志 OK 成功；其他失败
    virtual int Term(uint64_t u64Index, uint64_t &u64Term) = 0;

    ///\brief 按索引号范围（闭区间），在满足空间限制的前提下读取一组日志
    ///\param u64Low 起始索引号
    ///\param u64High 终止索引号
    ///\param u64MaxSize 日志占用内存大小
    ///\param vecEntries 返回的日志数组
    ///\attention 如果u64MaxSize ==0则返回第一条符合条件的日志
    virtual int Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<CRaftEntry> &entries) = 0;

    ///\brief 创建快照
    ///\param u64Index 日志索引号
    ///\param pConfState 如果不为空，则替代快照的节点ID集合
    ///\param strData  快照的数据
    ///\param pSnapshot 如果不为空，返回当前的快照.
    ///\return 成功标志 OK 成功；其他失败
    virtual int CreateSnapshot(uint64_t u64Index, CConfState *pConfState, const string& strData, CSnapshot *pSnapshot) = 0;

    ///\brief 取得当前快照
    ///\param snapshot 返回的快照对象
    ///\return 成功标志 OK 成功；其他失败
    virtual int GetSnapshot(CSnapshot & snapshot) = 0;
    
public:
    ///\brief 提交的日志号
    uint64_t m_u64Committed;

    ///\brief 应用的日志号
    ///\attention 应用和提交日志号的关系 m_u64Applied <= m_u64Committed 
    uint64_t m_u64Applied;
    
    ///\brief 串行化
    CRaftSerializer *m_pRaftSerializer;
};



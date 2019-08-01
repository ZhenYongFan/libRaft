#pragma once
#include "RaftStorage.h"
namespace rocksdb
{
    class DB;
};
namespace raftpb
{
    class Snapshot;
};

///\brief 采用RocksDB实现日志的持久化存储
class LIBRAFTCORE_API CRocksDbStorage : public CRaftStorage
{
public:
    ///\brief 构造函数
    ///\param pLogger 日志输出
    CRocksDbStorage(CLogger * pLogger);

    ///\brief 析构函数
    virtual ~CRocksDbStorage(void);

    ///\brief 初始化
    ///\param strDbPath 保存数据的目录
    ///\return 成功标志 0 成功，其他失败
    virtual int Init(std::string &strDbPath);
    
    ///\brief 释放资源
    virtual void Uninit(void);
    
    ///\brief 取得第一个索引号
    ///\param u64Index 返回的索引号
    ///\return 成功标志 0 成功，其他失败
    virtual int FirstIndex(uint64_t &u64Index);

    ///\brief 取得最后一个索引号
    ///\param u64Index 返回的索引号
    ///\return 成功标志 0 成功，其他失败
    virtual int LastIndex(uint64_t &u64Index);

    ///\brief 设置提交索引号
    ///\param u64Committed 提交索引号
    ///\return 成功标志 0 成功，其他失败
    virtual int SetCommitted(uint64_t u64Committed);

    ///\brief 设置应用索引号
    ///\param u64tApplied 提交应用号
    ///\return 成功标志 0 成功，其他失败
    virtual int SetApplied(uint64_t u64tApplied);

    ///\brief 取得提交索引号
    ///\param u64Committed 返回的提交索引号
    ///\return 成功标志 0 成功，其他失败
    virtual int GetCommitted(uint64_t &u64Committed);

    ///\brief 取得应用索引号
    ///\param u64Applied 返回提交应用号
    ///\return 成功标志 0 成功，其他失败
    virtual int GetApplied(uint64_t &u64Applied);

    ///\brief 取得日志中对应索引号的任期号
    ///\param u64Index 索引号
    ///\param u64Term 取得的任期号
    ///\return 成功标志 0 成功，其他失败
    virtual int Term(uint64_t u64Index, uint64_t &u64Term);

    ///\brief 追加日志到存储中
    ///\param entries 日志数组
    ///\return 成功标志 0 成功，其他失败
    virtual int Append(const EntryVec& entries);

    ///\brief 读取日志
    ///\param u64Low 起始索引号
    ///\param u64High 终止索引号
    ///\param u64MaxSize 最大容量，字节数
    ///\param entries 返回的日志数组
    ///\return 成功标志 0 成功，其他失败
    virtual int Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<Entry> &entries);

    virtual int InitialState(HardState &stateHard, ConfState &stateConfig);
    
    virtual int SetHardState(const HardState &stateHard);
    
    virtual int GetSnapshot(raftpb::Snapshot **snapshot);
    
    virtual int CreateSnapshot(uint64_t i, ConfState *cs, const string& data, raftpb::Snapshot *ss);
protected:

    ///\brief 通过判断迭代器是否有效判断数据库是否为空
    ///\return true 没有数据 ；false 有数据
    bool IsEmpty(void);
protected:
    rocksdb::DB * m_pStocksDB;      ///< RocksDB对象
    raftpb::Snapshot * m_pSnapShot; ///< 快照
    CLogger * m_pLogger;            ///< 用于输出日志
    HardState m_stateHard;
    uint64_t m_u64FirstIndex; ///< 第一个索引号
    uint64_t m_u64LastIndex ; ///< 最后一个索引号
};

#pragma once
#include "KvService.h"

namespace rocksdb
{
    class DB;
};

///\brief 使用Rocksdb实现KV服务
class LIBRAFTEXT_API CKvRocksdbService :public CKvService
{
public:
    ///\brief 构造函数
    CKvRocksdbService(void);

    ///\brief 析构函数
    virtual ~CKvRocksdbService(void);
    
    ///\brief 初始化
    ///\param strDbPath 存放数据的目录
    ///\return 成功标志 0 成功；其他 失败
    virtual int Init(std::string &strDbPath);

    ///\brief 释放资源
    virtual void Uninit(void);

    ///\brief Put操作
    ///\param strKey Key
    ///\param strValue Value
    ///\return 成功标志 0 成功；其他 失败
    virtual int Put(const std::string &strKey, const std::string &strValue);

    ///\brief Put操作
    ///\param strKey Key
    ///\param strValue 如果成功返回的Value
    ///\return 成功标志 0 成功；其他 失败
    virtual int Get(const std::string &strKey, std::string &strValue);

    ///\brief Delete操作
    ///\param strKey Key
    ///\return 成功标志 0 成功；其他 失败
    virtual int Delete(const std::string &strKey);
protected:
    rocksdb::DB * m_pStocksDB; ///< RocksDB对象
};

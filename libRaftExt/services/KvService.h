#pragma once
#include "libRaftExt.h"

///\brief KV存储
class LIBRAFTEXT_API CKvService
{
public:
    ///\brief 构造函数
    CKvService()
    {
    };

    ///\brief 析构函数
    virtual ~CKvService()
    {
    };

    ///\brief 初始化
    ///\param strDbPath 存放持久化数据的目录
    ///\return 成功标志 0 成功；其他 失败
    virtual int Init(std::string &strDbPath) = 0;

    ///\brief 释放资源
    virtual void Uninit(void) = 0;

    ///\brief Put（C+U）操作
    ///\param strKey Key
    ///\param strValue Value
    ///\return 成功标志 0 成功；其他 失败
    virtual int Put(const std::string &strKey, const std::string &strValue) = 0;

    ///\brief Get（R）操作
    ///\param strKey Key
    ///\param strValue 如果成功返回的Value
    ///\return 成功标志 0 成功；其他 失败
    virtual int Get(const std::string &strKey, std::string &strValue) = 0;

    ///\brief Delete（D）操作
    ///\param strKey Key
    ///\return 成功标志 0 成功；其他 失败
    virtual int Delete(const std::string &strKey) = 0;
};


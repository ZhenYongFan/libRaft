#pragma once
#include "libRaftCore.h"

#ifndef _WIN64
#include <semaphore.h>
#include <errno.h>
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0  0
#endif

#ifndef INFINITE
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif

#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT                     258L    // dderror
#endif

#ifndef WAIT_ABANDONED
#define WAIT_ABANDONED 0x80
#endif
#endif

///\brief 无名信号量的封装
class LIBRAFTCORE_API CRaftSemaphore
{
public:
    ///\brief 构造函数
    CRaftSemaphore(void);

    ///\brief 析构函数
    ~CRaftSemaphore(void);

    ///\brief 产生
    ///\param nInitValue 初始值
    ///\return true 成功 false 失败
    bool Create(int nInitValue);

    ///\brief 释放资源
    void Destroy(void);

    ///\brief 等待一个信号
    ///\param	dwMilliseconds
    ///\return 成功标志 WAIT_OBJECT_0 WAIT_TIMEOUT WAIT_ABANDONED
    uint32_t Wait(uint32_t dwMilliseconds);

    ///\brief 发送一个信号
    bool Post(int nCount);

protected:
#ifdef _WIN64
    HANDLE m_hSem;   ///< 信号量句柄
#else
    sem_t *m_hSem;    ///< 信号量
#endif
};

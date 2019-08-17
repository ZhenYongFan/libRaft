#pragma once
#include "libRaftCore.h"
#include <thread>

///\brief 对libevent base的简单封装，启动一个线程负责event base的loop函数
class LIBRAFTCORE_API CEventBase
{
public:
    ///\brief 构造函数
    CEventBase(void);

    ///\brief 析构函数
    virtual ~CEventBase(void);
    
    ///\brief 初始化
    ///\param  成功标志 true 成功； false 失败
    virtual bool Init(void);

    ///\brief 释放资源
    virtual void Uninit(void);

    ///\brief 启动线程
    ///\return 成功标志 0 成功； 1 已经启动；2 启动失败（创建线程失败）
    virtual int Start(void);

    ///\brief 设置标志通知即将停止
    virtual void NotifyStop(void);

    ///\brief 停止event base，等待线程停止
    virtual void Stop(void);

    ///\brief 取得event base对象
    struct event_base *GetBase(void)
    {
        return m_pBase;
    };
    ///\brief 取得当前状态
    int GetState(void)
    {
        return m_nIoState;
    }
protected:
    ///\brief 线程函数的Shell
    static void WorkShell(void *pContext);

    ///\brief 线程函数，执行event base 的loop函数
    virtual void WorkFunc(void);
protected:
    struct event_base *m_pBase; ///< libevent对象
    std::thread *m_pThread;     ///< 工作线程
    volatile int m_nIoState;    ///< 工作状态 0 未初始化 ;1 初始化；2 已启动 ；3 正在停止 ；4 已经停止
};

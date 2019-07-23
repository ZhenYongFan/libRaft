#pragma once
#include "libRaftCore.h"
#include "RaftSemaphore.h"

///\brief 生产者，消费者模式用的队列，对应go的channel
class LIBRAFTCORE_API CRaftQueue
{
public:
    ///\brief 构造函数
    CRaftQueue(void);

    ///\brief 析构函数
    virtual ~CRaftQueue(void);

    ///\brief 初始化，创建信号灯
    ///\param nMaxSpace 可以在队列中对象的最大个数
    ///\param strErrMsg 如果失败返回的错误信息
    ///\return 成功标志 true 成功；false 失败
    bool Init(int nMaxSpace,std::string &strErrMsg);

    ///\brief 压栈，将对象放入队列
    ///\param pObject 对象指针
    ///\param uTimeout 超时，一般在抛弃型场景中，设置为0
    ///\return 成功标志 true 成功；false 失败，一般是超时
    virtual bool Push(void *pObject, uint32_t uTimeout = INFINITE);

    ///\brief 弹出栈，从队列中取出对象
    ///\param uTimeout 超时
    ///\return 对象指针,注意：可能取得对象指针为NULL
    virtual void *Pop(uint32_t uTimeout = INFINITE);

protected:
    std::list<void *> m_listMsg; ///< 用列表保存对象指针
    std::mutex m_mutxQueue;      ///< 多线程保护
    CRaftSemaphore m_semSpace;   ///< 空间信号灯
    CRaftSemaphore m_semObj;     ///< 对象信号灯
};

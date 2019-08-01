#pragma once
#include "libRaftCore.h"
#include "RaftLogger.h"

#include <mutex>
#include <log4cxx/logger.h>
#include <log4cxx/level.h>

///\brief 采用log4cxx的日志输出器
class LIBRAFTCORE_API CLog4CxxLogger :public CLogger
{
public:
    ///\brief 构造函数
    CLog4CxxLogger(void);

    ///\brief 析构函数
    virtual ~CLog4CxxLogger(void);

    ///\brief 设置Log4Cxx对象
    ///\param logger Log4Cxx对象
    void SetLogger(log4cxx::LoggerPtr logger);

    ///\brief 取得当前的Log4Cxx对象
    log4cxx::LoggerPtr & GetLogger(void);

    ///\brief 输出Debug级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Debugf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...);

    ///\brief 输出Info级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Infof(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...);

    ///\brief 输出Warning级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Warningf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...);

    ///\brief 输出Error级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Errorf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...);

    ///\brief 输出Fatal级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    ///\attention 一般此时会打出栈信息后退出
    virtual void Fatalf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...);

    ///\brief 设置日志等级
    ///\param levelLog "TRACE" < "DEBUG" < "INFO" < "WARN" < "ERROR" < "FATAL"
    ///\return 设置成功标志 true 成功 false 失败
    bool SetLevel(const log4cxx::LogString &levelLog);

protected:
    ///\brief 输出日志
    ///\param nLevel 日志级别 0：FATAL；1：ERROR；2：WARN；3：INFO；其他 DEBUG
    ///\param pstrLogMsg 日志信息
    virtual void OutputLog(int nLevel,const char *pstrLogMsg);
protected:
    log4cxx::LoggerPtr m_logger ; ///< 日志对象
    std::mutex m_mutxLog;         ///< 多线程保护
};


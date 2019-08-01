#pragma once
#include "libRaftCore.h"

///\brief 用于运维用的日志输出器,此为抽象类
class LIBRAFTCORE_API CLogger
{
public:
    ///\brief 输出Debug级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Debugf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...) = 0;

    ///\brief 输出Info级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Infof(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...) = 0;

    ///\brief 输出Warning级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Warningf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...) = 0;

    ///\brief 输出Error级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    virtual void Errorf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...) = 0;

    ///\brief 输出Fatal级别日志
    ///\param pstrFileName 对应的文件名
    ///\param nLineNo 对应的行号
    ///\param pstrFormat 日志格式化参数
    ///\attention 一般此时会打出栈信息后退出
    virtual void Fatalf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...) = 0;
};

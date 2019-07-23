#include "stdafx.h"
#include <stdarg.h>
#include "Log4CxxLogger.h"

using namespace log4cxx;
const int cnMaxMsgLen = 4096;

CLog4CxxLogger::CLog4CxxLogger(void)
{
}

CLog4CxxLogger::~CLog4CxxLogger(void)
{
}

void CLog4CxxLogger::Debugf(const char *pstrFileName, int nLineNo, const char *pstrFormat, ...)
{
    if (NULL != m_logger)
    {
        char pstrLogMsg[cnMaxMsgLen];
        int nFileLine = snprintf(pstrLogMsg, sizeof(pstrLogMsg), "%s:%d\t", pstrFileName, nLineNo);
        if (nFileLine < cnMaxMsgLen)
        {
            va_list ArgList;
            va_start(ArgList, pstrFormat);
            vsnprintf(pstrLogMsg + nFileLine, sizeof(pstrLogMsg) - nFileLine, pstrFormat, ArgList);
            va_end(ArgList);
        }
        OutputLog(4, pstrLogMsg);
    }
}

void CLog4CxxLogger::Infof(const char *pstrFileName, int nLineNo, const char *format, ...)
{
    if (NULL != m_logger)
    {
        char pstrLogMsg[cnMaxMsgLen];
        int nFileLine = snprintf(pstrLogMsg, sizeof(pstrLogMsg), "%s:%d\t", pstrFileName, nLineNo);
        if (nFileLine < cnMaxMsgLen)
        {
            va_list ArgList;
            va_start(ArgList, format);
            vsnprintf(pstrLogMsg + nFileLine, sizeof(pstrLogMsg) - nFileLine, format, ArgList);
            va_end(ArgList);
        }
        OutputLog(3, pstrLogMsg);
    }
}

void CLog4CxxLogger::Warningf(const char *pstrFileName, int nLineNo, const char *format, ...)
{
    if (NULL != m_logger)
    {
        char pstrLogMsg[cnMaxMsgLen];
        int nFileLine = snprintf(pstrLogMsg, sizeof(pstrLogMsg), "%s:%d\t", pstrFileName, nLineNo);
        if (nFileLine < cnMaxMsgLen)
        {
            va_list ArgList;
            va_start(ArgList, format);
            vsnprintf(pstrLogMsg + nFileLine, sizeof(pstrLogMsg) - nFileLine, format, ArgList);
            va_end(ArgList);
        }
        OutputLog(2, pstrLogMsg);
    }
}

void CLog4CxxLogger::Errorf(const char *pstrFileName, int nLineNo, const char *format, ...)
{
    if (NULL != m_logger)
    {
        char pstrLogMsg[cnMaxMsgLen];
        int nFileLine = snprintf(pstrLogMsg, sizeof(pstrLogMsg), "%s:%d\t", pstrFileName, nLineNo);
        if (nFileLine < cnMaxMsgLen)
        {
            va_list ArgList;
            va_start(ArgList, format);
            vsnprintf(pstrLogMsg + nFileLine, sizeof(pstrLogMsg) - nFileLine, format, ArgList);
            va_end(ArgList);
        }
        OutputLog(1, pstrLogMsg);
    }
}

void CLog4CxxLogger::Fatalf(const char *pstrFileName, int nLineNo, const char *format, ...)
{
    if (NULL != m_logger)
    {
        char pstrLogMsg[cnMaxMsgLen];
        int nFileLine = snprintf(pstrLogMsg, sizeof(pstrLogMsg), "%s:%d\t", pstrFileName, nLineNo);
        if (nFileLine < cnMaxMsgLen)
        {
            va_list ArgList;
            va_start(ArgList, format);
            vsnprintf(pstrLogMsg + nFileLine, sizeof(pstrLogMsg) - nFileLine, format, ArgList);
            va_end(ArgList);
        }
        OutputLog(0, pstrLogMsg);
    }
}

void CLog4CxxLogger::OutputLog(int nLevel, const char *pstrLogMsg)
{
    std::lock_guard<std::mutex> logGuard(m_mutxLog);
    switch (nLevel)
    {
    case 0:
        LOG4CXX_FATAL(m_logger, pstrLogMsg);
        break;
    case 1:
        LOG4CXX_ERROR(m_logger, pstrLogMsg);
        break;
    case 2:
        LOG4CXX_WARN(m_logger, pstrLogMsg);
        break;
    case 3:
        LOG4CXX_INFO(m_logger, pstrLogMsg);
        break;
    default :
        LOG4CXX_DEBUG(m_logger, pstrLogMsg);
        break;
    }
}

bool CLog4CxxLogger::SetLevel(const log4cxx::LogString &levelLog)
{
    bool bSet = false;
    if (NULL != m_logger)
    {
        //如果字符串不符合要求，则返回DEBUG级别的指针
        LevelPtr pLevel = Level::toLevel(levelLog);
        m_logger->setLevel(pLevel);
    }
    return bSet;
}

void CLog4CxxLogger::SetLogger(log4cxx::LoggerPtr logger)
{
    m_logger = logger;
}

log4cxx::LoggerPtr & CLog4CxxLogger::GetLogger(void)
{
    return m_logger;
}

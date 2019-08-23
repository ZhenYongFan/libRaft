#include "stdafx.h"
#include "RaftFrame.h"
#include "RaftConfig.h"
#include "RaftQueue.h"
#include "Raft.h"
#include "KvRocksdbService.h"
#include "RocksDbStorage.h"
#include "RaftMemLog.h"
#include "RaftUtil.h"
#include "RaftExtUtil.h"

#include "protobuffer/RaftProtoBufferSerializer.h"

#include "Log4CxxLogger.h"
#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

CRaftFrame::CRaftFrame(void)
{
    m_pConfig = NULL;
    m_pRaft = NULL;
    m_pRaftStorage = NULL;
    m_pRaftLog = NULL;
    m_pMsgQueue = NULL;
    m_pIoQueue = NULL;
    m_pLogger = NULL;
    m_pKvService = NULL;
    m_pSerializer = NULL;
}

CRaftFrame::~CRaftFrame(void)
{
}

bool CRaftFrame::InitLogger(std::string &strLogCfgFile, std::string &strErrMsg)
{
    bool bInit = false;
    PropertyConfigurator::configure(strLogCfgFile);
    //如果想检测日志是否初始化成功要用LogManager::exists
    //如果用Logger::getLogger则肯定生成一个对象
    LoggerPtr logger_raft(LogManager::exists("raftnode"));
    if (NULL != logger_raft)
    {
        CLog4CxxLogger *pLogger = new CLog4CxxLogger();
        pLogger->SetLogger(logger_raft);
        m_pLogger = pLogger;
        bInit = true;
    }
    return bInit;
}
bool CRaftFrame::InitCfg(std::string &strCfgFile, std::string &strErrMsg)
{
    m_pConfig = CRaftExtUtil::InitCfg(strCfgFile, strErrMsg);
    return (m_pConfig != NULL);
}

bool CRaftFrame::Init(std::string &strConfigPath, std::string &strErrMsg)
{
    bool bInit = false;
    std::string strLogCfgFile = strConfigPath + "/log4cxx.properties";
    if (InitLogger(strLogCfgFile, strErrMsg))
    {
        std::string strConfigFile = strConfigPath + "/raft.config";
        if (InitCfg(strConfigFile, strErrMsg))
        {
            m_pSerializer = new CRaftProtoBufferSerializer();
            bInit = InitRaft(strErrMsg);
        }
        else
            m_pLogger->Infof(__FILE__, __LINE__, "Error on load config %s \n", strErrMsg.c_str());
    }
    if (!bInit)
        Uninit();
    return bInit;
}

bool CRaftFrame::InitRaftLog(std::string &strErrMsg)
{
    bool bInit = false;
    m_pRaftStorage = new CRocksDbStorage(m_pLogger);
    CRaftMemLog *pRaftLog = new CRaftMemLog(m_pRaftStorage, m_pLogger);

    uint64_t firstIndex, lastIndex;
    int nErrorNo = m_pRaftStorage->FirstIndex(firstIndex);
    if (SUCCESS(nErrorNo))
    {
        nErrorNo = m_pRaftStorage->LastIndex(lastIndex);
        if (SUCCESS(nErrorNo))
        {
            pRaftLog->m_unstablePart.m_u64Offset = lastIndex + 1;
            pRaftLog->m_unstablePart.m_pLogger = m_pLogger;
            m_pRaftLog = pRaftLog;
            bInit = true;
        }
        else
            m_pLogger->Fatalf(__FILE__, __LINE__, "get last index err:%s", GetErrorString(nErrorNo));
    }
    else
        m_pLogger->Fatalf(__FILE__, __LINE__, "get first index err:%s", GetErrorString(nErrorNo));
    return bInit;
}

bool CRaftFrame::InitRaft(std::string &strErrMsg)
{
    bool bInit = false;
    m_pMsgQueue = new CRaftQueue();
    if (m_pMsgQueue->Init(1024, strErrMsg))
    {
        m_pIoQueue = new CRaftQueue();
        if (m_pIoQueue->Init(1024, strErrMsg))
        {
            if (InitRaftLog(strErrMsg))
            {
                m_pRaft = new CRaft(m_pConfig, m_pRaftLog, m_pMsgQueue, m_pIoQueue, m_pLogger);
                if (m_pRaft->Init(strErrMsg))
                {
                    m_pKvService = new CKvRocksdbService();
                    std::string strDbPath = m_pConfig->m_strDataPath + "/kv";
                    if (0 == m_pKvService->Init(strDbPath))
                        bInit = true;
                }
            }
        }
    }
    return bInit;
}

void CRaftFrame::Uninit(void)
{
    if (NULL != m_pKvService)
    {
        m_pKvService->Uninit();
        delete m_pKvService;
    }
    if (NULL != m_pRaft)
    {
        m_pRaft->Uninit();
        delete m_pRaft;
        m_pRaft = NULL;
    }
    if (NULL != m_pRaftLog)
    {
        delete m_pRaftLog;
        m_pRaftLog = NULL;
    }
    if (NULL != m_pRaftStorage)
    {
        delete m_pRaftStorage;
        m_pRaftStorage = NULL;
    }
    if (NULL != m_pIoQueue)
    {
        delete m_pIoQueue;
        m_pIoQueue = NULL;
    }
    if (NULL != m_pMsgQueue)
    {
        delete m_pMsgQueue;
        m_pMsgQueue = NULL;
    }
    if (NULL != m_pConfig)
    {
        delete m_pConfig;
        m_pConfig = NULL;
    }
}

#include "stdafx.h"
#include <signal.h>
#include <assert.h>

#include <event2/event.h>
#include <event2/thread.h>

#ifndef _WIN64
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/propertyconfigurator.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include "Log4CxxLogger.h"
#include "RaftConfig.h"
#include "RaftExtUtil.h"
#include "network/RaftClientPool.h"
#include "network/RaftAdminClient.h"

//语法
// set key value
// get key
// del key
// stop num
// resume num
// restart num
// transfer num
//
#include <iostream>
using namespace std;

void SplitString(const string& strText, const string &strSeparator, vector<string> &vecString)
{
    vecString.clear();
    int nTxtLen = static_cast<int>(strText.size());
    int nSpLen = static_cast<int>(strSeparator.size());
    int nLastPosition = 0;
    int nFoundPos = -1;
    //忽略前导空白字符
    while (nLastPosition < nTxtLen && isspace(strText[nLastPosition]))
        nLastPosition++;
    while (-1 != (nFoundPos = strText.find(strSeparator, nLastPosition)))
    {
        vecString.push_back(strText.substr(nLastPosition, nFoundPos - nLastPosition));
        nLastPosition = nFoundPos + nSpLen;
        //忽略中间空白字符
        while (nLastPosition < nTxtLen && isspace(strText[nLastPosition]))
            nLastPosition++;
    }
    string strLast = strText.substr(nLastPosition);
    if (!strLast.empty())
        vecString.push_back(strLast);
}

int Test(void)
{
    std::string strCurrentDir;
#ifdef _WIN64

#else
    strCurrentDir = get_current_dir_name();
#endif
    std::string strConfigPath = strCurrentDir + "/config";
    std::string strLogCfgFile = strConfigPath + "/log4cxx_client.properties";
    PropertyConfigurator::configure(strLogCfgFile);
    //如果想检测日志是否初始化成功要用LogManager::exists
    //如果用Logger::getLogger则肯定生成一个对象
    LoggerPtr logger_raft(LogManager::exists("raftnode"));
    if (NULL != logger_raft)
    {
        CLog4CxxLogger loggerRaft;
        loggerRaft.SetLogger(logger_raft);
        std::string strCfgFile = strConfigPath + "/raft_client.config";
        std::string strErrMsg;
        CRaftConfig *pConfig = CRaftExtUtil::InitCfg(strCfgFile,strErrMsg);
        if(pConfig != NULL)
        {
            loggerRaft.Infof(__FILE__, __LINE__, "Load config file ok");
            CRaftClientPool poolClient(NULL);
            poolClient.m_cfgSelf = *pConfig;
            delete pConfig;
            poolClient.SetLogger(&loggerRaft);
            if (poolClient.Init())
            {
                loggerRaft.Infof(__FILE__, __LINE__, "Init OK");
                if (0 == poolClient.Start())
                {
                    CRaftClient client;
                    if (client.Init(&poolClient))
                    {
                        loggerRaft.Infof(__FILE__, __LINE__, "Start OK");
                        bool bExit = false;
                        while (!bExit)
                        {
                            char cLines[256];
                            string strInput;
                            std::cout << "raft > ";
                            std::cin.getline(cLines,256);
                            strInput = cLines;
                            if (strInput == "exit")
                                bExit = true;
                            else
                            {
                                vector<string> vecString;
                                string strSeparator = " ";
                                SplitString(strInput, strSeparator, vecString);
                                if (vecString.size() == 2)
                                {
                                    if (vecString[0] == "get")
                                    {
                                        std::string strValue;
                                        int nGet = client.Get(vecString[1], strValue);
                                        cout << "get " << vecString[1] << " = " << strValue << ((0 == nGet) ? " OK " : " ERROR ") << "\n";
                                    }
                                    else
                                        cout << "error command :" << strInput << "\n";
                                }
                                else if (vecString.size() == 3)
                                {
                                    if (vecString[0] == "set")
                                    {
                                        int nPut = client.Put(vecString[1], vecString[2]);
                                        cout << "put " << vecString[1] << " = " << vecString[2] << ((0 == nPut) ? " OK " : " ERROR ") << "\n";
                                    }
                                    else
                                        cout << "error command :" << strInput << "\n";
                                }
                                else
                                {
                                    cout << "error command \n";
                                }
                            }
                        }
                        client.Uninit();
                    }
                    poolClient.NotifyStop();
                    poolClient.Stop();
                }
                else
                    loggerRaft.Warningf(__FILE__, __LINE__, "Start Error");
                poolClient.Uninit();
            }
            else
                loggerRaft.Warningf(__FILE__, __LINE__, "Init Error");
        }
        else
        {
            loggerRaft.Infof(__FILE__, __LINE__, "Error on load config %s \n", strErrMsg.c_str());
            printf("Error on load config %s \n", strErrMsg.c_str());
        }
    }
    else
        printf("Error on init log \n");       
    return 0;
}

int main(void)
{
#ifndef _WIN64
    evthread_use_pthreads();
#endif
    Test();
    return 0;
}
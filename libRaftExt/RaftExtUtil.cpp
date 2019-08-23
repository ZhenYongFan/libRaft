#include "stdafx.h"
#include "RaftExtUtil.h"
#include "RaftConfig.h"
#include "libconfig.h++"
using namespace libconfig;

CRaftConfig * CRaftExtUtil::InitCfg(std::string &strCfgFile, std::string &strErrMsg)
{
    CRaftConfig *pConfig = new CRaftConfig();
    strErrMsg.clear();
    try
    {
        Config cfgRaft;                          // 声明 Config对象
        cfgRaft.readFile(strCfgFile.c_str());    // 读取配置文件
        const Setting& root = cfgRaft.getRoot();

        const Setting &selfid = root["inventory"]["seftid"];
        pConfig->m_nRaftID = uint32_t(int(selfid));

        const Setting &setMsPerTick = root["inventory"]["MsPerTick"];
        const Setting &setHeartbeatTicks = root["inventory"]["HeartbeatTicks"];
        const Setting &setElectionTicks = root["inventory"]["ElectionTicks"];
        pConfig->m_nMsPerTick = int(setMsPerTick);
        pConfig->m_nTicksHeartbeat = int(setHeartbeatTicks);
        pConfig->m_nTicksElection = int(setElectionTicks);

        const Setting &setCheckQuorum = root["inventory"]["CheckQuorum"];
        const Setting &setPreVote = root["inventory"]["PreVote"];
        pConfig->m_bCheckQuorum = bool(setCheckQuorum);
        pConfig->m_bPreVote = bool(setPreVote);

        const Setting &setMaxMsgSize = root["inventory"]["MaxMsgSize"];
        const Setting &setMaxInfilght = root["inventory"]["MaxInfilght"];
        pConfig->m_nMaxMsgSize = int(setMaxMsgSize);
        pConfig->m_nMaxInfilght = int(setMaxInfilght);

        const Setting &setReadOnlyMode = root["inventory"]["ReadOnlyMode"];
        pConfig->m_optionReadOnly = ReadOnlyOption(int(setReadOnlyMode));

        const Setting &setDataPath = root["inventory"]["DataPath"];
        pConfig->m_strDataPath = setDataPath.c_str();

        const Setting &nodes = root["inventory"]["nodes"];
        int nCount = nodes.getLength();
        for (int nIndex = 0; nIndex < nCount; nIndex++)
        {
            const Setting &node = nodes[nIndex];
            CRaftInfo nodeCfg;
            nodeCfg.m_strHost = node["host"].c_str();
            nodeCfg.m_nPort = int(node["port"]);
            nodeCfg.m_nPeerPort = int(node["peerport"]);
            nodeCfg.m_nNodeId = int(node["nodeid"]);
            pConfig->m_aNodes.push_back(nodeCfg);
        }
    }
    catch (const FileIOException &excepIo)
    {
        strErrMsg = excepIo.what();
    }
    catch (const ParseException &excepParse)
    {
        strErrMsg = excepParse.what();
    }
    catch (const SettingNotFoundException &excepNoFound)
    {
        strErrMsg = excepNoFound.what();
    }
    catch (const std::exception &excep)
    {
        strErrMsg = excep.what();
    }
    if (!strErrMsg.empty())
    {
        delete pConfig;
        pConfig = NULL;
    }
    return pConfig;
}

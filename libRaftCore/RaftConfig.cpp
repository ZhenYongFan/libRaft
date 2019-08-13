#include "stdafx.h"
#include <map>
#include <string>
#include <algorithm>
#include <assert.h>
#include "RaftConfig.h"

CRaftConfig::CRaftConfig(void)
{
    m_nMsPerTick = 1000;
    m_nTicksHeartbeat = 1;
    m_nTicksElection = 10;
    m_nMaxMsgSize = 1024 * 1024;
    m_nMaxInfilght = 1024;
    m_bCheckQuorum = true;
    m_bPreVote = true;
    m_nRaftID = 0;
    m_u64Applied = 0;
    m_optionReadOnly = ReadOnlySafe;
}

CRaftConfig::~CRaftConfig()
{
}

bool CRaftConfig::ValidateNodes(std::string &strErrMsg) const
{
    strErrMsg.clear();
    if (0 == m_nRaftID)
        strErrMsg = "cannot use none as id";
    else if(m_aNodes.empty())
        strErrMsg = "must define at least one node";
    else
    {
        bool bFound = false;
        std::map<uint32_t, bool> mapIDs;
        std::map<std::string, bool> mapHost;
        for (size_t nNode = 0; nNode < m_aNodes.size() && strErrMsg.empty(); nNode++)
        {
            const CRaftInfo &info = m_aNodes[nNode];
            if (info.m_nNodeId == m_nRaftID)
                bFound = true;
            if (mapIDs.find(info.m_nNodeId) == mapIDs.end())
            {
                mapIDs[info.m_nNodeId] = true;
                
                char strPort[10];
                snprintf(strPort, sizeof(strPort), "%d", info.m_nPeerPort);
                std::string strKey = info.m_strHost + strPort;
                transform(strKey.begin(), strKey.end(), strKey.begin(), ::toupper);
                if (mapHost.find(strKey) == mapHost.end())
                    mapHost[strKey] = true;
                else
                    strErrMsg = "found same host + port in nodes";
            }
            else
                strErrMsg = "found same id in nodes";
            
        }
        if (!bFound)
            strErrMsg = "Not found self node id";
    }
    return strErrMsg.empty();
}

void CRaftConfig::GetPeers(std::vector<uint32_t> &peers) const
{
    peers.clear();
    for (size_t nNode = 0; nNode < m_aNodes.size(); nNode++)
        peers.push_back(m_aNodes[nNode].m_nNodeId);
}

const CRaftInfo & CRaftConfig::GetSelf(void)
{
    size_t nNode = 0;
    for (nNode = 0; nNode < m_aNodes.size(); nNode++)
    {
        const CRaftInfo &info = m_aNodes[nNode];
        if (m_aNodes[nNode].m_nNodeId == m_nRaftID)
            break;
    }
    assert(nNode < m_aNodes.size());
    return m_aNodes[nNode];
}

bool CRaftConfig::Validate(std::string &strErrMsg) const
{
    bool bValidate = false;
    strErrMsg.clear();  
    if (m_nMsPerTick < 100)
        strErrMsg = "ms per tick must be greater than 100";
    else if (m_nTicksHeartbeat <= 0)
        strErrMsg = "heartbeat tick must be greater than 0";
    else if (m_nTicksElection <= m_nTicksHeartbeat)
        strErrMsg = "election tick must be greater than heartbeat tick";
    else if (m_nMaxInfilght <= 0)
        strErrMsg = "max inflight messages must be greater than 0";
    else if (m_optionReadOnly != ReadOnlySafe && m_optionReadOnly != ReadOnlyLeaseBased)
            strErrMsg = "ReadOnlyMode must be 0 or 1";
    else if (m_optionReadOnly == ReadOnlyLeaseBased && !m_bCheckQuorum)
        strErrMsg = "CheckQuorum must be enabled when ReadOnlyOption is ReadOnlyLeaseBased";
    else
        bValidate = ValidateNodes(strErrMsg);
    return bValidate;
}

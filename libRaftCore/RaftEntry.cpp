#include "stdafx.h"
#include "RaftEntry.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRaftEntry::CRaftEntry(void)
{
    m_u64Index = 0;
    m_u64Term = 0;
    m_eType = eNormal;
}

CRaftEntry::CRaftEntry(const CRaftEntry & entry)
{
    Copy(entry);
}

CRaftEntry& CRaftEntry::operator =(const CRaftEntry& entry)
{
    Copy(entry);
    return *this;
}

CRaftEntry::~CRaftEntry(void)
{
}

void CRaftEntry::Copy(const CRaftEntry & entry)
{
    m_u64Term = entry.m_u64Term;
    m_u64Index = entry.m_u64Index;
    m_eType = entry.m_eType;
    m_strData = entry.m_strData;
}

CConfState::~CConfState(void)
{
    if (m_pnNodes != NULL)
    {
        delete []m_pnNodes;
        m_pnNodes = NULL;
    }
}

void CConfState::Copy(const CConfState & stateConf)
{
    if (m_pnNodes != NULL)
    {
        delete[]m_pnNodes;
        m_pnNodes = NULL;
    }
    m_nNum = stateConf.m_nNum;
    if (m_nNum > 0)
    {
        m_pnNodes = new uint32_t[m_nNum];
        memcpy(m_pnNodes, stateConf.m_pnNodes, m_nNum * sizeof(uint32_t));
    }
}

void CConfState::add_nodes(uint32_t nNodeID)
{
    uint32_t * pnNodes = new uint32_t[m_nNum +1];
    if (m_nNum > 0)
    {
        memcpy(pnNodes, m_pnNodes, m_nNum * sizeof(uint32_t));
        delete[]m_pnNodes;
    }
    pnNodes[m_nNum] = nNodeID;
    m_nNum++;
    m_pnNodes = pnNodes;
}

CSnapshot::~CSnapshot(void)
{
    if (NULL != m_pData)
    {
        delete[]m_pData;
        m_pData = NULL;
    }
}

void CSnapshot::CopyFrom(const CSnapshot& snapshot)
{
    m_metaDat.Copy(snapshot.m_metaDat);
    if (NULL != m_pData)
    {
        delete[]m_pData;
        m_pData = NULL;
    }
    m_u32DatLen = snapshot.m_u32DatLen;
    if (m_u32DatLen > 0)
    {
        m_pData = new unsigned char[m_u32DatLen];
        memcpy(m_pData, snapshot.m_pData, m_u32DatLen);
    }
}

void CSnapshot::set_data(const std::string &data)
{
    if (NULL != m_pData)
    {
        delete[]m_pData;
        m_pData = NULL;
    }
    m_u32DatLen = static_cast<uint32_t>(data.length());
    if (m_u32DatLen > 0)
    {
        m_pData = new unsigned char[m_u32DatLen];
        memcpy(m_pData, data.c_str(), m_u32DatLen);
    }
}

CMessage::CMessage(void)
{
    m_pEntry = NULL;
    m_nEntryNum = 0;
    m_u64Index = 0;
    m_u64Term = 0;
    m_u64Committed = 0;
    m_u64LogTerm = 0;
    m_u64RejectHint = 0;
    m_nFromID = 0;
    m_nToID = 0;
    m_typeMsg = MsgUnreachable;
    m_bReject = false;
}

CMessage::CMessage(const CMessage &msg)
{
    m_pEntry = NULL;
    m_nEntryNum = 0;
    Copy(msg);
}

void CMessage::Copy(const CMessage &msg)
{
    if (m_pEntry != NULL)
    {
        delete[]m_pEntry;
        m_pEntry = NULL;
        m_nEntryNum = 0;
    }
    m_nToID = msg.m_nToID;
    m_nFromID = msg.m_nFromID;
    m_u64Term = msg.m_u64Term;
    m_u64Index = msg.m_u64Index;
    m_u64LogTerm = msg.m_u64LogTerm;
    m_u64Committed = msg.m_u64Committed;
    m_u64RejectHint = msg.m_u64RejectHint;
    set_entries_from_msg(msg);
    m_typeMsg = msg.m_typeMsg;
    m_bReject = msg.m_bReject;
    m_snapshotLog.CopyFrom(msg.m_snapshotLog);
    m_strContext = msg.m_strContext;
}

CMessage& CMessage::operator =(const CMessage &msg)
{
    Copy(msg);
    return *this;
}

CMessage::~CMessage(void)
{
    if (m_pEntry != NULL)
    {
        delete[]m_pEntry;
        m_pEntry = NULL;
        m_nEntryNum = 0;
    }
}

void CMessage::set_entries(std::vector<CRaftEntry> &entries)
{
    if (m_pEntry != NULL)
    {
        delete []m_pEntry;
        m_pEntry = NULL;
    }
    m_nEntryNum = static_cast<uint16_t> (entries.size());
    if (m_nEntryNum > 0)
    {
        m_pEntry = new CRaftEntry[m_nEntryNum];
        for (uint16_t nIndex = 0 ;nIndex < m_nEntryNum ; nIndex ++)
        {
            m_pEntry[nIndex].Copy(entries[nIndex]);
        }
    }
}

void CMessage::set_entries_from_msg(const CMessage &msg)
{
    if (m_pEntry != NULL)
    {
        delete[]m_pEntry;
        m_pEntry = NULL;
    }
    m_nEntryNum = msg.m_nEntryNum;
    if (m_nEntryNum > 0)
    {
        m_pEntry = new CRaftEntry[m_nEntryNum];
        for (uint16_t nIndex = 0; nIndex < m_nEntryNum; nIndex ++)
            m_pEntry[nIndex].Copy(msg.m_pEntry[nIndex]);
    }
}

CRaftEntry *CMessage::entries(uint16_t nIndex) const
{
    CRaftEntry *pEntry = NULL;
    if (nIndex < m_nEntryNum)
        pEntry = m_pEntry + nIndex;
    return pEntry;
}

CRaftEntry *CMessage::add_entries(void)
{
    assert(0 == m_nEntryNum);
    m_nEntryNum = 1;
    m_pEntry = new CRaftEntry[1];
    return m_pEntry;
}

void CConfChange::SerializeToString(std::string &strData)
{
    char cTemp[256];
    snprintf(cTemp, 256, "%lld\t%d\t%d\t%d", m_nID,int(m_typeChange), m_nRaftID, m_u32Len);
    strData = cTemp;
}

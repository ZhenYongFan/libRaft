#include "stdafx.h"
#include "RaftSerializer.h"
#include "RaftEntry.h"

CRaftSerializer::CRaftSerializer()
{
}

CRaftSerializer::~CRaftSerializer()
{
}

size_t CRaftSerializer::ByteSize(const CRaftEntry &entry) const
{
    return sizeof(entry.m_u64Term) + sizeof(entry.m_u64Term) + sizeof(entry.m_eType) + 
           sizeof(int) + entry.m_strData.size();
}

void CRaftSerializer::SerializeEntry(const CRaftEntry &entry, std::string &strValue)
{
    size_t nEntrySize = ByteSize(entry);
    strValue.resize(nEntrySize);
    char *pstrBuffer = (char *)strValue.c_str();
    //only for x86
    memcpy(pstrBuffer,&entry.m_u64Term, sizeof(entry.m_u64Term));
    pstrBuffer += sizeof(entry.m_u64Term);

    memcpy(pstrBuffer, &entry.m_u64Index,sizeof(entry.m_u64Index));
    pstrBuffer += sizeof(entry.m_u64Index);

    memcpy(pstrBuffer,&entry.m_eType, sizeof(entry.m_eType));
    pstrBuffer += sizeof(entry.m_eType);

    int nLen = static_cast<int>(entry.m_strData.size());
    memcpy(pstrBuffer, &nLen,sizeof(int));
    pstrBuffer += sizeof(int);
    memcpy(pstrBuffer, entry.m_strData.c_str(),nLen);
}

bool CRaftSerializer::ParseEntry(CRaftEntry &entry,const std::string &strValue)
{
    bool bParse = false;
    entry.m_strData.clear();
    size_t nEntrySize = ByteSize(entry);
    if (strValue.size() >= nEntrySize)
    {
        const char *pstrBuffer = strValue.c_str();
        //only for x86
        memcpy(&entry.m_u64Term, pstrBuffer, sizeof(entry.m_u64Term));
        pstrBuffer += sizeof(entry.m_u64Term);

        memcpy(&entry.m_u64Index, pstrBuffer, sizeof(entry.m_u64Index));
        pstrBuffer += sizeof(entry.m_u64Index);

        memcpy(&entry.m_eType, pstrBuffer, sizeof(entry.m_eType));
        pstrBuffer += sizeof(entry.m_eType);

        int nLen = 0;
        memcpy(&nLen, pstrBuffer,sizeof(int));
        if (nLen > 0)
        {
            if (strValue.size() == nEntrySize + nLen)
            {
                entry.m_strData.resize(nLen);
                pstrBuffer += sizeof(int);
                char * pstrData = (char *)entry.m_strData.c_str();
                memcpy(pstrData, pstrBuffer, nLen);
                bParse = true;
            }
        }
        else
        {
            bParse = true;
        }
    }
    return bParse;
}

void CRaftSerializer::SerializeMessage(const CMessage &msg, std::string &strValue)
{
    assert(false);
}

bool CRaftSerializer::ParseMessage(CMessage &msg, const std::string &strValue)
{
    assert(false);
    return false;
}
#include "stdafx.h"
#include "SequenceID.h"

CSequenceID::CSequenceID(uint32_t u32BaseID)
{
    assert(u32BaseID < uint32_t(0xFFFFFFFF) - (64 * 1024));
    m_u32BaseID = u32BaseID;
    m_pu64Mask = new uint64_t[1024];
    memset(m_pu64Mask, 0, sizeof(uint64_t) * 1024);
    m_nCurIdx = 0;
}

CSequenceID::~CSequenceID()
{
    delete []m_pu64Mask;
}

int CSequenceID::GetZeroBit(uint64_t u64Value)
{
    int nBits = 0;
    unsigned char *pbyteTest = (unsigned char *)(&u64Value);
    for (int nIndex = 0; nIndex < 8; nIndex++)
    {
        if (pbyteTest[nIndex] != 0xFF)
        {
            unsigned char byteValue = pbyteTest[nIndex];
            unsigned char byteMask = 1;
            for (int nBit = 0; nBit < 8; nBit++)
            {
                if ((byteValue & byteMask) == 0)
                {
                    nBits = (nIndex << 3) + nBit;
                    break;
                }
                else
                    byteMask <<= 1;
            }
            break;
        }
    }
    return nBits;
}

bool CSequenceID::AllocSeqID(uint32_t &nSeqID)
{
    bool bCreate = false;
    std::lock_guard<std::mutex> guard(m_mutexSeq);
    if (m_listFreeSeq.empty())
    {
        nSeqID = 0;
        for (int nIndex = 0; nIndex < 1024; nIndex++)
        {
            if (m_pu64Mask[m_nCurIdx] != 0xFFFFFFFFFFFFFFFF)
            {
                int nBits = GetZeroBit(m_pu64Mask[m_nCurIdx]);
                nSeqID = (m_nCurIdx << 6) + nBits;
                nSeqID += m_u32BaseID;
                m_pu64Mask[m_nCurIdx] |= uint64_t(1) << nBits;
                bCreate = true;
                break;
            }
            else
            {
                m_nCurIdx++;
                if (1024 == m_nCurIdx)
                    m_nCurIdx = 0;
            }
        }
    }
    else
    {
        nSeqID = m_listFreeSeq.front();
        m_listFreeSeq.pop_front();
        bCreate = true;
    }
    return bCreate;
}

void CSequenceID::FreeSeqID(uint32_t nSeqID)
{
    std::lock_guard<std::mutex> guard(m_mutexSeq);
    m_listFreeSeq.push_back(nSeqID);
}

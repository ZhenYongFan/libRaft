#include "stdafx.h"
// #include "raft.pb.h"
// using namespace raftpb;

#include "RocksDbStorage.h"
#include "RaftSerializer.h"
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
using namespace rocksdb;

CRocksDbStorage::CRocksDbStorage(CLogger *pLogger,CRaftSerializer *pRaftSerializer)
    :CRaftStorage(pRaftSerializer),m_pLogger(pLogger)
{
    assert(NULL != pRaftSerializer);
    m_pSnapShot = new CSnapshot();
    m_u64FirstIndex = uint64_t(-1);
    m_u64LastIndex = uint64_t(-1);
    m_u64Committed = uint64_t(-1);
    m_u64Applied = uint64_t(-1);
}

CRocksDbStorage::~CRocksDbStorage()
{
    if (m_pSnapShot != NULL)
    {
        delete m_pSnapShot;
        m_pSnapShot = NULL;
    }
}

int CRocksDbStorage::Init(std::string &strDbPath)
{
    int nInit = 0;
    if (NULL == m_pStocksDB)
    {
        Options optionsDB;
        optionsDB.create_if_missing = true;
        Status statusDB = DB::Open(optionsDB, strDbPath, &m_pStocksDB);
        if (statusDB.ok())
        {
            if (IsEmpty())
            {
                m_u64FirstIndex = 0;
                m_u64LastIndex = 0;
                EntryVec entries;
                entries.push_back(CRaftEntry());
                //也可以优化为一次batch操作
                int nAppend = Append(entries);
                if (0 == nInit)
                {
                    nInit = SetCommitted(0);
                    if (0 == nInit)
                        nInit = SetApplied(0);
                }
            }
            else
            {
                uint64_t u64Index;
                if ((0 == FirstIndex(u64Index)) 
                    && (0 == LastIndex(u64Index))
                    && (0 == GetCommitted(u64Index))
                    && (0 == GetApplied(u64Index)))
                {
                    nInit = 0;
                }
            }
        }
        else
            nInit = int(statusDB.code());
    }
    else
        nInit = -1;
    return nInit;
}

bool CRocksDbStorage::IsEmpty(void)
{
    bool bEmpty = false;
    Iterator* pIter = m_pStocksDB->NewIterator(ReadOptions());
    if (NULL != pIter)
    {
        pIter->SeekToFirst();
        bEmpty = pIter->Valid();
        delete pIter;
    }
    return bEmpty;
}

void CRocksDbStorage::Uninit(void)
{
    if (NULL != m_pStocksDB)
    {
        Status statusDB = m_pStocksDB->Close();
        delete m_pStocksDB;
        m_pStocksDB = NULL;
    }
}

int CRocksDbStorage::FirstIndex(uint64_t &u64Index)
{
    int nGet = 1;
    if (uint64_t(-1) == m_u64FirstIndex)
    {
        rocksdb::Slice strKey, strData;
        Iterator* pIter = m_pStocksDB->NewIterator(ReadOptions());
        if (NULL != pIter)
        {
            pIter->SeekToFirst();
            if (pIter->Valid())
            {
                strKey = pIter->key().data();
                strData = pIter->value();
                std::string strValue(strData.data(), strData.size());
                CRaftEntry entry;
                m_pRaftSerializer->ParseFromString(entry,strValue);
                u64Index = entry.index();
                m_u64FirstIndex = u64Index;
                nGet = 0;
            }
            delete pIter;
        }
    }
    else
    {
        u64Index = m_u64FirstIndex;
        nGet = 0;
    }
    return nGet;
}

int CRocksDbStorage::LastIndex(uint64_t &u64Index)
{
    int nGet = 1;
    if (uint64_t(-1) == m_u64LastIndex)
    {
        rocksdb::Slice strKey, strData;
        Iterator* pIter = m_pStocksDB->NewIterator(ReadOptions());
        if (NULL != pIter)
        {
            pIter->SeekToLast();
            if (pIter->Valid())
            {
                strKey = pIter->key().data();
                strData = pIter->value();
                std::string strValue(strData.data(), strData.size());
                CRaftEntry entry;
                m_pRaftSerializer->ParseFromString(entry, strValue);
                u64Index = entry.index();
                m_u64LastIndex = u64Index;
                nGet = 0;
            }
            delete pIter;
        }
    }
    else
    {
        u64Index = m_u64LastIndex;
        nGet = 0;
    }
    return nGet;
}

int CRocksDbStorage::SetCommitted(uint64_t u64Committed)
{
    char cTemp[64];
    snprintf(cTemp, 64, "%llu", u64Committed);
    std::string strKey("Committed");
    std::string strValue = cTemp;
    Status statusDB = m_pStocksDB->Put(WriteOptions(), strKey, strValue);
    if (statusDB.ok())
        m_u64Committed = u64Committed;
    return int(statusDB.code());
}

int CRocksDbStorage::GetCommitted(uint64_t &u64Committed)
{
    int nGet = 1;
    if (uint64_t(-1) == m_u64Committed)
    {
        std::string strKey("Committed");
        std::string strValue;
        Status statusDB = m_pStocksDB->Get(ReadOptions(), strKey, &strValue);
        if (statusDB.ok())
        {
            m_u64Committed = atoll(strValue.c_str());
            u64Committed = m_u64Committed;
        }
        nGet = int(statusDB.code());
    }
    else
    {
        u64Committed = m_u64Committed;
        nGet = 0;
    }
    return nGet;
}

int CRocksDbStorage::SetApplied(uint64_t u64Applied)
{
    char cTemp[64];
    snprintf(cTemp, 64, "%llu", u64Applied);
    std::string strKey("Applied");
    std::string strValue = cTemp;
    Status statusDB = m_pStocksDB->Put(WriteOptions(), strKey, strValue);
    if (statusDB.ok())
        m_u64Applied = u64Applied;
    return int(statusDB.code());
}

int CRocksDbStorage::GetApplied(uint64_t &u64Applied)
{
    int nGet = 1;
    if (uint64_t(-1) == m_u64Applied)
    {
        std::string strKey("Applied");
        std::string strValue;
        Status statusDB = m_pStocksDB->Get(ReadOptions(), strKey, &strValue);
        if (statusDB.ok())
        {
            m_u64Applied = atoll(strValue.c_str());
            u64Applied = m_u64Applied;
        }
        nGet = int(statusDB.code());
    }
    else
    {
        u64Applied = m_u64Applied;
        nGet = 0;
    }
    return nGet;
}

int CRocksDbStorage::Term(uint64_t u64Index, uint64_t &u64Term)
{
    char cTemp[64];
    snprintf(cTemp, 64, "%llu", u64Index);
    std::string strKey = cTemp;
    std::string strValue;
    Status statusDB = m_pStocksDB->Get(ReadOptions(), strKey, &strValue);
    if (statusDB.code() == Status::kOk)
    {
        CRaftEntry entry;
        m_pRaftSerializer->ParseFromString(entry, strValue);
        u64Term = entry.term();
    }
    return int(statusDB.code());
}

int CRocksDbStorage::Append(const EntryVec& entries)
{
    char cTemp[64];  
    std::string strKey, strValue;
    WriteBatch batchWrite;
    uint64_t u64LastIndex = 0;
    for (auto entry :entries)
    {
        u64LastIndex = entry.index();
        snprintf(cTemp, 64, "%llu",u64LastIndex );
        strKey = cTemp;
        m_pRaftSerializer->SerializeAsString(entry, strValue);
        batchWrite.Put(strKey, strValue);
    }
    Status statusDB = m_pStocksDB->Write(WriteOptions(), &batchWrite);
    if(statusDB.ok())
        m_u64LastIndex = u64LastIndex;
    return int(statusDB.code());
}

int CRocksDbStorage::Entries(uint64_t u64Low, uint64_t u64High, uint64_t u64MaxSize, vector<CRaftEntry> &entries)
{
    char cTemp[64];
    std::string strKey,strValue;
    entries.clear();
    uint64_t u64DatSize = 0;
    Status statusDB;
    for (uint64_t u64Index = u64Low ; u64Index < u64High ; u64Index ++)
    {
        snprintf(cTemp, 64, "%llu", u64Index);
        strKey = cTemp;
        statusDB = m_pStocksDB->Get(ReadOptions(), strKey, &strValue);
        if (statusDB.code() == Status::kOk)
        {
            CRaftEntry entry;
            m_pRaftSerializer->ParseFromString(entry, strValue);
            if (u64DatSize + m_pRaftSerializer->ByteSize(entry) <= u64MaxSize)
            {
                entries.push_back(entry);
                u64DatSize += m_pRaftSerializer->ByteSize(entry);
            }
            else
                break;
        }
    }
    return int(statusDB.code());
}

int CRocksDbStorage::InitialState(CHardState &stateHard, CConfState &stateConfig)
{
    stateHard = m_stateHard;
    stateConfig = m_pSnapShot->metadata().conf_state();
    return OK;
}

int CRocksDbStorage::SetHardState(const CHardState &stateHard)
{
    m_stateHard = stateHard;
    return OK;
}

int CRocksDbStorage::GetSnapshot(CSnapshot **pSnapShot)
{
    *pSnapShot = m_pSnapShot;
    return OK;
}

int CRocksDbStorage::CreateSnapshot(uint64_t u64Index, CConfState *pConfState, const string& datSnapshot, CSnapshot *pSnapshot)
{
#if 0
    if (u64Index <= m_pSnapShot->metadata().index())
        return ErrSnapOutOfDate;

    uint64_t offset = entries_[0].index();
    if (u64Index > LastIndex())
    {
        m_pLogger->Fatalf(__FILE__, __LINE__, "snapshot %d is out of bound lastindex(%llu)", u64Index, LastIndex());
    }

    m_pSnapShot->mutable_metadata()->set_index(u64Index);
    m_pSnapShot->mutable_metadata()->set_term(entries_[u64Index - offset].term());
    if (pConfState != NULL)
        *(m_pSnapShot->mutable_metadata()->mutable_conf_state()) = *pConfState;
    m_pSnapShot->set_data(datSnapshot);
    if (pSnapshot != NULL)
        *pSnapshot = *m_pSnapShot;
#endif
    return OK;
}

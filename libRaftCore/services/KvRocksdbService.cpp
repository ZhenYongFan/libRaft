#include "stdafx.h"
#include "KvRocksdbService.h"

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
using namespace rocksdb;

CKvRocksdbService::CKvRocksdbService(void)
{
    m_pStocksDB = NULL;
}

CKvRocksdbService::~CKvRocksdbService(void)
{

}

int CKvRocksdbService::Init(std::string &strDbPath)
{
    int nInit = 0;
    if (NULL == m_pStocksDB)
    {
        Options optionsDB;
        optionsDB.create_if_missing = true;
        Status statusDB = DB::Open(optionsDB, strDbPath, &m_pStocksDB);
        nInit = int(statusDB.code());
    }
    return nInit;
}

void CKvRocksdbService::Uninit(void)
{
    if (NULL != m_pStocksDB)
    {
        Status statusDB = m_pStocksDB->Close();
        delete m_pStocksDB;
        m_pStocksDB = NULL;
    }
}

//简单调用RocksDB的Put接口
int CKvRocksdbService::Put(const std::string &strKey, const std::string &strValue)
{
    Status statusDB = m_pStocksDB->Put(WriteOptions(), strKey, strValue);
    return int(statusDB.code());
}

int CKvRocksdbService::Get(const std::string &strKey, std::string &strValue)
{
    Status statusDB = m_pStocksDB->Get(ReadOptions(), strKey, &strValue);
    return int(statusDB.code());
}

int CKvRocksdbService::Delete(const std::string &strKey)
{
    Status statusDB = m_pStocksDB->Delete(WriteOptions(), strKey);
    return int(statusDB.code());
}

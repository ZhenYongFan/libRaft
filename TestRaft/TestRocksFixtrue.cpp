#include "stdafx.h"
#include "TestRocksFixtrue.h"
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
using namespace rocksdb;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( CTestRocksFixtrue );

// CTestSampleFixtrue

CTestRocksFixtrue::CTestRocksFixtrue()
{
}

CTestRocksFixtrue::~CTestRocksFixtrue()
{
}

void CTestRocksFixtrue::setUp(void)
{
}

void CTestRocksFixtrue::tearDown(void)
{

}

int CTestRocksFixtrue::TestDB(std::string &strDbPath)
{
    DB * pStocksDB;
    Options optionsDB;
    optionsDB.create_if_missing = true;

    //open DB
    Status statusDB = DB::Open(optionsDB, strDbPath, &pStocksDB);
    CPPUNIT_ASSERT(statusDB.ok());

    //Put key-value
    statusDB = pStocksDB->Put(WriteOptions(), "key1", "value");
    CPPUNIT_ASSERT(statusDB.ok());
    int nTimes = 10;
    char pstrKey[128];
    char pstrValue[128];
    for (int nIndex = 0; nIndex < 1000 * nTimes; nIndex++)
    {
        snprintf(pstrKey, sizeof(pstrKey), "TestMyKeyRaftKVStoreiscomingKey%d", nIndex);
        snprintf(pstrValue, sizeof(pstrValue), "TestMyKeyRaftKVStoreiscomingValue%d", nIndex);
        statusDB = pStocksDB->Put(WriteOptions(), pstrKey, pstrValue);
        CPPUNIT_ASSERT(statusDB.ok());
    }
    std::string value;
    //get value
    statusDB = pStocksDB->Get(ReadOptions(), "key1", &value);
    CPPUNIT_ASSERT(statusDB.ok());
    CPPUNIT_ASSERT(value == "value");

    //将一组更新原子化
    {
        WriteBatch batch;
        batch.Delete("key1");
        batch.Put("key2", value);
        WriteOptions optWrite;
        statusDB = pStocksDB->Write(optWrite, &batch);
    }

    for (int nIndex = 0; nIndex < 1000 * nTimes; nIndex++)
    {
        snprintf(pstrKey, sizeof(pstrKey), "00TestMyKeyRaftKVStoreiscomingKey%d", nIndex);
        snprintf(pstrValue, sizeof(pstrValue), "00TestMyKeyRaftKVStoreiscomingValue%d", nIndex);
        statusDB = pStocksDB->Put(WriteOptions(), pstrKey, pstrValue);
        CPPUNIT_ASSERT(statusDB.ok());
    }
    statusDB = pStocksDB->Get(ReadOptions(), "key1", &value);
    CPPUNIT_ASSERT(statusDB.IsNotFound());

    pStocksDB->Get(ReadOptions(), "key2", &value);
    CPPUNIT_ASSERT(value == "value");

    {
        PinnableSlice pinnable_val;
        pStocksDB->Get(ReadOptions(), pStocksDB->DefaultColumnFamily(), "key2", &pinnable_val);
        CPPUNIT_ASSERT(pinnable_val == "value");
    }

    {
        std::string string_val;
        //如果无法锁定值，则将该值复制到其内部缓冲区
        //内部缓冲区可以在构造时设置
        PinnableSlice pinnable_val(&string_val);
        pStocksDB->Get(ReadOptions(), pStocksDB->DefaultColumnFamily(), "key2", &pinnable_val);
        CPPUNIT_ASSERT(pinnable_val == "value");
        //如果值没被固定，则一定在内部缓冲区里
        CPPUNIT_ASSERT(pinnable_val.IsPinned() || string_val == "value");
    }

    PinnableSlice pinnable_val;
    pStocksDB->Get(ReadOptions(), pStocksDB->DefaultColumnFamily(), "key1", &pinnable_val);
    CPPUNIT_ASSERT(statusDB.IsNotFound());
    //每次使用之后和每次重新使用之前重置PinnableSlice
    pinnable_val.Reset();
    pStocksDB->Get(ReadOptions(), pStocksDB->DefaultColumnFamily(), "key2", &pinnable_val);
    CPPUNIT_ASSERT(pinnable_val == "value");
    pinnable_val.Reset();
    //在这之后，pinnable_val指向的Slice无效。

    delete pStocksDB;
    return 0;
}

void CTestRocksFixtrue::TestBase(void)
{
    std::string strDbPath;
#ifdef _WIN64
    strDbPath = "c:\\temp\\testdb\\";
#else
    strDbPath = "/temp/testdb/";
#endif
        TestDB(strDbPath);
}

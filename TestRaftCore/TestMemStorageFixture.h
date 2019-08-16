#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestMemStorageFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestMemStorageFixture);

    CPPUNIT_TEST(TestStorageTerm);
    CPPUNIT_TEST(TestStorageEntries);
    CPPUNIT_TEST(TestStorageLastIndex);
    CPPUNIT_TEST(TestStorageFirstIndex);
    CPPUNIT_TEST(TestStorageCompact);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestMemStorageFixture();

    ~CTestMemStorageFixture();

    void setUp(void);

    void tearDown(void);

    void TestStorageTerm(void);
    void TestStorageEntries(void);
    void TestStorageLastIndex(void);
    void TestStorageFirstIndex(void);
    void TestStorageCompact(void);
};


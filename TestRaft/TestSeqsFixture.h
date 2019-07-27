#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestSeqsFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestSeqsFixture);

    CPPUNIT_TEST(TestExample);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestSeqsFixture();
    
    ~CTestSeqsFixture();

    void setUp(void);

    void tearDown(void);

    void TestExample(void);
};


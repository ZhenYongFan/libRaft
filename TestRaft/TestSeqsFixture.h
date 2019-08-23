#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestSeqsFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestSeqsFixture);

    CPPUNIT_TEST(TestSeqs);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestSeqsFixture();
    
    ~CTestSeqsFixture();

    void setUp(void);

    void tearDown(void);

    void TestSeqs(void);
};


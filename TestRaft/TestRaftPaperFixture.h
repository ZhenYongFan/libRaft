#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestRaftPaperFixture :public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestRaftPaperFixture);

    CPPUNIT_TEST(TestFollowerUpdateTermFromMessage);
    CPPUNIT_TEST(TestCandidateUpdateTermFromMessage);
    CPPUNIT_TEST(TestLeaderUpdateTermFromMessage);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestRaftPaperFixture();

    ~CTestRaftPaperFixture();
    
    void setUp(void);

    void tearDown(void);
    void TestFollowerUpdateTermFromMessage(void);
    void TestCandidateUpdateTermFromMessage(void);
    void TestLeaderUpdateTermFromMessage(void);
};


#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestFlowControllerFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestFlowControllerFixture);
    CPPUNIT_TEST(TestBase);
    CPPUNIT_TEST(TestMsgAppFlowControlFull);
    CPPUNIT_TEST(TestMsgAppFlowControlMoveForward);
    CPPUNIT_TEST(TestMsgAppFlowControlRecvHeartbeat);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestFlowControllerFixture();
    ~CTestFlowControllerFixture();

    void setUp(void);

    void tearDown(void);
    void TestBase(void);
    void TestMsgAppFlowControlFull(void);
    void TestMsgAppFlowControlMoveForward(void);
    void TestMsgAppFlowControlRecvHeartbeat(void);
};


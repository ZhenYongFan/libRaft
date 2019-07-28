#pragma once
#include <cppunit/extensions/HelperMacros.h>
class CTestProgressFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestProgressFixture);

    CPPUNIT_TEST(TestInflightsAdd);
    CPPUNIT_TEST(TestInflightFreeTo);
    CPPUNIT_TEST(TestInflightFreeFirstOne);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestProgressFixture();
    ~CTestProgressFixture();
    void setUp(void);
    void tearDown(void);
    void TestInflightsAdd(void);
    void TestInflightFreeTo(void);
    void TestInflightFreeFirstOne(void);
};


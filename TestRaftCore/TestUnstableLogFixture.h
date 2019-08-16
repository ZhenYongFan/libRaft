#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestUnstableLogFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestUnstableLogFixture);

    CPPUNIT_TEST(TestExample);
    CPPUNIT_TEST(TestUnstableMaybeFirstIndex);
    CPPUNIT_TEST(TestMaybeLastIndex);
    CPPUNIT_TEST(TestUnstableMaybeTerm);
    CPPUNIT_TEST(TestUnstableRestore);
    CPPUNIT_TEST(TestUnstableStableTo);
    CPPUNIT_TEST(TestUnstableTruncateAndAppend);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestUnstableLogFixture();
    ~CTestUnstableLogFixture();

    void setUp(void);

    void tearDown(void);

    void TestExample(void);
    void TestUnstableMaybeFirstIndex(void);
    void TestMaybeLastIndex(void);
    void TestUnstableMaybeTerm(void);
    void TestUnstableRestore(void);
    void TestUnstableStableTo(void);
    void TestUnstableTruncateAndAppend(void);
};


#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestRaftLogFixture : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestRaftLogFixture);

    CPPUNIT_TEST(TestFindConflict);
    CPPUNIT_TEST(TestIsUpToDate);
    CPPUNIT_TEST(TestAppend);
    CPPUNIT_TEST(TestLogMaybeAppend);
    CPPUNIT_TEST(TestCompactionSideEffects);
    CPPUNIT_TEST(TestHasNextEnts);
    CPPUNIT_TEST(TestNextEnts);
    CPPUNIT_TEST(TestUnstableEnts);
    CPPUNIT_TEST(TestCommitTo);
    CPPUNIT_TEST(TestStableTo);
    CPPUNIT_TEST(TestStableToWithSnap);
    CPPUNIT_TEST(TestCompaction);
    CPPUNIT_TEST(TestLogRestore);
    CPPUNIT_TEST(TestIsOutOfBounds);
    CPPUNIT_TEST(TestTerm);
    CPPUNIT_TEST(TestTermWithUnstableSnapshot);
    CPPUNIT_TEST(TestSlice);
    CPPUNIT_TEST_SUITE_END();
public:
    CTestRaftLogFixture();

    ~CTestRaftLogFixture();

    void setUp(void);

    void tearDown(void);

    void TestFindConflict(void);
    void TestIsUpToDate(void);
    void TestAppend(void);
    void TestLogMaybeAppend(void);
    void TestCompactionSideEffects(void);
    void TestHasNextEnts(void);
    void TestNextEnts(void);
    void TestUnstableEnts(void);
    void TestCommitTo(void);
    void TestStableTo(void);
    void TestStableToWithSnap(void);
    void TestCompaction(void);
    void TestLogRestore(void);
    void TestIsOutOfBounds(void);
    void TestTerm(void);
    void TestTermWithUnstableSnapshot(void);
    void TestSlice(void);
};


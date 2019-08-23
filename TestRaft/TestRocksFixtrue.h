#pragma once
#include <cppunit/extensions/HelperMacros.h>

// CTestRocksFixtrue 文档

class CTestRocksFixtrue :public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( CTestRocksFixtrue );

	CPPUNIT_TEST( TestBase);

	CPPUNIT_TEST_SUITE_END();

public:
	CTestRocksFixtrue();

	virtual ~CTestRocksFixtrue();

	void setUp(void);

	void tearDown(void);

	void TestBase(void);
protected:
    int TestDB(std::string &strDbPath);
};

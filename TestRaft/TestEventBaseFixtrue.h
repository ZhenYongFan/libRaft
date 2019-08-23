#pragma once
#include <cppunit/extensions/HelperMacros.h>

// CTestEventBaseFixtrue 文档

class CTestEventBaseFixtrue :public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( CTestEventBaseFixtrue );

	CPPUNIT_TEST( TestBase);

	CPPUNIT_TEST_SUITE_END();

public:
	CTestEventBaseFixtrue();

	virtual ~CTestEventBaseFixtrue();

	void setUp(void);

	void tearDown(void);

	void TestBase(void);
};

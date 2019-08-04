// [!output IMPL_FILE] : 实现文件
//

#include "stdafx.h"
#include "TestEventBaseFixtrue.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "network/EventBase.h"
CPPUNIT_TEST_SUITE_REGISTRATION( CTestEventBaseFixtrue );

// CTestSampleFixtrue

CTestEventBaseFixtrue::CTestEventBaseFixtrue()
{
}

CTestEventBaseFixtrue::~CTestEventBaseFixtrue()
{
}

void CTestEventBaseFixtrue::setUp(void)
{
}

void CTestEventBaseFixtrue::tearDown(void)
{

}

void CTestEventBaseFixtrue::TestExample(void)
{
#if 0
    CEventBase base;
    CPPUNIT_ASSERT(0 == base.GetState());
    bool bInit = base.Init();
    CPPUNIT_ASSERT(bInit);
    CPPUNIT_ASSERT(1 == base.GetState());
    bInit = base.Init();
    CPPUNIT_ASSERT(!bInit);
    CPPUNIT_ASSERT(1 == base.GetState());
    base.Uninit();
    CPPUNIT_ASSERT(0 == base.GetState());
    //重复调用Init
    bInit = base.Init();
    CPPUNIT_ASSERT(bInit);
    CPPUNIT_ASSERT(1 == base.GetState());
    int nStart = base.Start();
    CPPUNIT_ASSERT(0 == nStart);
    CPPUNIT_ASSERT(2 == base.GetState());
    //重复调用Start
    nStart = base.Start();
    CPPUNIT_ASSERT(1 == nStart);
    CPPUNIT_ASSERT(2 == base.GetState());
    base.NotifyStop();
    CPPUNIT_ASSERT(3 == base.GetState());
    base.Stop();
    CPPUNIT_ASSERT(4 == base.GetState());
    base.Uninit();
    CPPUNIT_ASSERT(0 == base.GetState());
#endif
}

#include "stdafx.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TestSeqsFixture.h"
#include <list>
#include <mutex>
#include "network/SequenceID.h"

CPPUNIT_TEST_SUITE_REGISTRATION(CTestSeqsFixture);

CTestSeqsFixture::CTestSeqsFixture()
{
}


CTestSeqsFixture::~CTestSeqsFixture()
{
}

void CTestSeqsFixture::setUp(void)
{
}

void CTestSeqsFixture::tearDown(void)
{

}

void CTestSeqsFixture::TestExample(void)
{
    CSequenceID seqIDs(1024);
    uint32_t nSeqID;
    for (int nIndex = 0 ; nIndex < 64 * 1024 ; nIndex ++)
    {
        bool bCreate = seqIDs.AllocSeqID(nSeqID);
        CPPUNIT_ASSERT(bCreate);
        CPPUNIT_ASSERT(nSeqID == nIndex + 1024);
    }
    bool bCreate = seqIDs.AllocSeqID(nSeqID);
    CPPUNIT_ASSERT(!bCreate);
    seqIDs.FreeSeqID(11111);
    seqIDs.FreeSeqID(22222);
    bCreate = seqIDs.AllocSeqID(nSeqID);
    CPPUNIT_ASSERT(bCreate);
    CPPUNIT_ASSERT(nSeqID == 11111);
    bCreate = seqIDs.AllocSeqID(nSeqID);
    CPPUNIT_ASSERT(bCreate);
    CPPUNIT_ASSERT(nSeqID == 22222);
    bCreate = seqIDs.AllocSeqID(nSeqID);
    CPPUNIT_ASSERT(!bCreate);
}

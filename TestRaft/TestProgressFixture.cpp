#include "stdafx.h"
#include "TestProgressFixture.h"
#include "Progress.h"
#include "NullLogger.h"
#include "RaftUtil.h"
extern CNullLogger kDefaultLogger;

CPPUNIT_TEST_SUITE_REGISTRATION(CTestProgressFixture);

CTestProgressFixture::CTestProgressFixture()
{
}

CTestProgressFixture::~CTestProgressFixture()
{
}

void CTestProgressFixture::setUp(void)
{
}

void CTestProgressFixture::tearDown(void)
{

}

bool deepEqualInflights(const inflights& in1, const inflights& in2)
{
    CPPUNIT_ASSERT_EQUAL(in1.start_, in2.start_);
    CPPUNIT_ASSERT_EQUAL(in1.count_, in2.count_);
    CPPUNIT_ASSERT_EQUAL(in1.size_, in2.size_);
    CPPUNIT_ASSERT_EQUAL(in1.buffer_.size(), in2.buffer_.size());
    int i = 0;
    for (i = 0; i < in1.buffer_.size(); ++i)
    {
        CPPUNIT_ASSERT_EQUAL(in1.buffer_[i], in2.buffer_[i]);
    }

    return true;
}

void CTestProgressFixture::TestInflightsAdd(void)
{
    inflights ins(10, &kDefaultLogger);
    int i;

    for (i = 0; i < 5; ++i)
    {
        ins.add(i);
    }

    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 0;
        wantIns.count_ = 5;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 1;
        wantIns.buffer_[2] = 2;
        wantIns.buffer_[3] = 3;
        wantIns.buffer_[4] = 4;
        wantIns.buffer_[5] = 0;
        wantIns.buffer_[6] = 0;
        wantIns.buffer_[7] = 0;
        wantIns.buffer_[8] = 0;
        wantIns.buffer_[9] = 0;
        // ↓------------
        // 0, 1, 2, 3, 4, 0, 0, 0, 0, 0
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }

    for (i = 5; i < 10; ++i)
    {
        ins.add(i);
    }

    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 0;
        wantIns.count_ = 10;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 1;
        wantIns.buffer_[2] = 2;
        wantIns.buffer_[3] = 3;
        wantIns.buffer_[4] = 4;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        // ↓--------------------------
        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }

    // rotating case
    inflights ins2(10, &kDefaultLogger);
    ins2.start_ = 5;

    for (i = 0; i < 5; ++i)
    {
        ins2.add(i);
    }
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 5;
        wantIns.count_ = 5;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 0;
        wantIns.buffer_[2] = 0;
        wantIns.buffer_[3] = 0;
        wantIns.buffer_[4] = 0;
        wantIns.buffer_[5] = 0;
        wantIns.buffer_[6] = 1;
        wantIns.buffer_[7] = 2;
        wantIns.buffer_[8] = 3;
        wantIns.buffer_[9] = 4;
        //                ↓------------
        // 0, 0, 0, 0, 0, 0, 1, 2, 3, 4
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins2, wantIns));
    }
    for (i = 5; i < 10; ++i)
    {
        ins2.add(i);
    }
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 5;
        wantIns.count_ = 10;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 5;
        wantIns.buffer_[1] = 6;
        wantIns.buffer_[2] = 7;
        wantIns.buffer_[3] = 8;
        wantIns.buffer_[4] = 9;
        wantIns.buffer_[5] = 0;
        wantIns.buffer_[6] = 1;
        wantIns.buffer_[7] = 2;
        wantIns.buffer_[8] = 3;
        wantIns.buffer_[9] = 4;
        // ---------------↓------------
        // 5, 6, 7, 8, 9, 0, 1, 2, 3, 4
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins2, wantIns));
    }
}

void CTestProgressFixture::TestInflightFreeTo(void)
{
    inflights ins(10, &kDefaultLogger);
    int i;

    for (i = 0; i < 10; ++i)
    {
        ins.add(i);
    }

    ins.freeTo(4);
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 5;
        wantIns.count_ = 5;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 1;
        wantIns.buffer_[2] = 2;
        wantIns.buffer_[3] = 3;
        wantIns.buffer_[4] = 4;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        //                ↓------------
        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }

    ins.freeTo(8);
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 9;
        wantIns.count_ = 1;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 1;
        wantIns.buffer_[2] = 2;
        wantIns.buffer_[3] = 3;
        wantIns.buffer_[4] = 4;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        //                            ↓
        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }

    // rotating case
    for (i = 10; i < 15; ++i)
    {
        ins.add(i);
    }
    ins.freeTo(12);
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 3;
        wantIns.count_ = 2;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 10;
        wantIns.buffer_[1] = 11;
        wantIns.buffer_[2] = 12;
        wantIns.buffer_[3] = 13;
        wantIns.buffer_[4] = 14;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        //             ↓----
        // 10, 11, 12, 13, 14, 5, 6, 7, 8, 9 
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }

    ins.freeTo(14);
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 0;
        wantIns.count_ = 0;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 10;
        wantIns.buffer_[1] = 11;
        wantIns.buffer_[2] = 12;
        wantIns.buffer_[3] = 13;
        wantIns.buffer_[4] = 14;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        // ↓
        // 10, 11, 12, 13, 14, 5, 6, 7, 8, 9 
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }
}

void CTestProgressFixture::TestInflightFreeFirstOne(void)
{
    inflights ins(10, &kDefaultLogger);
    int i;

    for (i = 0; i < 10; ++i)
    {
        ins.add(i);
    }

    ins.freeFirstOne();
    {
        inflights wantIns(10, &kDefaultLogger);
        wantIns.start_ = 1;
        wantIns.count_ = 9;
        wantIns.size_ = 10;
        wantIns.buffer_[0] = 0;
        wantIns.buffer_[1] = 1;
        wantIns.buffer_[2] = 2;
        wantIns.buffer_[3] = 3;
        wantIns.buffer_[4] = 4;
        wantIns.buffer_[5] = 5;
        wantIns.buffer_[6] = 6;
        wantIns.buffer_[7] = 7;
        wantIns.buffer_[8] = 8;
        wantIns.buffer_[9] = 9;
        //    ↓-----------------------
        // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 
        CPPUNIT_ASSERT_EQUAL(true, deepEqualInflights(ins, wantIns));
    }
}

#include "stdafx.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TestMemStorageFixture.h"
#include "RaftMemStorage.h"
#include "NullLogger.h"
#include "RaftUtil.h"
#include "RaftSerializer.h"
extern CNullLogger kDefaultLogger;

CRaftSerializer serializer;

CPPUNIT_TEST_SUITE_REGISTRATION(CTestMemStorageFixture);

CTestMemStorageFixture::CTestMemStorageFixture()
{
}

CTestMemStorageFixture::~CTestMemStorageFixture()
{
}

void CTestMemStorageFixture::setUp(void)
{
}

void CTestMemStorageFixture::tearDown(void)
{

}

void CTestMemStorageFixture::TestStorageTerm(void)
{
    EntryVec entries;

    {
        CRaftEntry entry;

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(5);
        entries.push_back(entry);
    }

    struct tmp
    {
        uint64_t i;
        int werr;
        uint64_t wterm;

        tmp(uint64_t i, int err, uint64_t term)
            : i(i), werr(err), wterm(term)
        {
        }
    };
    vector<tmp> tests;
    tests.push_back(tmp(2, ErrCompacted, 0));
    tests.push_back(tmp(3, OK, 3));
    tests.push_back(tmp(4, OK, 4));
    tests.push_back(tmp(5, OK, 5));
    tests.push_back(tmp(6, ErrUnavailable, 0));
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        s.entries_ = entries;
        uint64_t term;
        int err = s.Term(test.i, term);
        CPPUNIT_ASSERT_EQUAL(err, test.werr);
        CPPUNIT_ASSERT_EQUAL(term, test.wterm);
    }
}

void CTestMemStorageFixture::TestStorageEntries(void)
{
    EntryVec entries;

    {
        CRaftEntry entry;

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(5);
        entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(6);
        entries.push_back(entry);
    }

    struct tmp
    {
        uint64_t lo, hi, maxsize;
        int werr;
        EntryVec entries;

        tmp(uint64_t lo, uint64_t hi, uint64_t maxsize, int err)
            : lo(lo), hi(hi), maxsize(maxsize), werr(err)
        {
        }
    };

    vector<tmp> tests;
    {
        tests.push_back(tmp(2, 6, noLimit, ErrCompacted));
        tests.push_back(tmp(3, 4, noLimit, ErrCompacted));

        {
            CRaftEntry entry;

            tmp t(4, 5, noLimit, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);
            tests.push_back(t);
        }

        {
            CRaftEntry entry;

            tmp t(4, 6, noLimit, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            tests.push_back(t);
        }

        {
            CRaftEntry entry;

            tmp t(4, 7, noLimit, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            entry.set_index(6);
            entry.set_term(6);
            t.entries.push_back(entry);
            tests.push_back(t);
        }
        // even if maxsize is zero, the first entry should be returned
        {
            CRaftEntry entry;

            tmp t(4, 7, 0, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            tests.push_back(t);
        }
        // limit to 2
        {
            CRaftEntry entry;

            size_t size = serializer.ByteSize(entries[1]) + serializer.ByteSize(entries[2]);
            tmp t(4, 7, size, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            tests.push_back(t);
        }
        // limit to 2
        {
            CRaftEntry entry;
            size_t size = serializer.ByteSize(entries[1]) + serializer.ByteSize(entries[2]) + serializer.ByteSize(entries[3])/2;
            tmp t(4, 7, size, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            tests.push_back(t);
        }
        {
            CRaftEntry entry;
            size_t size = serializer.ByteSize(entries[1]) + serializer.ByteSize(entries[2]) + serializer.ByteSize(entries[3]) -1;
            tmp t(4, 7, size, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            tests.push_back(t);
        }
        // all
        {
            CRaftEntry entry;
            size_t size = serializer.ByteSize(entries[1]) + serializer.ByteSize(entries[2]) + serializer.ByteSize(entries[3]);
            tmp t(4, 7, size, OK);

            entry.set_index(4);
            entry.set_term(4);
            t.entries.push_back(entry);

            entry.set_index(5);
            entry.set_term(5);
            t.entries.push_back(entry);

            entry.set_index(6);
            entry.set_term(6);
            t.entries.push_back(entry);

            tests.push_back(t);
        }
    }
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        EntryVec ret;
        s.entries_ = entries;

        int err = s.Entries(test.lo, test.hi, test.maxsize, ret);
        CPPUNIT_ASSERT_EQUAL(err, test.werr);
        CPPUNIT_ASSERT(isDeepEqualEntries(ret, test.entries));
    }
}

void CTestMemStorageFixture::TestStorageLastIndex(void)
{
    EntryVec entries;

    {
        CRaftEntry entry;

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(5);
        entries.push_back(entry);
    }

    CRaftMemStorage s(&kDefaultLogger);
    s.entries_ = entries;

    uint64_t last;
    int err = s.LastIndex(last);
    CPPUNIT_ASSERT_EQUAL(int(OK), err);
    CPPUNIT_ASSERT(5 == last);

    {
        EntryVec entries;
        CRaftEntry entry;

        entry.set_index(6);
        entry.set_term(5);
        entries.push_back(entry);
        s.Append(entries);
    }

    err = s.LastIndex(last);
    CPPUNIT_ASSERT(OK == err);
    CPPUNIT_ASSERT(6 == last);
}

void CTestMemStorageFixture::TestStorageFirstIndex(void)
{
    EntryVec entries;

    {
        CRaftEntry entry;

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(5);
        entries.push_back(entry);
    }

    CRaftMemStorage s(&kDefaultLogger);
    s.entries_ = entries;

    {
        uint64_t first;
        int err = s.FirstIndex(first);

        CPPUNIT_ASSERT(OK == err);
        CPPUNIT_ASSERT(4 == first);
    }

    s.Compact(4);

    {
        uint64_t first;
        int err = s.FirstIndex(first);

        CPPUNIT_ASSERT(OK ==err);
        CPPUNIT_ASSERT(5 == first);
    }
}

void CTestMemStorageFixture::TestStorageCompact(void)
{
    EntryVec entries;

    {
        CRaftEntry entry;

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(5);
        entries.push_back(entry);
    }

    CRaftMemStorage s(&kDefaultLogger);
    s.entries_ = entries;

    struct tmp
    {
        uint64_t i;
        int werr;
        uint64_t wterm;
        uint64_t windex;
        int wlen;

        tmp(uint64_t i, int err, uint64_t index, uint64_t term, int len)
            : i(i), werr(err), wterm(term), windex(index), wlen(len)
        {
        }
    };
    vector<tmp> tests;
    tests.push_back(tmp(2, ErrCompacted, 3, 3, 3));
    tests.push_back(tmp(3, ErrCompacted, 3, 3, 3));
    tests.push_back(tmp(4, OK, 4, 4, 2));
    tests.push_back(tmp(5, OK, 5, 5, 1));
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        s.entries_ = entries;

        int err = s.Compact(test.i);
        CPPUNIT_ASSERT_EQUAL(err, test.werr);
        CPPUNIT_ASSERT_EQUAL(s.entries_[0].index(), test.windex);
        CPPUNIT_ASSERT_EQUAL(s.entries_[0].term(), test.wterm);
        CPPUNIT_ASSERT(s.entries_.size() == test.wlen);
    }
}

//TODO:TestStorageCreateSnapshot

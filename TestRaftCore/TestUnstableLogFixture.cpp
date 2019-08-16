#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TestUnstableLogFixture.h"
#include "UnstableLog.h"
#include "NullLogger.h"
#include "RaftUtil.h"
CNullLogger kDefaultLogger;

CPPUNIT_TEST_SUITE_REGISTRATION(CTestUnstableLogFixture);

CTestUnstableLogFixture::CTestUnstableLogFixture()
{
}


CTestUnstableLogFixture::~CTestUnstableLogFixture()
{
}

void CTestUnstableLogFixture::setUp(void)
{
}

void CTestUnstableLogFixture::tearDown(void)
{

}

void CTestUnstableLogFixture::TestExample(void)
{
}

#include <RaftDef.h>
void CTestUnstableLogFixture::TestUnstableMaybeFirstIndex()
{
    struct tmp
    {
        EntryVec entries;
        uint64_t offset;
        CSnapshot *snapshot;
        bool wok;
        uint64_t windex;

        tmp(EntryVec ens, uint64_t off, CSnapshot *snap, bool w, uint64_t index)
            : entries(ens), offset(off), snapshot(snap), wok(w), windex(index)
        {
        }
    };

    vector<tmp> tests;

    // no  snapshot
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        tmp t(entries, 5, NULL, false, 0);

        tests.push_back(t);
    }
    {
        EntryVec entries;
        tmp t(entries, 0, NULL, false, 0);

        tests.push_back(t);
    }
    // has snapshot
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, true, 5);

        tests.push_back(t);
    }
    {
        EntryVec entries;
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, true, 5);

        tests.push_back(t);
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CUnstableLog unstable;
        unstable.m_vecEntries = tests[i].entries;
        unstable.m_u64Offset = tests[i].offset;
        unstable.m_pSnapshot = tests[i].snapshot;
        unstable.m_pLogger = NULL;

        uint64_t index;
        bool ok = unstable.MaybeFirstIndex(index);
        CPPUNIT_ASSERT(tests[i].wok == ok);
    }
}

void CTestUnstableLogFixture::TestMaybeLastIndex(void)
{
    struct tmp
    {
        EntryVec entries;
        uint64_t offset;
        CSnapshot *snapshot;
        bool wok;
        uint64_t windex;

        tmp(EntryVec ens, uint64_t off, CSnapshot *snap, bool w, uint64_t index)
            : entries(ens), offset(off), snapshot(snap), wok(w), windex(index)
        {
        }
    };

    vector<tmp> tests;

    // last in entries
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        tmp t(entries, 5, NULL, true, 5);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, true, 5);

        tests.push_back(t);
    }
    // last in entries
    {
        EntryVec entries;
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, true, 4);

        tests.push_back(t);
    }
    // empty unstable
    {
        EntryVec entries;
        tmp t(entries, 0, NULL, false, 0);

        tests.push_back(t);
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CUnstableLog unstable;
        unstable.m_vecEntries = tests[i].entries;
        unstable.m_u64Offset = tests[i].offset;
        unstable.m_pSnapshot = tests[i].snapshot;
        unstable.m_pLogger = NULL;

        uint64_t index;
        bool ok = unstable.MaybeLastIndex(index);
        CPPUNIT_ASSERT_EQUAL(tests[i].wok, ok);
    }
}

void CTestUnstableLogFixture::TestUnstableMaybeTerm(void)
{
    struct tmp
    {
        EntryVec entries;
        uint64_t offset;
        CSnapshot *snapshot;
        uint64_t index;
        bool wok;
        uint64_t wterm;

        tmp(EntryVec ens, uint64_t off, CSnapshot *snap, uint64_t i, bool w, uint64_t t)
            : entries(ens), offset(off), snapshot(snap), index(i), wok(w), wterm(t)
        {
        }
    };

    vector<tmp> tests;
    // term from entries
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        tmp t(entries, 5, NULL, 5, true, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        tmp t(entries, 5, NULL, 6, false, 0);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        tmp t(entries, 5, NULL, 4, false, 0);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 5, true, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 6, false, 0);

        tests.push_back(t);
    }
    // term from snapshot
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 4, true, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 3, false, 0);

        tests.push_back(t);
    }
    {
        EntryVec entries;
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 5, false, 0);

        tests.push_back(t);
    }
    {
        EntryVec entries;
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        tmp t(entries, 5, snapshot, 4, true, 1);

        tests.push_back(t);
    }
    {
        EntryVec entries;
        tmp t(entries, 0, NULL, 5, false, 0);

        tests.push_back(t);
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CUnstableLog unstable;
        unstable.m_vecEntries = tests[i].entries;
        unstable.m_u64Offset = tests[i].offset;
        unstable.m_pSnapshot = tests[i].snapshot;
        unstable.m_pLogger = NULL;

        uint64_t term;
        bool ok = unstable.MaybeTerm(tests[i].index, term);
        CPPUNIT_ASSERT_EQUAL(tests[i].wok, ok);
        CPPUNIT_ASSERT_EQUAL(tests[i].wterm, term);
    }
}

void CTestUnstableLogFixture::TestUnstableRestore(void)
{
    CSnapshot *snapshot = new CSnapshot();
    CSnapshotMetadata *meta = snapshot->mutable_metadata();
    meta->set_index(4);
    meta->set_term(1);
    CRaftEntry entry;
    entry.set_index(5);
    entry.set_term(1);
    EntryVec entries;
    entries.push_back(entry);

    CUnstableLog unstable;
    unstable.m_vecEntries = entries;
    unstable.m_u64Offset = 5;
    unstable.m_pSnapshot = snapshot;
    unstable.m_pLogger = NULL;

    CSnapshot s;
    {
        CSnapshotMetadata *meta = s.mutable_metadata();
        meta->set_index(6);
        meta->set_term(2);
        unstable.Restore(s);
    }

    CPPUNIT_ASSERT_EQUAL(unstable.m_u64Offset, s.metadata().index() + 1);
    CPPUNIT_ASSERT_EQUAL(int(unstable.m_vecEntries.size()), 0);
    CPPUNIT_ASSERT_EQUAL(true, isDeepEqualSnapshot(unstable.m_pSnapshot, &s));
}

void CTestUnstableLogFixture::TestUnstableStableTo(void)
{
    struct tmp
    {
        EntryVec entries;
        uint64_t offset;
        CSnapshot *snapshot;
        uint64_t index, term;

        uint64_t woffset;
        int wlen;

        tmp(EntryVec ens, uint64_t off, CSnapshot *snap, uint64_t index, uint64_t term, uint64_t wo, int wl)
            : entries(ens), offset(off), snapshot(snap), index(index), term(term), woffset(wo), wlen(wl)
        {
        }
    };

    vector<tmp> tests;
    {
        EntryVec entries;
        tmp t(entries, 0, NULL, 5, 1, 0, 0);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        // stable to the first entry
        tmp t(entries, 5, NULL, 5, 1, 6, 0);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
        // stable to the first entry
        tmp t(entries, 5, NULL, 5, 1, 6, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(6);
        entry.set_term(2);
        EntryVec entries;
        entries.push_back(entry);
        // stable to the first entry and term mismatch
        tmp t(entries, 6, NULL, 6, 1, 6, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        // stable to old entry
        tmp t(entries, 5, NULL, 4, 1, 5, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        // stable to old entry
        tmp t(entries, 5, NULL, 4, 2, 5, 1);

        tests.push_back(t);
    }
    // with snapshot
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        // stable to the first entry
        tmp t(entries, 5, snapshot, 5, 1, 6, 0);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(1);
        // stable to the first entry
        tmp t(entries, 5, snapshot, 5, 1, 6, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(6);
        entry.set_term(2);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(5);
        meta->set_term(1);
        // stable to the first entry and term mismatch
        tmp t(entries, 6, snapshot, 6, 1, 6, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(1);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(5);
        meta->set_term(1);
        // stable to snapshot
        tmp t(entries, 5, snapshot, 4, 1, 5, 1);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        entry.set_index(5);
        entry.set_term(2);
        EntryVec entries;
        entries.push_back(entry);
        CSnapshot *snapshot = new CSnapshot();
        CSnapshotMetadata *meta = snapshot->mutable_metadata();
        meta->set_index(4);
        meta->set_term(2);
        // stable to snapshot
        tmp t(entries, 5, snapshot, 4, 1, 5, 1);

        tests.push_back(t);
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CUnstableLog unstable;
        unstable.m_vecEntries = tests[i].entries;
        unstable.m_u64Offset = tests[i].offset;
        unstable.m_pSnapshot = tests[i].snapshot;
        unstable.m_pLogger = NULL;

        unstable.StableTo(tests[i].index, tests[i].term);
        CPPUNIT_ASSERT_EQUAL(unstable.m_u64Offset, tests[i].woffset);
        CPPUNIT_ASSERT_EQUAL(int(unstable.m_vecEntries.size()), tests[i].wlen);
    }
}

void CTestUnstableLogFixture::TestUnstableTruncateAndAppend(void)
{
    struct tmp
    {
        EntryVec entries;
        uint64_t offset;
        CSnapshot *snapshot;
        EntryVec toappend;
        uint64_t woffset;
        EntryVec wentries;

        tmp(EntryVec ens, uint64_t off, CSnapshot *snap, EntryVec append, uint64_t wo, EntryVec wents)
            : entries(ens), offset(off), snapshot(snap), toappend(append), woffset(wo), wentries(wents)
        {
        }
    };

    vector<tmp> tests;
    // append to the end
    {
        CRaftEntry entry;
        EntryVec entries, append, wents;

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(1);
        append.push_back(entry);
        entry.set_index(7);
        entry.set_term(1);
        append.push_back(entry);

        entry.set_index(5);
        entry.set_term(1);
        wents.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        wents.push_back(entry);
        entry.set_index(7);
        entry.set_term(1);
        wents.push_back(entry);
        tmp t(entries, 5, NULL, append, 5, wents);

        tests.push_back(t);
    }
    // replace the unstable entries
    {
        CRaftEntry entry;
        EntryVec entries, append, wents;

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(2);
        append.push_back(entry);
        entry.set_index(6);
        entry.set_term(2);
        append.push_back(entry);

        entry.set_index(5);
        entry.set_term(2);
        wents.push_back(entry);
        entry.set_index(6);
        entry.set_term(2);
        wents.push_back(entry);

        tmp t(entries, 5, NULL, append, 5, wents);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        EntryVec entries, append, wents;

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(2);
        append.push_back(entry);
        entry.set_index(5);
        entry.set_term(2);
        append.push_back(entry);
        entry.set_index(6);
        entry.set_term(2);
        append.push_back(entry);

        entry.set_index(4);
        entry.set_term(2);
        wents.push_back(entry);
        entry.set_index(5);
        entry.set_term(2);
        wents.push_back(entry);
        entry.set_index(6);
        entry.set_term(2);
        wents.push_back(entry);

        tmp t(entries, 5, NULL, append, 4, wents);

        tests.push_back(t);
    }
    // truncate the existing entries and append
    {
        CRaftEntry entry;
        EntryVec entries, append, wents;

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
        entry.set_index(7);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(2);
        append.push_back(entry);

        entry.set_index(5);
        entry.set_term(1);
        wents.push_back(entry);
        entry.set_index(6);
        entry.set_term(2);
        wents.push_back(entry);

        tmp t(entries, 5, NULL, append, 5, wents);

        tests.push_back(t);
    }
    {
        CRaftEntry entry;
        EntryVec entries, append, wents;

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
        entry.set_index(7);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(7);
        entry.set_term(2);
        append.push_back(entry);
        entry.set_index(8);
        entry.set_term(2);
        append.push_back(entry);

        entry.set_index(5);
        entry.set_term(1);
        wents.push_back(entry);
        entry.set_index(6);
        entry.set_term(1);
        wents.push_back(entry);
        entry.set_index(7);
        entry.set_term(2);
        wents.push_back(entry);
        entry.set_index(8);
        entry.set_term(2);
        wents.push_back(entry);

        tmp t(entries, 5, NULL, append, 5, wents);

        tests.push_back(t);
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CUnstableLog unstable;
        unstable.m_vecEntries = tests[i].entries;
        unstable.m_u64Offset = tests[i].offset;
        unstable.m_pSnapshot = tests[i].snapshot;
        unstable.m_pLogger = &kDefaultLogger;

        unstable.TruncateAndAppend(tests[i].toappend);
        CPPUNIT_ASSERT_EQUAL(unstable.m_u64Offset, tests[i].woffset);
        CPPUNIT_ASSERT_EQUAL(true, isDeepEqualEntries(unstable.m_vecEntries, tests[i].wentries));
    }
}

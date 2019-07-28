#include "stdafx.h"
#include "TestRaftLogFixture.h"
#include "RaftMemLog.h"
#include "RaftMemStorage.h"
#include "NullLogger.h"
#include "RaftUtil.h"
extern CNullLogger kDefaultLogger;

CPPUNIT_TEST_SUITE_REGISTRATION(CTestRaftLogFixture);

CTestRaftLogFixture::CTestRaftLogFixture()
{
}

CTestRaftLogFixture::~CTestRaftLogFixture()
{
}

void CTestRaftLogFixture::setUp(void)
{
}

void CTestRaftLogFixture::tearDown(void)
{

}

void CTestRaftLogFixture::TestFindConflict(void)
{
    EntryVec entries;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);
    }

    struct tmp
    {
        uint64_t wconflict;
        EntryVec entries;

        tmp(uint64_t conflict) : wconflict(conflict)
        {
        }
    };

    vector<tmp> tests;
    // no conflict, empty ent
    {
        tmp t(0);
        tests.push_back(t);
    }
    // no conflict
    {
        tmp t(0);
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        t.entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(0);
        Entry entry;

        entry.set_index(2);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(0);
        Entry entry;

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    // no conflict, but has new entries
    {
        tmp t(4);
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        t.entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(4);
        Entry entry;

        entry.set_index(2);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(4);
        Entry entry;

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(4);
        Entry entry;

        entry.set_index(4);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    // conflicts with existing entries
    {
        tmp t(1);
        Entry entry;

        entry.set_index(1);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(2);
        Entry entry;

        entry.set_index(2);
        entry.set_term(1);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(3);
        Entry entry;

        entry.set_index(3);
        entry.set_term(1);
        t.entries.push_back(entry);

        entry.set_index(4);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }

    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        log->Append(entries);

        uint64_t conflict = log->findConflict(test.entries);
        CPPUNIT_ASSERT_EQUAL(conflict, test.wconflict);

        delete log;
    }
}

void CTestRaftLogFixture::TestIsUpToDate(void)
{
    EntryVec entries;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);
    }

    struct tmp
    {
        uint64_t lastindex;
        uint64_t term;
        bool isUpToDate;

        tmp(uint64_t last, uint64_t term, bool uptodate)
            : lastindex(last), term(term), isUpToDate(uptodate)
        {
        }
    };

    CRaftMemStorage s(&kDefaultLogger);
    CRaftMemLog *log = newLog(&s, &kDefaultLogger);
    log->Append(entries);

    vector<tmp> tests;
    // greater term, ignore GetLastIndex
    tests.push_back(tmp(log->GetLastIndex() - 1, 4, true));
    tests.push_back(tmp(log->GetLastIndex(), 4, true));
    tests.push_back(tmp(log->GetLastIndex() + 1, 4, true));
    // smaller term, ignore GetLastIndex
    tests.push_back(tmp(log->GetLastIndex() - 1, 2, false));
    tests.push_back(tmp(log->GetLastIndex(), 2, false));
    tests.push_back(tmp(log->GetLastIndex() + 1, 2, false));
    // equal term, equal or lager GetLastIndex wins
    tests.push_back(tmp(log->GetLastIndex() - 1, 3, false));
    tests.push_back(tmp(log->GetLastIndex(), 3, true));
    tests.push_back(tmp(log->GetLastIndex() + 1, 3, true));
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        bool isuptodate = log->IsUpToDate(test.lastindex, test.term);
        CPPUNIT_ASSERT_EQUAL(isuptodate, test.isUpToDate);
    }

    delete log;
}

void CTestRaftLogFixture::TestAppend(void)
{
    EntryVec entries;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);
    }

    struct tmp
    {
        uint64_t windex, wunstable;
        EntryVec entries, wentries;

        tmp(uint64_t index, uint64_t unstable) : windex(index), wunstable(unstable)
        {
        }
    };
    vector<tmp> tests;
    {
        tmp t(2, 3);
        Entry entry;
        entry.set_index(1);
        entry.set_term(1);
        t.wentries.push_back(entry);
        entry.set_index(2);
        entry.set_term(2);
        t.wentries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(3, 3);
        Entry entry;

        entry.set_index(3);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(1);
        entry.set_term(1);
        t.wentries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        t.wentries.push_back(entry);

        entry.set_index(3);
        entry.set_term(2);
        t.wentries.push_back(entry);

        tests.push_back(t);
    }
    // conflicts with index 1
    {
        tmp t(1, 1);
        Entry entry;

        entry.set_index(1);
        entry.set_term(2);
        t.entries.push_back(entry);

        entry.set_index(1);
        entry.set_term(2);
        t.wentries.push_back(entry);

        tests.push_back(t);
    }
    // conflicts with index 2
    {
        tmp t(3, 2);
        Entry entry;

        entry.set_index(2);
        entry.set_term(3);
        t.entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.entries.push_back(entry);

        entry.set_index(1);
        entry.set_term(1);
        t.wentries.push_back(entry);

        entry.set_index(2);
        entry.set_term(3);
        t.wentries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        t.wentries.push_back(entry);
        tests.push_back(t);
    }
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        s.Append(entries);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        uint64_t index = log->Append(test.entries);

        CPPUNIT_ASSERT_EQUAL(index, test.windex);

        EntryVec entries;
        int err = log->GetEntries(1, noLimit, entries);
        CPPUNIT_ASSERT(err == OK);
        CPPUNIT_ASSERT(isDeepEqualEntries(entries, test.wentries));
        CPPUNIT_ASSERT_EQUAL(log->m_unstablePart.m_u64Offset, test.wunstable);
        delete log;
    }
}

// TestLogMaybeAppend ensures:
// If the given (index, term) matches with the existing log:
//  1. If an existing entry conflicts with a new one (same index
//  but different terms), delete the existing entry and all that
//  follow it
//  2.Append any new entries not already in the log
// If the given (index, term) does not match with the existing log:
//  return false
void CTestRaftLogFixture::TestLogMaybeAppend(void)
{
    EntryVec entries;
    uint64_t lastindex = 3;
    uint64_t lastterm = 3;
    uint64_t commit = 1;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t logTerm;
        uint64_t index;
        uint64_t commited;
        EntryVec entries;

        uint64_t lasti;
        bool wappend;
        uint64_t wcommit;
        bool wpanic;

        tmp(uint64_t logterm, uint64_t index, uint64_t commited,
            uint64_t lasti, bool Append, uint64_t commit, bool panic)
            : logTerm(logterm), index(index), commited(commited),
            lasti(lasti), wappend(Append), wcommit(commit), wpanic(panic)
        {
        }
    };

    vector<tmp> tests;
    // not match: term is different
    {
        tmp t(lastterm - 1, lastindex, lastindex, 0, false, commit, false);
        Entry entry;

        entry.set_index(lastindex + 1);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    // not match: index out of bound
    {
        tmp t(lastterm, lastindex + 1, lastindex, 0, false, commit, false);
        Entry entry;

        entry.set_index(lastindex + 2);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    // match with the last existing entry
    {
        tmp t(lastterm, lastindex, lastindex, lastindex, true, lastindex, false);

        tests.push_back(t);
    }
    {
        // do not increase commit higher than lastnewi
        tmp t(lastterm, lastindex, lastindex + 1, lastindex, true, lastindex, false);

        tests.push_back(t);
    }
    {
        // commit up to the commit in the message
        tmp t(lastterm, lastindex, lastindex - 1, lastindex, true, lastindex - 1, false);

        tests.push_back(t);
    }
    {
        // commit do not decrease
        tmp t(lastterm, lastindex, 0, lastindex, true, commit, false);

        tests.push_back(t);
    }
    {
        // commit do not decrease
        tmp t(0, 0, lastindex, 0, true, commit, false);

        tests.push_back(t);
    }
    {
        tmp t(lastterm, lastindex, lastindex, lastindex + 1, true, lastindex, false);

        Entry entry;

        entry.set_index(lastindex + 1);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        // do not increase commit higher than lastnewi
        tmp t(lastterm, lastindex, lastindex + 2, lastindex + 1, true, lastindex + 1, false);

        Entry entry;

        entry.set_index(lastindex + 1);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(lastterm, lastindex, lastindex + 2, lastindex + 2, true, lastindex + 2, false);

        Entry entry;

        entry.set_index(lastindex + 1);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(lastindex + 2);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    // match with the the entry in the middle
    {
        tmp t(lastterm - 1, lastindex - 1, lastindex, lastindex, true, lastindex, false);

        Entry entry;

        entry.set_index(lastindex);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(lastterm - 2, lastindex - 2, lastindex, lastindex - 1, true, lastindex - 1, false);

        Entry entry;

        entry.set_index(lastindex - 1);
        entry.set_term(4);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    // conflict with existing committed entry
    /*
    {
    tmp t(lastterm-3, lastindex-3, lastindex, lastindex-2, true, lastindex-2, true);

    Entry entry;

    entry.set_index(lastindex-2);
    entry.set_term(4);
    t.entries.push_back(entry);

    tests.push_back(t);
    }
    */
    {
        tmp t(lastterm - 2, lastindex - 2, lastindex, lastindex, true, lastindex, false);

        Entry entry;

        entry.set_index(lastindex - 1);
        entry.set_term(4);
        t.entries.push_back(entry);

        entry.set_index(lastindex);
        entry.set_term(4);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &test = tests[i];
        CRaftMemStorage s(&kDefaultLogger);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);
        log->Append(entries);
        log->m_pStorage->m_u64Committed = commit;

        uint64_t glasti;
        bool ok = log->MaybeAppend(test.index, test.logTerm, test.commited, test.entries, glasti);
        uint64_t gcommit = log->m_pStorage->m_u64Committed;

        CPPUNIT_ASSERT_EQUAL(glasti, test.lasti);
        CPPUNIT_ASSERT_EQUAL(ok, test.wappend);
        CPPUNIT_ASSERT_EQUAL(gcommit, test.wcommit);
        if (glasti > 0 && test.entries.size() != 0)
        {
            EntryVec entries;
            int err = log->GetSliceEntries(log->GetLastIndex() - test.entries.size() + 1, log->GetLastIndex() + 1, noLimit, entries);
            CPPUNIT_ASSERT(err == OK);
            CPPUNIT_ASSERT(isDeepEqualEntries(test.entries, entries));
        }
        delete log;
    }
}

// TestCompactionSideEffects ensures that all the log related functionality works correctly after
// a compaction.
void CTestRaftLogFixture::TestCompactionSideEffects(void)
{
    // Populate the log with 1000 entries; 750 in stable storage and 250 in unstable.
    uint64_t GetLastIndex = 1000;
    uint64_t unstableIndex = 750;
    uint64_t lastTerm = GetLastIndex;
    CRaftMemStorage s(&kDefaultLogger);
    int i;

    for (i = 1; i <= unstableIndex; ++i)
    {
        EntryVec entries;
        Entry entry;

        entry.set_index(i);
        entry.set_term(i);
        entries.push_back(entry);

        s.Append(entries);
    }

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);
    for (i = unstableIndex; i < GetLastIndex; ++i)
    {
        EntryVec entries;
        Entry entry;

        entry.set_index(i + 1);
        entry.set_term(i + 1);
        entries.push_back(entry);

        log->Append(entries);
    }

    bool ok = log->MaybeCommit(GetLastIndex, lastTerm);
    CPPUNIT_ASSERT(ok);

    log->AppliedTo(log->m_pStorage->m_u64Committed);

    uint64_t offset = 500;
    s.Compact(offset);

    CPPUNIT_ASSERT_EQUAL(log->GetLastIndex(), GetLastIndex);

    for (i = offset; i <= log->GetLastIndex(); ++i)
    {
        uint64_t t;
        int err = log->GetTerm(i, t);
        CPPUNIT_ASSERT(err == OK);
        CPPUNIT_ASSERT(t == i);
    }

    for (i = offset; i <= log->GetLastIndex(); ++i)
    {
        CPPUNIT_ASSERT(log->IsMatchTerm(i, i));
    }

    EntryVec unstableEntries;
    log->unstableEntries(unstableEntries);
    CPPUNIT_ASSERT(unstableEntries.size() == 250);
    CPPUNIT_ASSERT(unstableEntries[0].index() == 751);

    uint64_t prev = log->GetLastIndex();
    {
        EntryVec entries;
        Entry entry;

        entry.set_index(log->GetLastIndex() + 1);
        entry.set_term(log->GetLastIndex() + 1);
        entries.push_back(entry);

        log->Append(entries);
    }
    CPPUNIT_ASSERT_EQUAL(log->GetLastIndex(), prev + 1);

    {
        EntryVec entries;
        int err = log->GetEntries(log->GetLastIndex(), noLimit, entries);
        CPPUNIT_ASSERT(err == OK);
        CPPUNIT_ASSERT(entries.size()== 1);
    }

    delete log;
}

void CTestRaftLogFixture::TestHasNextEnts(void)
{
    Snapshot sn;
    sn.mutable_metadata()->set_index(3);
    sn.mutable_metadata()->set_term(1);

    EntryVec entries;
    {
        Entry entry;

        entry.set_index(4);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t applied;
        bool hasNext;

        tmp(uint64_t applied, bool hasnext)
            : applied(applied), hasNext(hasnext)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(0, true));
    tests.push_back(tmp(3, true));
    tests.push_back(tmp(4, true));
    tests.push_back(tmp(5, false));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CRaftMemStorage s(&kDefaultLogger);
        s.ApplySnapshot(sn);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        log->Append(entries);
        log->MaybeCommit(5, 1);
        log->AppliedTo(tests[i].applied);

        CPPUNIT_ASSERT_EQUAL(log->hasNextEntries(), tests[i].hasNext);

        delete log;
    }
}

void CTestRaftLogFixture::TestNextEnts(void)
{
    Snapshot sn;
    sn.mutable_metadata()->set_index(3);
    sn.mutable_metadata()->set_term(1);

    EntryVec entries;
    {
        Entry entry;

        entry.set_index(4);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(5);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(6);
        entry.set_term(1);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t applied;
        EntryVec entries;

        tmp(uint64_t applied)
            : applied(applied)
        {
        }
    };

    vector<tmp> tests;
    {
        tmp t(0);
        t.entries.insert(t.entries.begin(), entries.begin(), entries.begin() + 2);
        tests.push_back(t);
    }
    {
        tmp t(3);
        t.entries.insert(t.entries.begin(), entries.begin(), entries.begin() + 2);
        tests.push_back(t);
    }
    {
        tmp t(4);
        t.entries.insert(t.entries.begin(), entries.begin() + 1, entries.begin() + 2);
        tests.push_back(t);
    }
    {
        tmp t(5);
        tests.push_back(t);
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        CRaftMemStorage s(&kDefaultLogger);
        s.ApplySnapshot(sn);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        log->Append(entries);
        log->MaybeCommit(5, 1);
        log->AppliedTo(tests[i].applied);

        EntryVec nextEntries;
        log->nextEntries(nextEntries);

        CPPUNIT_ASSERT(isDeepEqualEntries(nextEntries, tests[i].entries));

        delete log;
    }
}

// TestUnstableEnts ensures unstableEntries returns the unstable part of the
// entries correctly.
void CTestRaftLogFixture::TestUnstableEnts(void)
{
    EntryVec entries;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t unstable;
        EntryVec entries;

        tmp(uint64_t unstable) : unstable(unstable)
        {
        }
    };

    vector<tmp> tests;
    {
        tmp t(3);
        tests.push_back(t);
    }
    {
        tmp t(1);
        t.entries = entries;
        tests.push_back(t);
    }

    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];

        // Append stable entries to storage
        CRaftMemStorage s(&kDefaultLogger);
        {
            EntryVec ents;
            ents.insert(ents.end(), entries.begin(), entries.begin() + (t.unstable - 1));
            s.Append(ents);
        }

        // Append unstable entries to raftlog
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);
        {
            EntryVec ents;
            ents.insert(ents.end(), entries.begin() + (t.unstable - 1), entries.end());
            log->Append(ents);
        }

        EntryVec unstableEntries;
        log->unstableEntries(unstableEntries);

        int len = unstableEntries.size();
        if (len > 0)
        {
            log->StableTo(unstableEntries[len - 1].index(), unstableEntries[len - i].term());
        }
        CPPUNIT_ASSERT(isDeepEqualEntries(unstableEntries, t.entries));

        uint64_t w = entries[entries.size() - 1].index() + 1;
        CPPUNIT_ASSERT_EQUAL(log->m_unstablePart.m_u64Offset, w);

        delete log;
    }
}

void CTestRaftLogFixture::TestCommitTo(void)
{
    EntryVec entries;
    uint64_t commit = 2;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);

        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t commit, wcommit;
        bool panic;

        tmp(uint64_t commit, uint64_t wcommit, bool panic)
            : commit(commit), wcommit(wcommit), panic(panic)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(3, 3, false));
    tests.push_back(tmp(1, 2, false));  // never decrease
                                        //tests.push_back(tmp(4,0,true));   // commit out of range -> panic

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];

        CRaftMemStorage s(&kDefaultLogger);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        log->Append(entries);
        log->m_pStorage->m_u64Committed = commit;
        log->CommitTo(t.commit);
        CPPUNIT_ASSERT(log->m_pStorage->m_u64Committed ==t.wcommit);
        delete log;
    }
}

void CTestRaftLogFixture::TestStableTo(void)
{
    EntryVec entries;

    {
        Entry entry;

        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);
    }
    struct tmp
    {
        uint64_t stablei, stablet, wunstable;

        tmp(uint64_t stablei, uint64_t stablet, uint64_t wunstable)
            : stablei(stablei), stablet(stablet), wunstable(wunstable)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(1, 1, 2));
    tests.push_back(tmp(2, 2, 3));
    tests.push_back(tmp(2, 1, 1));  // bad term
    tests.push_back(tmp(3, 1, 1));  // bad index

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];

        CRaftMemStorage s(&kDefaultLogger);
        CRaftMemLog *log = newLog(&s, &kDefaultLogger);

        log->Append(entries);
        log->StableTo(t.stablei, t.stablet);
        CPPUNIT_ASSERT_EQUAL(log->m_unstablePart.m_u64Offset, t.wunstable);
        delete log;
    }
}

void CTestRaftLogFixture::TestStableToWithSnap(void)
{
    uint64_t snapi = 5, snapt = 2;

    struct tmp
    {
        uint64_t stablei, stablet, wunstable;
        EntryVec entries;

        tmp(uint64_t stablei, uint64_t stablet, uint64_t wunstable)
            : stablei(stablei), stablet(stablet), wunstable(wunstable)
        {
        }
    };

    vector<tmp> tests;
    {
        tmp t(snapi + 1, snapt, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi, snapt + 1, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi - 1, snapt, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi + 1, snapt + 1, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi, snapt + 1, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi - 1, snapt + 1, snapi + 1);
        tests.push_back(t);
    }
    {
        tmp t(snapi + 1, snapt, snapi + 2);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(snapi, snapt, snapi + 1);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(snapi - 1, snapt, snapi + 1);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(snapi + 1, snapt + 1, snapi + 1);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(snapi, snapt + 1, snapi + 1);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(snapi - 1, snapt + 1, snapi + 1);
        Entry entry;
        entry.set_index(snapi + 1);
        entry.set_term(snapt);
        t.entries.push_back(entry);
        tests.push_back(t);
    }

    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];

        CRaftMemStorage s(&kDefaultLogger);

        Snapshot sn;
        sn.mutable_metadata()->set_index(snapi);
        sn.mutable_metadata()->set_term(snapt);
        s.ApplySnapshot(sn);

        CRaftMemLog *log = newLog(&s, &kDefaultLogger);
        log->Append(t.entries);
        log->StableTo(t.stablei, t.stablet);
        CPPUNIT_ASSERT_EQUAL(log->m_unstablePart.m_u64Offset, t.wunstable);
        delete log;
    }
}

//TestCompaction ensures that the number of log entries is correct after compactions.
void CTestRaftLogFixture::TestCompaction(void)
{
    struct tmp
    {
        uint64_t GetLastIndex;
        vector<uint64_t> compact;
        vector<int> wleft;
        bool wallow;

        tmp(uint64_t i, bool allow)
            : GetLastIndex(i), wallow(allow)
        {
        }
    };

    vector<tmp> tests;
    {
        // out of upper bound
        tmp t(1000, false);
        t.compact.push_back(1001);
        t.wleft.push_back(-1);

        //tests.push_back(t);
    }
    {
        // out of upper bound
        tmp t(1000, true);
        t.compact.push_back(300);
        t.compact.push_back(500);
        t.compact.push_back(800);
        t.compact.push_back(900);
        t.wleft.push_back(700);
        t.wleft.push_back(500);
        t.wleft.push_back(200);
        t.wleft.push_back(100);

        tests.push_back(t);
    }

    int i = 0;
    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];
        int j = 0;

        CRaftMemStorage s(&kDefaultLogger);
        EntryVec entries;
        for (j = 1; j <= t.GetLastIndex; ++j)
        {
            Entry entry;
            entry.set_index(j);
            entries.push_back(entry);
        }
        s.Append(entries);

        CRaftMemLog *log = newLog(&s, &kDefaultLogger);
        log->MaybeCommit(t.GetLastIndex, 0);
        log->AppliedTo(log->m_pStorage->m_u64Committed);

        for (j = 0; j < t.compact.size(); ++j)
        {
            int err = s.Compact(t.compact[j]);
            if (!SUCCESS(err))
            {
                CPPUNIT_ASSERT(t.wallow);
                continue;
            }
            EntryVec all;
            log->allEntries(all);
            CPPUNIT_ASSERT(t.wleft[j] == all.size());
        }
        delete log;
    }
}

void CTestRaftLogFixture::TestLogRestore(void)
{
    uint64_t index = 1000;
    uint64_t term = 1000;

    Snapshot sn;
    sn.mutable_metadata()->set_index(index);
    sn.mutable_metadata()->set_term(term);

    CRaftMemStorage s(&kDefaultLogger);
    s.ApplySnapshot(sn);

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);

    EntryVec all;
    log->allEntries(all);

    CPPUNIT_ASSERT(all.empty());
    CPPUNIT_ASSERT_EQUAL(log->GetFirstIndex(), index + 1);
    CPPUNIT_ASSERT(log->m_pStorage->m_u64Committed == index);
    CPPUNIT_ASSERT(log->m_unstablePart.m_u64Offset == (index + 1));
    uint64_t t;
    int err = log->GetTerm(index, t);
    CPPUNIT_ASSERT(SUCCESS(err));
    CPPUNIT_ASSERT_EQUAL(t, term);

    delete log;
}

void CTestRaftLogFixture::TestIsOutOfBounds(void)
{
    uint64_t offset = 100;
    uint64_t num = 100;

    Snapshot sn;
    sn.mutable_metadata()->set_index(offset);

    CRaftMemStorage s(&kDefaultLogger);
    s.ApplySnapshot(sn);

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);

    int i = 0;
    EntryVec entries;
    for (i = 1; i <= num; ++i)
    {
        Entry entry;
        entry.set_index(i + offset);
        entries.push_back(entry);
    }
    log->Append(entries);

    uint64_t first = offset + 1;

    struct tmp
    {
        uint64_t lo, hi;
        bool wpanic;
        bool errCompacted;

        tmp(uint64_t lo, uint64_t hi, bool panic, bool err)
            : lo(lo), hi(hi), wpanic(panic), errCompacted(err)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(first - 2, first + 1, false, true));
    tests.push_back(tmp(first - 1, first + 1, false, true));
    tests.push_back(tmp(first, first, false, false));
    tests.push_back(tmp(first + num / 2, first + num / 2, false, false));
    tests.push_back(tmp(first + num - 1, first + num - 1, false, false));
    tests.push_back(tmp(first + num, first + num, false, false));

    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];

        int err = log->CheckOutOfBounds(t.lo, t.hi);
        CPPUNIT_ASSERT(!(t.errCompacted && err != ErrCompacted));
        CPPUNIT_ASSERT(!(!t.errCompacted && !SUCCESS(err)));
    }

    delete log;
}

void CTestRaftLogFixture::TestTerm(void)
{
    uint64_t offset = 100;
    uint64_t num = 100;

    Snapshot sn;
    sn.mutable_metadata()->set_index(offset);
    sn.mutable_metadata()->set_term(1);

    CRaftMemStorage s(&kDefaultLogger);
    s.ApplySnapshot(sn);

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);

    int i = 0;
    EntryVec entries;
    for (i = 1; i < num; ++i)
    {
        Entry entry;
        entry.set_index(i + offset);
        entry.set_term(i);
        entries.push_back(entry);
    }
    log->Append(entries);

    struct tmp
    {
        uint64_t index, w;

        tmp(uint64_t index, uint64_t w)
            : index(index), w(w)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(offset - 1, 0));
    tests.push_back(tmp(offset, 1));
    tests.push_back(tmp(offset + num / 2, num / 2));
    tests.push_back(tmp(offset + num - 1, num - 1));
    tests.push_back(tmp(offset + num, 0));

    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];
        uint64_t tm;
        int err = log->GetTerm(t.index, tm);
        CPPUNIT_ASSERT(SUCCESS(err));
        CPPUNIT_ASSERT_EQUAL(tm, t.w);
    }

    delete log;
}

void CTestRaftLogFixture::TestTermWithUnstableSnapshot(void)
{
    uint64_t storagesnapi = 100;
    uint64_t unstablesnapi = storagesnapi + 5;

    Snapshot sn;
    sn.mutable_metadata()->set_index(storagesnapi);
    sn.mutable_metadata()->set_term(1);

    CRaftMemStorage s(&kDefaultLogger);
    s.ApplySnapshot(sn);

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);
    {
        Snapshot sn;
        sn.mutable_metadata()->set_index(unstablesnapi);
        sn.mutable_metadata()->set_term(1);

        log->Restore(sn);
    }
    int i = 0;

    struct tmp
    {
        uint64_t index, w;

        tmp(uint64_t index, uint64_t w)
            : index(index), w(w)
        {
        }
    };

    vector<tmp> tests;
    // cannot get term from storage
    tests.push_back(tmp(storagesnapi, 0));
    // cannot get term from the gap between storage ents and unstable snapshot
    tests.push_back(tmp(storagesnapi + 1, 0));
    tests.push_back(tmp(unstablesnapi - 1, 0));
    // get term from unstable snapshot index
    tests.push_back(tmp(unstablesnapi, 1));

    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];
        uint64_t tm;
        int err = log->GetTerm(t.index, tm);
        CPPUNIT_ASSERT(SUCCESS(err));
        CPPUNIT_ASSERT_EQUAL(tm, t.w);
    }

    delete log;
}

void CTestRaftLogFixture::TestSlice(void)
{
    uint64_t offset = 100;
    uint64_t num = 100;
    uint64_t last = offset + num;
    uint64_t half = offset + num / 2;

    Entry halfe;
    halfe.set_index(half);
    halfe.set_term(half);

    Snapshot sn;
    sn.mutable_metadata()->set_index(offset);

    CRaftMemStorage s(&kDefaultLogger);
    s.ApplySnapshot(sn);

    CRaftMemLog *log = newLog(&s, &kDefaultLogger);

    int i = 0;
    EntryVec entries;
    for (i = 1; i < num; ++i)
    {
        Entry entry;
        entry.set_index(i + offset);
        entry.set_term(i + offset);
        entries.push_back(entry);
    }
    log->Append(entries);

    struct tmp
    {
        uint64_t from, to, limit;
        EntryVec entries;
        bool wpanic;

        tmp(uint64_t from, uint64_t to, uint64_t limit, bool panic)
            : from(from), to(to), limit(limit), wpanic(panic)
        {
        }
    };

    vector<tmp> tests;
    // test no limit
    {
        tmp t(offset - 1, offset + 1, noLimit, false);
        tests.push_back(t);
    }
    {
        tmp t(offset, offset + 1, noLimit, false);
        tests.push_back(t);
    }
    {
        tmp t(half - 1, half + 1, noLimit, false);
        Entry entry;

        entry.set_index(half - 1);
        entry.set_term(half - 1);
        t.entries.push_back(entry);

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(half, half + 1, noLimit, false);
        Entry entry;

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(last - 1, last, noLimit, false);
        Entry entry;

        entry.set_index(last - 1);
        entry.set_term(last - 1);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    // test limit
    {
        tmp t(half - 1, half + 1, 0, false);
        Entry entry;

        entry.set_index(half - 1);
        entry.set_term(half - 1);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(half - 1, half + 1, halfe.ByteSize() + 1, false);
        Entry entry;

        entry.set_index(half - 1);
        entry.set_term(half - 1);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(half - 2, half + 1, halfe.ByteSize() + 1, false);
        Entry entry;

        entry.set_index(half - 2);
        entry.set_term(half - 2);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(half - 1, half + 1, halfe.ByteSize() * 2, false);
        Entry entry;

        entry.set_index(half - 1);
        entry.set_term(half - 1);
        t.entries.push_back(entry);

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(half - 1, half + 2, halfe.ByteSize() * 3, false);
        Entry entry;

        entry.set_index(half - 1);
        entry.set_term(half - 1);
        t.entries.push_back(entry);

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);

        entry.set_index(half + 1);
        entry.set_term(half + 1);
        t.entries.push_back(entry);

        tests.push_back(t);
    }
    {
        tmp t(half, half + 2, halfe.ByteSize(), false);
        Entry entry;

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);
        tests.push_back(t);
    }
    {
        tmp t(half, half + 2, halfe.ByteSize() * 2, false);
        Entry entry;

        entry.set_index(half);
        entry.set_term(half);
        t.entries.push_back(entry);

        entry.set_index(half + 1);
        entry.set_term(half + 1);
        t.entries.push_back(entry);
        tests.push_back(t);
    }

    for (i = 0; i < tests.size(); ++i)
    {
        const tmp &t = tests[i];
        EntryVec ents;

        int err = log->GetSliceEntries(t.from, t.to, t.limit, ents);

        CPPUNIT_ASSERT(!(t.from <= offset && err != ErrCompacted));
        CPPUNIT_ASSERT(!(t.from > offset && !SUCCESS(err)));
        CPPUNIT_ASSERT(isDeepEqualEntries(ents, t.entries));
    }

    delete log;
}

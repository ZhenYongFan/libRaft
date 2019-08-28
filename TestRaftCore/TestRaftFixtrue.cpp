#include "stdafx.h"

#include "RaftConfig.h"
#include "ReadOnly.h"
#include "TestRaftFixtrue.h"
#include "RaftUtil.h"
#include "NullLogger.h"
extern CNullLogger kDefaultLogger;
#include "TestRaftUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPPUNIT_TEST_SUITE_REGISTRATION( CTestRaftFixtrue );

// CTestSampleFixtrue

CTestRaftFixtrue::CTestRaftFixtrue()
{
}

CTestRaftFixtrue::~CTestRaftFixtrue()
{
}

stateMachine *nopStepper;

void CTestRaftFixtrue::setUp(void)
{
    nopStepper = new blackHole();
}

void CTestRaftFixtrue::tearDown(void)
{
    delete nopStepper;
}



void preVoteConfig(CRaftConfig *c)
{
    c->m_bPreVote = true;
}

raftStateMachine* entsWithConfig(ConfigFun fun, const vector<uint64_t>& terms)
{
    vector<CRaftEntry> entries;
    int i;
    for (i = 0; i < terms.size(); ++i)
    {
        CRaftEntry entry;
        entry.set_index(i + 1);
        entry.set_term(terms[i]);
        entries.push_back(entry);
    }
    vector<uint32_t> peers;
    raftStateMachine *sm = new raftStateMachine(1, peers, 5, 1, &kDefaultLogger, entries, fun);

    CRaft *r = (CRaft*)sm->data();
    r->Reset(terms[terms.size() - 1]);
    return sm;
}

// votedWithConfig creates a CRaft state machine with Vote and Term set
// to the given value but no log entries (indicating that it voted in
// the given term but has not received any logs).
raftStateMachine* votedWithConfig(ConfigFun fun, uint64_t vote, uint64_t term)
{
    CHardState hs;
    hs.set_vote(vote);
    hs.set_term(term);
    vector<uint32_t> peers;
    raftStateMachine *sm = new raftStateMachine(1, peers, 5, 1, &kDefaultLogger, hs, fun);

    CRaft *r = (CRaft*)sm->data();
    r->Reset(term);
    return sm;
}

void CTestRaftFixtrue::TestProgressBecomeProbe(void)
{
    uint64_t match = 1;
    struct tmp
    {
        CProgress p;
        uint64_t wnext;

        tmp(CProgress p, uint64_t next)
            : p(p), wnext(next)
        {
        }
    };

    vector<tmp> tests;
    {
        CProgress p(5, 256, &kDefaultLogger);
        p.m_statePro = ProgressStateReplicate;
        p.m_u64MatchLogIndex = match;
        tests.push_back(tmp(p, 2));
    }
    // snapshot finish
    {
        CProgress p(5, 256, &kDefaultLogger);
        p.m_statePro = ProgressStateSnapshot;
        p.m_u64MatchLogIndex = match;
        p.pendingSnapshot_ = 10;
        tests.push_back(tmp(p, 11));
    }
    // snapshot failure
    {
        CProgress p(5, 256, &kDefaultLogger);
        p.m_statePro = ProgressStateSnapshot;
        p.m_u64MatchLogIndex = match;
        p.pendingSnapshot_ = 0;
        tests.push_back(tmp(p, 2));
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        t.p.BecomeProbe();
        CPPUNIT_ASSERT_EQUAL(t.p.m_statePro, ProgressStateProbe);
        CPPUNIT_ASSERT_EQUAL(t.p.m_u64MatchLogIndex, match);
        CPPUNIT_ASSERT_EQUAL(t.p.m_u64NextLogIndex, t.wnext);
    }
}

void CTestRaftFixtrue::TestProgressBecomeReplicate(void)
{
    CProgress p(5, 256, &kDefaultLogger);
    p.m_statePro = ProgressStateProbe;
    p.m_u64MatchLogIndex = 1;

    p.BecomeReplicate();
    CPPUNIT_ASSERT_EQUAL(p.m_statePro, ProgressStateReplicate);
    CPPUNIT_ASSERT(p.m_u64MatchLogIndex == 1);
    CPPUNIT_ASSERT_EQUAL(p.m_u64NextLogIndex, p.m_u64MatchLogIndex + 1);
}

void CTestRaftFixtrue::TestProgressBecomeSnapshot(void)
{
    CProgress p(5, 256, &kDefaultLogger);
    p.m_statePro = ProgressStateProbe;
    p.m_u64MatchLogIndex = 1;

    p.BecomeSnapshot(10);
    CPPUNIT_ASSERT_EQUAL(p.m_statePro, ProgressStateSnapshot);
    CPPUNIT_ASSERT(p.m_u64MatchLogIndex == 1);
    CPPUNIT_ASSERT(p.pendingSnapshot_ == 10);
}

void CTestRaftFixtrue::TestProgressUpdate(void)
{
    uint64_t prevM = 3;
    uint64_t prevN = 5;

    struct tmp
    {
        uint64_t update;
        uint64_t wm;
        uint64_t wn;
        bool     wok;

        tmp(uint64_t update, uint64_t wm, uint64_t wn, bool ok)
            : update(update), wm(wm), wn(wn), wok(ok)
        {
        }
    };

    vector<tmp> tests;
    // do not decrease match, next
    tests.push_back(tmp(prevM - 1, prevM, prevN, false));
    // do not decrease next
    tests.push_back(tmp(prevM, prevM, prevN, false));
    // increase match, do not decrease next
    tests.push_back(tmp(prevM + 1, prevM + 1, prevN, true));
    // increase match, next
    tests.push_back(tmp(prevM + 2, prevM + 2, prevN + 1, true));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        CProgress p(prevN, 256, &kDefaultLogger);
        p.m_u64MatchLogIndex = prevM;

        bool ok = p.MaybeUpdate(t.update);
        CPPUNIT_ASSERT_EQUAL(ok, t.wok);
        CPPUNIT_ASSERT_EQUAL(p.m_u64MatchLogIndex, t.wm);
        CPPUNIT_ASSERT_EQUAL(p.m_u64NextLogIndex, t.wn);
    }
}

void CTestRaftFixtrue::TestProgressMaybeDecr(void)
{
    struct tmp
    {
        ProgressState state;
        uint64_t m;
        uint64_t n;
        uint64_t rejected;
        uint64_t last;
        bool w;
        uint64_t wn;

        tmp(ProgressState s, uint64_t m, uint64_t n, uint64_t rejected, uint64_t last, bool w, uint64_t wn)
            : state(s), m(m), n(n), rejected(rejected), last(last), w(w), wn(wn)
        {
        }
    };

    vector<tmp> tests;
    // state replicate and rejected is not greater than match
    tests.push_back(tmp(ProgressStateReplicate, 5, 10, 5, 5, false, 10));
    // state replicate and rejected is not greater than match
    tests.push_back(tmp(ProgressStateReplicate, 5, 10, 4, 4, false, 10));
    // state replicate and rejected is greater than match
    // directly decrease to match+1
    tests.push_back(tmp(ProgressStateReplicate, 5, 10, 9, 9, true, 6));
    // next-1 != rejected is always false
    tests.push_back(tmp(ProgressStateProbe, 0, 0, 0, 0, false, 0));
    // next-1 != rejected is always false
    tests.push_back(tmp(ProgressStateProbe, 0, 10, 5, 5, false, 10));
    // next>1 = decremented by 1
    tests.push_back(tmp(ProgressStateProbe, 0, 10, 9, 9, true, 9));
    // next>1 = decremented by 1
    tests.push_back(tmp(ProgressStateProbe, 0, 2, 1, 1, true, 1));
    // next<=1 = reset to 1
    tests.push_back(tmp(ProgressStateProbe, 0, 1, 0, 0, true, 1));
    // decrease to min(rejected, last+1)
    tests.push_back(tmp(ProgressStateProbe, 0, 10, 9, 2, true, 3));
    // rejected < 1, reset to 1
    tests.push_back(tmp(ProgressStateProbe, 0, 10, 9, 0, true, 1));
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        CProgress p(t.n, 256, &kDefaultLogger);
        p.m_u64MatchLogIndex = t.m;
        p.m_statePro = t.state;

        bool g = p.maybeDecrTo(t.rejected, t.last);
        CPPUNIT_ASSERT_EQUAL(g, t.w);
        CPPUNIT_ASSERT_EQUAL(p.m_u64MatchLogIndex, t.m);
        CPPUNIT_ASSERT_EQUAL(p.m_u64NextLogIndex, t.wn);
    }
}

void CTestRaftFixtrue::TestProgressIsPaused(void)
{
    struct tmp
    {
        ProgressState state;
        bool paused;
        bool w;

        tmp(ProgressState s, bool paused, bool w)
            : state(s), paused(paused), w(w)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(ProgressStateProbe, false, false));
    tests.push_back(tmp(ProgressStateProbe, true, true));
    tests.push_back(tmp(ProgressStateReplicate, false, false));
    tests.push_back(tmp(ProgressStateReplicate, true, false));
    tests.push_back(tmp(ProgressStateSnapshot, false, true));
    tests.push_back(tmp(ProgressStateSnapshot, true, true));
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        CProgress p(0, 256, &kDefaultLogger);
        p.m_bPaused = t.paused;
        p.m_statePro = t.state;

        bool g = p.IsPaused();
        CPPUNIT_ASSERT_EQUAL(g, t.w);
    }
}

// TestProgressResume ensures that progress.MaybeUpdate and progress.maybeDecrTo
// will reset progress.paused.
void CTestRaftFixtrue::TestProgressResume(void)
{
    CProgress p(2, 256, &kDefaultLogger);
    p.m_bPaused = true;

    p.maybeDecrTo(1, 1);
    CPPUNIT_ASSERT(!(p.m_bPaused));

    p.m_bPaused = true;
    p.MaybeUpdate(2);
    CPPUNIT_ASSERT(!(p.m_bPaused));
}

// TestProgressResumeByHeartbeatResp ensures CRaft.heartbeat reset progress.paused by heartbeat response.
void CTestRaftFixtrue::TestProgressResumeByHeartbeatResp(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();
    r->m_mapProgress[2]->m_bPaused = true;
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgBeat);

        r->Step(msg);
        CPPUNIT_ASSERT(r->m_mapProgress[2]->m_bPaused);
    }

    r->m_mapProgress[2]->BecomeReplicate();
    {
        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHeartbeatResp);

        r->Step(msg);
        CPPUNIT_ASSERT(!(r->m_mapProgress[2]->m_bPaused));
    }

    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestProgressPaused(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("somedata");
        r->Step(msg);
    }
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("somedata");
        r->Step(msg);
    }
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("somedata");
        r->Step(msg);
    }

    vector<CMessage *> msgs;
    pFrame->ReadMessages(msgs);
    CPPUNIT_ASSERT(msgs.size()== 1);
    pFrame->FreeMessages(msgs);

    pFrame->Uninit();
    delete pFrame;
}

void testLeaderElection(bool prevote)
{
    ConfigFun fun = NULL;
    if (prevote)
    {
        fun = &preVoteConfig;
    }
    struct tmp
    {
        network *net;
        EStateType state;
        uint64_t expTerm;

        tmp(network *net, EStateType state, uint64_t t)
            : net(net), state(state), expTerm(t)
        {
        }
    };

    vector<tmp> tests;
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(NULL);
        tmp t(newNetworkWithConfig(fun, peers), eStateLeader, 1);
        tests.push_back(t);
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        tmp t(newNetworkWithConfig(fun, peers), eStateLeader, 1);
        tests.push_back(t);
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);
        tmp t(newNetworkWithConfig(fun, peers), eStateCandidate, 1);
        tests.push_back(t);
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);
        peers.push_back(NULL);
        tmp t(newNetworkWithConfig(fun, peers), eStateCandidate, 1);
        tests.push_back(t);
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);
        peers.push_back(NULL);
        peers.push_back(NULL);
        tmp t(newNetworkWithConfig(fun, peers), eStateLeader, 1);
        tests.push_back(t);
    }
    // three logs further along than 0, but in the same term so rejections
    // are returned instead of the votes being ignored.
    {
        vector<uint64_t> terms;
        terms.push_back(1);

        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(entsWithConfig(fun, terms));
        peers.push_back(entsWithConfig(fun, terms));
        terms.push_back(1);
        peers.push_back(entsWithConfig(fun, terms));
        peers.push_back(NULL);
        tmp t(newNetworkWithConfig(fun, peers), eStateFollower, 1);
        tests.push_back(t);
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        t.net->send(&msgs);
        CRaft *r = (CRaft*)t.net->peers[1]->data();
        EStateType expState;
        uint64_t expTerm;
        if (t.state == eStateCandidate && prevote)
        {
            // In pre-vote mode, an election that fails to complete
            // leaves the node in pre-candidate state without advancing
            // the term. 
            expState = eStatePreCandidate;
            expTerm = 0;
        }
        else
        {
            expState = t.state;
            expTerm = t.expTerm;
        }

        CPPUNIT_ASSERT_EQUAL(r->GetState(), expState);
        CPPUNIT_ASSERT_EQUAL(r->GetTerm(), expTerm);
    }
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        delete t.net;
    }
}

void CTestRaftFixtrue::TestLeaderElection(void)
{
    testLeaderElection(false);
}

void CTestRaftFixtrue::TestLeaderElectionPreVote(void)
{
    testLeaderElection(true);
}

// testLeaderCycle verifies that each node in a cluster can campaign
// and be elected in turn. This ensures that elections (including
// pre-vote) work when not starting from a clean slate (as they do in
// TestLeaderElection)
void testLeaderCycle(bool prevote)
{
    ConfigFun fun = NULL;
    if (prevote)
    {
        fun = &preVoteConfig;
    }
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetworkWithConfig(fun, peers);
    int i;
    for (i = 1; i <= 3; i++)
    {
        CMessage msg;
        msg.set_from(i);
        msg.set_to(i);
        msg.set_type(CMessage::MsgHup);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        net->send(&msgs);

        map<uint64_t, stateMachine*>::iterator iter;
        for (iter = net->peers.begin(); iter != net->peers.end(); ++iter)
        {
            stateMachine *s = iter->second;
            CRaft *r = (CRaft*)s->data();
            CPPUNIT_ASSERT(!(r->m_pConfig->m_nRaftID == i && r->GetState() != eStateLeader));
            CPPUNIT_ASSERT(!(r->m_pConfig->m_nRaftID != i && r->GetState() != eStateFollower));
        }
    }
    delete net;
}

void CTestRaftFixtrue::TestLeaderCycle(void)
{
    testLeaderCycle(false);
}

void CTestRaftFixtrue::TestLeaderCyclePreVote(void)
{
    testLeaderCycle(true);
}

void testLeaderElectionOverwriteNewerLogs(bool preVote)
{
    ConfigFun fun = NULL;
    if (preVote)
    {
        fun = &preVoteConfig;
    }
    // This network represents the results of the following sequence of
    // events:
    // - Node 1 won the election in term 1.
    // - Node 1 replicated a log entry to node 2 but died before sending
    //   it to other nodes.
    // - Node 3 won the second election in term 2.
    // - Node 3 wrote an entry to its logs but died without sending it
    //   to any other nodes.
    //
    // At this point, nodes 1, 2, and 3 all have uncommitted entries in
    // their logs and could win an election at term 3. The winner's log
    // entry overwrites the losers'. (TestLeaderSyncFollowerLog tests
    // the case where older log entries are overwritten, so this test
    // focuses on the case where the newer entries are lost).
    vector<stateMachine*> peers;
    {
        // Node 1: Won first election
        vector<uint64_t> terms;
        terms.push_back(1);
        peers.push_back(entsWithConfig(fun, terms));
    }
    {
        // Node 2: Got logs from node 1
        vector<uint64_t> terms;
        terms.push_back(1);
        peers.push_back(entsWithConfig(fun, terms));
    }
    {
        // Node 3: Won second election
        vector<uint64_t> terms;
        terms.push_back(2);
        peers.push_back(entsWithConfig(fun, terms));
    }
    {
        // Node 4: Voted but didn't get logs
        peers.push_back(votedWithConfig(fun, 3, 2));
    }
    {
        // Node 5: Voted but didn't get logs
        peers.push_back(votedWithConfig(fun, 3, 2));
    }
    network *net = newNetworkWithConfig(fun, peers);

    // Node 1 campaigns. The election fails because a quorum of nodes
    // know about the election that already happened at term 2. Node 1's
    // term is pushed ahead to 2.
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *r1 = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r1->GetState(), eStateFollower);
    CPPUNIT_ASSERT(r1->GetTerm() == 2);

    // Node 1 campaigns again with a higher term. This time it succeeds.
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(r1->GetState(), eStateLeader);
    CPPUNIT_ASSERT(r1->GetTerm() == 3);

    // Now all nodes agree on a log entry with term 1 at index 1 (and
    // term 3 at index 2).
    map<uint64_t, stateMachine*>::iterator iter;
    for (iter = net->peers.begin(); iter != net->peers.end(); ++iter)
    {
        CRaft *r = (CRaft*)iter->second->data();
        EntryVec entries;
        CRaftMemLog *pLog = dynamic_cast<CRaftMemLog *>(r->GetLog());
        pLog->allEntries(entries);
        CPPUNIT_ASSERT(entries.size() == 2);
        CPPUNIT_ASSERT(entries[0].term() == 1);
        CPPUNIT_ASSERT(entries[1].term() == 3);
    }
    delete net;
}

// TestLeaderElectionOverwriteNewerLogs tests a scenario in which a
// newly-elected leader does *not* have the newest (i.e. highest term)
// log entries, and must overwrite higher-term log entries with
// lower-term ones.
void CTestRaftFixtrue::TestLeaderElectionOverwriteNewerLogs(void)
{
    testLeaderElectionOverwriteNewerLogs(false);
}

void CTestRaftFixtrue::TestLeaderElectionOverwriteNewerLogsPreVote(void)
{
    testLeaderElectionOverwriteNewerLogs(true);
}

void testVoteFromAnyState(CMessage::EMessageType vt)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    int i;
    for (i = 0; i < (int)numStates; ++i)
    {
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;

        r->SetTerm(1);

        EStateType st = (EStateType)i;
        switch (st)
        {
        case eStateFollower:
            r->BecomeFollower(r->GetTerm(), 3);
            break;
        case eStatePreCandidate:
            r->BecomePreCandidate();
            break;
        case eStateCandidate:
            r->BecomeCandidate();
            break;
        case eStateLeader:
            r->BecomeCandidate();
            r->BecomeLeader();
            break;
        }

        // Note that setting our state above may have advanced r.Term
        // past its initial value.
        uint64_t origTerm = r->GetTerm();
        uint64_t newTerm = r->GetTerm() + 1;

        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(vt);
        msg.set_term(newTerm);
        msg.set_logterm(newTerm);
        msg.set_index(42);
        int err = r->Step(msg);

        CPPUNIT_ASSERT(err == CRaftErrNo::eOK);
        vector<CMessage*> msgs;
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        CMessage *resp = msgs[0];
        CPPUNIT_ASSERT_EQUAL(resp->type(), VoteRespMsgType(vt));
        CPPUNIT_ASSERT(!resp->reject());

        if (vt == CMessage::MsgVote)
        {
            // If this was a real vote, we reset our state and term.
            CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);
            CPPUNIT_ASSERT_EQUAL(r->GetTerm(), newTerm);
            CPPUNIT_ASSERT(r->GetVoted() == 2);
        }
        else
        {
            // In a prevote, nothing changes.
            CPPUNIT_ASSERT_EQUAL(r->GetState(), st);
            CPPUNIT_ASSERT_EQUAL(r->GetTerm(), origTerm);
            // if st == eStateFollower or eStatePreCandidate, r hasn't voted yet.
            // In eStateCandidate or eStateLeader, it's voted for itself.
            CPPUNIT_ASSERT(!(r->GetVoted() != None && r->GetVoted() != 1));
        }
        pFrame->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestVoteFromAnyState(void)
{
    testVoteFromAnyState(CMessage::MsgVote);
}

void CTestRaftFixtrue::TestPreVoteFromAnyState(void)
{
    testVoteFromAnyState(CMessage::MsgPreVote);
}

void CTestRaftFixtrue::TestLogReplication(void)
{
    struct tmp
    {
        network *net;
        vector<CMessage> msgs;
        uint64_t wcommitted;

        tmp(network *net, vector<CMessage> msgs, uint64_t w)
            : net(net), msgs(msgs), wcommitted(w)
        {
        }
    };

    vector<tmp> tests;
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(NULL);

        vector<CMessage> msgs;
        {
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgProp);
            CRaftEntry *entry = msg.add_entries();
            entry->set_data("somedata");

            msgs.push_back(msg);
        }
        tests.push_back(tmp(newNetwork(peers), msgs, 2));
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(NULL);

        vector<CMessage> msgs;
        {
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgProp);
            CRaftEntry *entry = msg.add_entries();
            entry->set_data("somedata");

            msgs.push_back(msg);
        }
        {
            CMessage msg;
            msg.set_from(1);
            msg.set_to(2);
            msg.set_type(CMessage::MsgHup);

            msgs.push_back(msg);
        }
        {
            CMessage msg;
            msg.set_from(1);
            msg.set_to(2);
            msg.set_type(CMessage::MsgProp);

            CRaftEntry *entry = msg.add_entries();
            entry->set_data("somedata");
            msgs.push_back(msg);
        }
        tests.push_back(tmp(newNetwork(peers), msgs, 4));
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        t.net->send(&msgs);

        int j;
        for (j = 0; j < t.msgs.size(); ++j)
        {
            vector<CMessage> msgs;
            msgs.push_back(t.msgs[j]);
            t.net->send(&msgs);
        }

        map<uint64_t, stateMachine*>::iterator iter;
        for (iter = t.net->peers.begin(); iter != t.net->peers.end(); ++iter)
        {
            CRaft *r = (CRaft*)iter->second->data();
            CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetCommitted(), t.wcommitted);

            EntryVec entries, ents;
            nextEnts(r, t.net->storage[iter->first], &entries);
            int m;
            for (m = 0; m < entries.size(); ++m)
            {
                if (!entries[m].data().empty())
                {
                    ents.push_back(entries[m]);
                }
            }

            vector<CMessage> props;
            for (m = 0; m < t.msgs.size(); ++m)
            {
                const CMessage& msg = t.msgs[m];
                if (msg.type() == CMessage::MsgProp)
                {
                    props.push_back(msg);
                }
            }
            for (m = 0; m < props.size(); ++m)
            {
                const CMessage& msg = props[m];
                CPPUNIT_ASSERT_EQUAL(ents[m].data(), msg.entries(0)->data());
            }
        }
    }
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        delete t.net;
    }
}

void CTestRaftFixtrue::TestSingleNodeCommit(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted()== 3);
    delete net;
}

// TestCannotCommitWithoutNewTermEntry tests the entries cannot be committed
// when leader changes, no new proposal comes in and ChangeTerm proposal is
// filtered.
void CTestRaftFixtrue::TestCannotCommitWithoutNewTermEntry(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // 0 cannot reach 2,3,4
    net->cut(1, 3);
    net->cut(1, 4);
    net->cut(1, 5);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 1);

    // network recovery
    net->recover();
    // avoid committing ChangeTerm proposal
    net->ignore(CMessage::MsgApp);

    // elect 2 as the new leader with term 2
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // no log entries from previous term should be committed
    r = (CRaft*)net->peers[2]->data();
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 1);

    net->recover();
    // send heartbeat; reset wait
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgBeat);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // append an entry at current term
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("somedata");
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 5);
    delete net;
}

// TestCommitWithoutNewTermEntry tests the entries could be committed
// when leader changes, no new proposal comes in.
void CTestRaftFixtrue::TestCommitWithoutNewTermEntry(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // 0 cannot reach 2,3,4
    net->cut(1, 3);
    net->cut(1, 4);
    net->cut(1, 5);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data("some data");
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 1);

    // network recovery
    net->recover();

    // elect 1 as the new leader with term 2
    // after append a ChangeTerm entry from the current term, all entries
    // should be committed
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 4);
    delete net;
}

void CTestRaftFixtrue::TestDuelingCandidates(void)
{
    vector<stateMachine*> peers;

    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(1, ids, 10, 1);
        peers.push_back(new raftStateMachine(pFrame));
    }
    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(2, ids, 10, 1);
        peers.push_back(new raftStateMachine(pFrame));
    }
    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(3, ids, 10, 1);
        peers.push_back(new raftStateMachine(pFrame));
    }
    network *net = newNetwork(peers);

    net->cut(1, 3);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // 1 becomes leader since it receives votes from 1 and 2
    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);

    // 3 stays as candidate since it receives a vote from 3 and a rejection from 2
    r = (CRaft*)net->peers[3]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateCandidate);

    net->recover();

    // candidate 3 now increases its term and tries to vote again
    // we expect it to disrupt the leader 1 since it has a higher term
    // 3 will be follower again since both 1 and 2 rejects its vote request since 3 does not have a long enough log
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
    {
        EntryVec entries;
        entries.push_back(CRaftEntry());
        CRaftEntry entry;
        entry.set_term(1);
        entry.set_index(1);
        entries.push_back(entry);

        s->Append(entries);
    }

    CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
    log->m_pStorage->SetCommitted(1);
    log->m_unstablePart.m_u64Offset = 2;

    struct tmp
    {
        CRaft *r;
        EStateType state;
        uint64_t term;
        CRaftMemLog* log;

        tmp(CRaft *r, EStateType state, uint64_t term, CRaftMemLog *log)
            : r(r), state(state), term(term), log(log)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp((CRaft*)peers[0]->data(), eStateFollower, 2, log));
    tests.push_back(tmp((CRaft*)peers[1]->data(), eStateFollower, 2, log));
    CRaftMemStorage *pMemStorage = new CRaftMemStorage(&kDefaultLogger);
    CRaftMemLog *pMemLog = new CRaftMemLog(pMemStorage, &kDefaultLogger);
    tests.push_back(tmp((CRaft*)peers[2]->data(), eStateFollower, 2, pMemLog));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        CPPUNIT_ASSERT_EQUAL(t.r->GetState(), t.state);
        CPPUNIT_ASSERT_EQUAL(t.r->GetTerm(), t.term);

        string base = raftLogString(t.log);
        if (net->peers[i + 1]->type() == raftType)
        {
            CRaft *r = (CRaft*)net->peers[i + 1]->data();
            string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
            CPPUNIT_ASSERT_EQUAL(base, str);
        }
    }

    delete net;
    delete s;
    delete log;
    delete pMemLog;
    delete pMemStorage;
}

void CTestRaftFixtrue::TestDuelingPreCandidates(void)
{
    vector<stateMachine*> peers;

    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(1, ids, 10, 1);
        pFrame->m_pRaftNode->m_pConfig->m_bPreVote = true;
        peers.push_back(new raftStateMachine(pFrame));
    }
    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(2, ids, 10, 1);
        pFrame->m_pRaftNode->m_pConfig->m_bPreVote = true;
        peers.push_back(new raftStateMachine(pFrame));
    }
    {
        vector<uint32_t> ids;
        ids.push_back(1);
        ids.push_back(2);
        ids.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(3, ids, 10, 1);
        pFrame->m_pRaftNode->m_pConfig->m_bPreVote = true;
        peers.push_back(new raftStateMachine(pFrame));
    }
    network *net = newNetwork(peers);

    net->cut(1, 3);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // 1 becomes leader since it receives votes from 1 and 2
    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);

    // 3 campaigns then reverts to follower when its PreVote is rejected
    r = (CRaft*)net->peers[3]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);

    net->recover();

    // candidate 3 now increases its term and tries to vote again
    // we expect it to disrupt the leader 1 since it has a higher term
    // 3 will be follower again since both 1 and 2 rejects its vote request since 3 does not have a long enough log
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
    {
        EntryVec entries;
        entries.push_back(CRaftEntry());
        CRaftEntry entry;
        entry.set_term(1);
        entry.set_index(1);
        entries.push_back(entry);

        s->Append(entries);
    }

    CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
    log->m_pStorage->SetCommitted(1);
    log->m_unstablePart.m_u64Offset = 2;

    struct tmp
    {
        CRaft *r;
        EStateType state;
        uint64_t term;
        CRaftMemLog* log;

        tmp(CRaft *r, EStateType state, uint64_t term, CRaftMemLog *log)
            : r(r), state(state), term(term), log(log)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp((CRaft*)peers[0]->data(), eStateLeader, 1, log));
    tests.push_back(tmp((CRaft*)peers[1]->data(), eStateFollower, 1, log));
    CRaftMemStorage *pMemStorage = new CRaftMemStorage(&kDefaultLogger);
    CRaftMemLog *pMemLog = new CRaftMemLog(pMemStorage, &kDefaultLogger);
    tests.push_back(tmp((CRaft*)peers[2]->data(), eStateFollower, 1, pMemLog));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        CPPUNIT_ASSERT_EQUAL(t.r->GetState(), t.state);
        CPPUNIT_ASSERT_EQUAL(t.r->GetTerm(), t.term);

        string base = raftLogString(t.log);
        if (net->peers[i + 1]->type() == raftType)
        {
            CRaft *r = (CRaft*)net->peers[i + 1]->data();
            string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
            CPPUNIT_ASSERT_EQUAL(base, str);
        }
    }

    delete net;
    delete log;
    delete pMemLog;
    delete pMemStorage;
    delete s;
}

void CTestRaftFixtrue::TestCandidateConcede(void)
{
    vector<stateMachine*> peers;

    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    net->isolate(1);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // heal the partition
    net->recover();

    // send heartbeat; reset wait
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgBeat);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    string data = "force follower";
    // send a proposal to 3 to flush out a CMessage::MsgApp to 1
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data(data);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // send heartbeat; flush out commit
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgBeat);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);
    CPPUNIT_ASSERT(r->GetTerm()== 1);

    CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
    EntryVec entries;

    entries.push_back(CRaftEntry());
    {
        CRaftEntry entry;
        entry.set_term(1);
        entry.set_index(1);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_term(1);
        entry.set_index(2);
        entry.set_data(data);
        entries.push_back(entry);
    }
    s->m_entries.clear();
    s->m_entries.insert(s->m_entries.end(), entries.begin(), entries.end());
  
    CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
    log->m_pStorage->SetCommitted(2);
    log->m_unstablePart.m_u64Offset = 3;

    string logStr = raftLogString(log);

    map<uint64_t, stateMachine*>::iterator iter;
    for (iter = net->peers.begin(); iter != net->peers.end(); ++iter)
    {
        stateMachine *s = iter->second;
        if (s->type() != raftType)
        {
            continue;
        }
        CRaft *r = (CRaft*)s->data();
        string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
        CPPUNIT_ASSERT_EQUAL(str, logStr);
    }
    delete log;
    delete s;
    delete net;
}

void CTestRaftFixtrue::TestSingleNodeCandidate(void)
{
    vector<stateMachine*> peers;

    peers.push_back(NULL);
    network *net = newNetwork(peers);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);
    delete net;
}

void CTestRaftFixtrue::TestSingleNodePreCandidate(void)
{
    vector<stateMachine*> peers;

    peers.push_back(NULL);

    network *net = newNetworkWithConfig(preVoteConfig, peers);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CRaft *r = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);
    delete net;
}

void CTestRaftFixtrue::TestOldMessages(void)
{
    vector<stateMachine*> peers;

    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    // make 0 leader @ term 3
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // pretend we're an old leader trying to make progress; this entry is expected to be ignored.
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_term(2);
        msg.set_type(CMessage::MsgApp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_index(3);
        entry->set_term(2);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // commit a new entry
    string data = "somedata";
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data(data);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
    EntryVec entries;

    entries.push_back(CRaftEntry());
    {
        CRaftEntry entry;
        entry.set_term(1);
        entry.set_index(1);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_term(2);
        entry.set_index(2);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_term(3);
        entry.set_index(3);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_term(3);
        entry.set_index(4);
        entry.set_data(data);
        entries.push_back(entry);
    }
    s->m_entries.clear();
    s->m_entries.insert(s->m_entries.end(), entries.begin(), entries.end());

    CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
    log->m_pStorage->SetCommitted(4);
    log->m_unstablePart.m_u64Offset = 5;

    string logStr = raftLogString(log);

    map<uint64_t, stateMachine*>::iterator iter;
    for (iter = net->peers.begin(); iter != net->peers.end(); ++iter)
    {
        stateMachine *s = iter->second;
        if (s->type() != raftType)
        {
            continue;
        }
        CRaft *r = (CRaft*)s->data();
        string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
        CPPUNIT_ASSERT_EQUAL(str, logStr);
    }
    delete log;
    delete s;
    delete net;
}

void CTestRaftFixtrue::TestProposal(void)
{
    struct tmp
    {
        network *net;
        bool success;

        tmp(network *net, bool success)
            : net(net), success(success)
        {
        }

        ~tmp(void)
        {
           // delete net;
        }
    };

    vector<tmp> tests;
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(NULL);

        network *net = newNetwork(peers);
        tests.push_back(tmp(net, true));
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(nopStepper);

        network *net = newNetwork(peers);
        tests.push_back(tmp(net, true));
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);

        network *net = newNetwork(peers);
        tests.push_back(tmp(net, false));
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);
        peers.push_back(NULL);

        network *net = newNetwork(peers);
        tests.push_back(tmp(net, false));
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(nopStepper);
        peers.push_back(nopStepper);
        peers.push_back(NULL);
        peers.push_back(NULL);

        network *net = newNetwork(peers);
        tests.push_back(tmp(net, true));
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        string data = "somedata";

        // promote 0 the leader
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgHup);
            msgs.push_back(msg);
            t.net->send(&msgs);
        }
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgProp);
            CRaftEntry *entry = msg.add_entries();
            entry->set_data(data);
            msgs.push_back(msg);
            t.net->send(&msgs);
        }

        string logStr = "";
        if (t.success)
        {
            CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
            EntryVec entries;

            entries.push_back(CRaftEntry());
            {
                CRaftEntry entry;
                entry.set_term(1);
                entry.set_index(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_term(1);
                entry.set_index(2);
                entry.set_data(data);
                entries.push_back(entry);
            }
            s->m_entries.clear();
            s->m_entries.insert(s->m_entries.end(), entries.begin(), entries.end());

            CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
            log->m_pStorage->SetCommitted(2);
            log->m_unstablePart.m_u64Offset = 3;
            logStr = raftLogString(log);
            delete log;
            delete s;
        }
        else
            logStr = "committed: 0\napplied: 0\nentries size: 0\n";

        map<uint64_t, stateMachine*>::iterator iter;
        for (iter = t.net->peers.begin(); iter != t.net->peers.end(); ++iter)
        {
            stateMachine *s = iter->second;
            if (s->type() != raftType)
            {
                continue;
            }
            CRaft *r = (CRaft*)s->data();
            string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
            CPPUNIT_ASSERT_EQUAL(str, logStr);
        }
    }
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        delete t.net;
    }
}

void CTestRaftFixtrue::TestProposalByProxy(void)
{
    vector<network*> tests;

    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(NULL);

        network *net = newNetwork(peers);
        tests.push_back(net);
    }
    {
        vector<stateMachine*> peers;
        peers.push_back(NULL);
        peers.push_back(NULL);
        peers.push_back(nopStepper);

        network *net = newNetwork(peers);
        tests.push_back(net);
    }

    int i;
    string data = "somedata";
    for (i = 0; i < tests.size(); ++i)
    {
        network *net = tests[i];
        // promote 0 the leader
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgHup);
            msgs.push_back(msg);
            net->send(&msgs);
        }
        // propose via follower
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(2);
            msg.set_to(2);
            msg.set_type(CMessage::MsgProp);
            CRaftEntry *entry = msg.add_entries();
            entry->set_data(data);
            msgs.push_back(msg);
            net->send(&msgs);
        }

        string logStr = "";
        CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
        EntryVec entries;

        entries.push_back(CRaftEntry());
        {
            CRaftEntry entry;
            entry.set_term(1);
            entry.set_index(1);
            entries.push_back(entry);
        }
        {
            CRaftEntry entry;
            entry.set_term(1);
            entry.set_index(2);
            entry.set_data(data);
            entries.push_back(entry);
        }
        s->m_entries.clear();
        s->m_entries.insert(s->m_entries.end(), entries.begin(), entries.end());

        CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
        log->m_pStorage->SetCommitted(2);
        log->m_unstablePart.m_u64Offset = 3;
        logStr = raftLogString(log);

        map<uint64_t, stateMachine*>::iterator iter;
        for (iter = net->peers.begin(); iter != net->peers.end(); ++iter)
        {
            stateMachine *s = iter->second;
            if (s->type() != raftType)
            {
                continue;
            }
            CRaft *r = (CRaft*)s->data();
            string str = raftLogString(dynamic_cast<CRaftMemLog*> (r->GetLog()));
            CPPUNIT_ASSERT_EQUAL(str, logStr);
        }
        CPPUNIT_ASSERT(((CRaft*)(net->peers[1]->data()))->GetTerm() == 1);
        delete log;
        delete s;
    }
    for (i = 0; i < tests.size(); ++i)
    {
        network *net = tests[i];
        delete net;
    }
}

void CTestRaftFixtrue::TestCommit(void)
{
    struct tmp
    {
        vector<uint64_t> matches;
        vector<CRaftEntry> logs;
        uint64_t term;
        uint64_t w;

        tmp(vector<uint64_t> matches, vector<CRaftEntry> entries, uint64_t term, uint64_t w)
            : matches(matches), logs(entries), term(term), w(w)
        {
        }
    };

    vector<tmp> tests;
    // single
    {
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 1, 1));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 2));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 1));
        }
    }
    // odd
    {
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 1, 1));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 2));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
    }
    // even
    {
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);
            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 1, 1));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);
            matches.push_back(1);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 1, 1));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(1);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(2);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(2);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 2));
        }
        {
            vector<uint64_t> matches;
            vector<CRaftEntry> entries;

            matches.push_back(2);
            matches.push_back(1);
            matches.push_back(2);
            matches.push_back(2);

            {
                CRaftEntry entry;
                entry.set_index(1);
                entry.set_term(1);
                entries.push_back(entry);
            }
            {
                CRaftEntry entry;
                entry.set_index(2);
                entry.set_term(1);
                entries.push_back(entry);
            }

            tests.push_back(tmp(matches, entries, 2, 0));
        }
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        vector<uint32_t> peers;
        peers.push_back(1);
        
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1, t.logs, t.term);
        CRaft *r = pFrame->m_pRaftNode;

        int j;
        for (j = 0; j < t.matches.size(); ++j)
        {
            r->ResetProgress(j + 1, t.matches[j], t.matches[j] + 1);
        }
        r->MaybeCommit();
        CPPUNIT_ASSERT(r->GetLog()->GetCommitted() ==  t.w);
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestPastElectionTimeout(void)
{
    struct tmp
    {
        int elapse;
        float probability;
        bool round;

        tmp(int elapse, float probability, bool round)
            : elapse(elapse), probability(probability), round(round)
        {
        }
    };
    vector<tmp> tests;

    tests.push_back(tmp(5, 0, false));
    tests.push_back(tmp(10,float(0.1), true));
    tests.push_back(tmp(13, float(0.4), true));
    tests.push_back(tmp(15, float(0.6), true));
    tests.push_back(tmp(18, float(0.9), true));
    tests.push_back(tmp(20, 1, false));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];

        vector<uint32_t> peers;
        peers.push_back(1);

        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;
        r->m_nTicksElectionElapsed = t.elapse;
        int c = 0, j;
        for (j = 0; j < 10000; ++j)
        {
            r->ResetRandomizedElectionTimeout();
            if (r->PastElectionTimeout())
            {
                ++c;
            }
        }

        float g = (float)c / float(10000.0);
        if (t.round)
        {
            g = floor(g * 10 + float(0.5)) /float( 10.0);
        }

        CPPUNIT_ASSERT_EQUAL(g, t.probability);
        pFrame->Uninit();
        delete pFrame;
    }
}

// TestHandleMsgApp ensures:
// 1. Reply false if log doesn’t contain an entry at prevLogIndex whose term matches prevLogTerm.
// 2. If an existing entry conflicts with a new one (same index but different terms),
//    delete the existing entry and all that follow it; append any new entries not already in the log.
// 3. If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry).
void CTestRaftFixtrue::TestHandleMsgApp(void)
{
    struct tmp
    {
        CMessage m;
        uint64_t index;
        uint64_t commit;
        bool reject;

        tmp(CMessage m, uint64_t index, uint64_t commit, bool reject)
            : m(m), index(index), commit(commit), reject(reject)
        {
        }
    };

    vector<tmp> tests;
    // Ensure 1

    // previous log mismatch
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(3);
        msg.set_index(2);
        msg.set_commit(3);

        tests.push_back(tmp(msg, 2, 0, true));
    }
    // previous log non-exist
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(3);
        msg.set_index(3);
        msg.set_commit(3);

        tests.push_back(tmp(msg, 2, 0, true));
    }

    // Ensure 2
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(1);
        msg.set_index(1);
        msg.set_commit(1);

        tests.push_back(tmp(msg, 2, 1, false));
    }
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(0);
        msg.set_index(0);
        msg.set_commit(1);

        CRaftEntry *entry = msg.add_entries();
        entry->set_index(1);
        entry->set_term(2);

        tests.push_back(tmp(msg, 1, 1, false));
    }
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(2);
        msg.set_index(2);
        msg.set_commit(3);
        vector<CRaftEntry> vecEntries;
        CRaftEntry entry;
        entry.set_index(3);
        entry.set_term(2);
        vecEntries.push_back(entry);
        entry.set_index(4);
        entry.set_term(2);
        vecEntries.push_back(entry);
        msg.set_entries(vecEntries);
        tests.push_back(tmp(msg, 4, 3, false));
    }
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(2);
        msg.set_index(2);
        msg.set_commit(4);

        CRaftEntry *entry = msg.add_entries();
        entry->set_index(3);
        entry->set_term(2);

        tests.push_back(tmp(msg, 3, 3, false));
    }
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(1);
        msg.set_index(1);
        msg.set_commit(4);

        CRaftEntry *entry = msg.add_entries();
        entry->set_index(2);
        entry->set_term(2);

        tests.push_back(tmp(msg, 2, 2, false));
    }

    // Ensure 3

    // match entry 1, commit up to last new entry 1
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(1);
        msg.set_logterm(1);
        msg.set_index(1);
        msg.set_commit(3);

        tests.push_back(tmp(msg, 2, 1, false));
    }
    // match entry 1, commit up to last new entry 2
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(1);
        msg.set_logterm(1);
        msg.set_index(1);
        msg.set_commit(3);

        CRaftEntry *entry = msg.add_entries();
        entry->set_index(2);
        entry->set_term(2);
        tests.push_back(tmp(msg, 2, 2, false));
    }
    // match entry 2, commit up to last new entry 2
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(2);
        msg.set_index(2);
        msg.set_commit(3);

        tests.push_back(tmp(msg, 2, 2, false));
    }
    {
        CMessage msg;
        msg.set_type(CMessage::MsgApp);
        msg.set_term(2);
        msg.set_logterm(2);
        msg.set_index(2);
        msg.set_commit(4);

        tests.push_back(tmp(msg, 2, 2, false));
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];

        vector<CRaftEntry> entries;
        {
            CRaftEntry entry;
            entry.set_index(1);
            entry.set_term(1);
            entries.push_back(entry);
        }
        {
            CRaftEntry entry;
            entry.set_index(2);
            entry.set_term(2);
            entries.push_back(entry);
        }

        vector<uint32_t> peers;
        peers.push_back(1);

        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1, entries);
        CRaft *r = pFrame->m_pRaftNode;

        r->OnAppendEntries(t.m);

        CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetLastIndex(), t.index);
        CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetCommitted(), t.commit);

        vector<CMessage*> msgs;
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->reject(), t.reject);
        pFrame->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
    }
}

// TestHandleHeartbeat ensures that the follower commits to the commit in the message.
void CTestRaftFixtrue::TestHandleHeartbeat(void)
{
    uint64_t commit = 2;
    struct tmp
    {
        CMessage m;
        uint64_t commit;

        tmp(CMessage m, uint64_t commit)
            : m(m), commit(commit)
        {
        }
    };

    vector<tmp> tests;
    {
        CMessage m;
        m.set_from(2);
        m.set_to(1);
        m.set_type(CMessage::MsgHeartbeat);
        m.set_term(2);
        m.set_commit(commit + 1);
        tests.push_back(tmp(m, commit + 1));
    }
    // do not decrease commit
    {
        CMessage m;
        m.set_from(2);
        m.set_to(1);
        m.set_type(CMessage::MsgHeartbeat);
        m.set_term(2);
        m.set_commit(commit - 1);
        tests.push_back(tmp(m, commit));
    }

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];

        vector<CRaftEntry> entries;
        {
            CRaftEntry entry;
            entry.set_index(1);
            entry.set_term(1);
            entries.push_back(entry);
        }
        {
            CRaftEntry entry;
            entry.set_index(2);
            entry.set_term(2);
            entries.push_back(entry);
        }
        {
            CRaftEntry entry;
            entry.set_index(3);
            entry.set_term(3);
            entries.push_back(entry);
        }

        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);

        CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1, entries);
        CRaft *r = pFrame->m_pRaftNode;

        r->BecomeFollower(2, 2);
        r->GetLog()->CommitTo(commit);
        r->OnHeartbeat(t.m);

        CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetCommitted(), t.commit);
        vector<CMessage *> msgs;
        pFrame->ReadMessages(msgs);

        CPPUNIT_ASSERT(msgs.size() == 1);

        CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgHeartbeatResp);
        pFrame->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
    }
}

// TestHandleHeartbeatResp ensures that we re-send log entries when we get a heartbeat response.
void CTestRaftFixtrue::TestHandleHeartbeatResp(void)
{
    vector<CRaftEntry> entries;
    {
        CRaftEntry entry;
        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_index(2);
        entry.set_term(2);
        entries.push_back(entry);
    }
    {
        CRaftEntry entry;
        entry.set_index(3);
        entry.set_term(3);
        entries.push_back(entry);
    }

    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1, entries);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    r->GetLog()->CommitTo(r->GetLog()->GetLastIndex());

    vector<CMessage*> msgs;
    // A heartbeat response from a node that is behind; re-send CMessage::MsgApp
    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgHeartbeatResp);
        r->Step(m);

        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size()== 1);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgApp);
        pFrame->FreeMessages(msgs);
    }

    // A second heartbeat response generates another CMessage::MsgApp re-send
    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgHeartbeatResp);
        r->Step(m);
        msgs.clear();
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgApp);
    }
    // Once we have an CMessage::MsgAppResp, heartbeats no longer send CMessage::MsgApp.
    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgAppResp);
        m.set_index(msgs[0]->index() + uint64_t(msgs[0]->entries_size()));
        r->Step(m);
        //vector<CMessage*> msgs;
        // Consume the message sent in response to CMessage::MsgAppResp
        pFrame->ReadMessages(msgs);
        pFrame->FreeMessages(msgs);
    }

    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgHeartbeatResp);
        r->Step(m);
        msgs.clear();
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 0);
        pFrame->FreeMessages(msgs);
    }
    pFrame->Uninit();
    delete pFrame;
}

// TestRaftFreesReadOnlyMem ensures CRaft will free read request from
// readOnly readIndexQueue and pendingReadIndex map.
// related issue: https://github.com/coreos/etcd/issues/7571
void CTestRaftFixtrue::TestRaftFreesReadOnlyMem(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    r->GetLog()->CommitTo(r->GetLog()->GetLastIndex());

    string ctx = "ctx";
    vector<CMessage*> msgs;

    // leader starts linearizable read request.
    // more info: CRaft dissertation 6.4, Step 2.
    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgReadIndex);
        CRaftEntry *entry = m.add_entries();
        entry->set_data(ctx);
        r->Step(m);

        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size()== 1);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgHeartbeat);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->context(), ctx);
        CPPUNIT_ASSERT(r->m_pReadOnly->m_listRead.size() == 1);
        CPPUNIT_ASSERT(r->m_pReadOnly->m_mapPendingReadIndex.size() ==  1);
        CPPUNIT_ASSERT(r->m_pReadOnly->m_mapPendingReadIndex.find(ctx) != r->m_pReadOnly->m_mapPendingReadIndex.end());
        pFrame->FreeMessages(msgs);
    }
    // heartbeat responses from majority of followers (1 in this case)
    // acknowledge the authority of the leader.
    // more info: CRaft dissertation 6.4, Step 3.
    {
        CMessage m;
        m.set_from(2);
        m.set_type(CMessage::MsgHeartbeatResp);
        m.set_context(ctx);
        r->Step(m);

        CPPUNIT_ASSERT(r->m_pReadOnly->m_listRead.empty());
        CPPUNIT_ASSERT(r->m_pReadOnly->m_mapPendingReadIndex.empty());
        CPPUNIT_ASSERT(r->m_pReadOnly->m_mapPendingReadIndex.find(ctx) == r->m_pReadOnly->m_mapPendingReadIndex.end());
    }
    pFrame->Uninit();
    delete pFrame;
}

// TestMsgAppRespWaitReset verifies the resume behavior of a leader
// CMessage::MsgAppResp.
void CTestRaftFixtrue::TestMsgAppRespWaitReset(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    vector<CMessage*> msgs;

    // The new leader has just emitted a new Term 4 entry; consume those messages
    // from the outgoing queue.
    r->BcastAppend();
    pFrame->ReadMessages(msgs);

    // Node 2 acks the first entry, making it committed.
    {
        CMessage msg;
        msg.set_from(2);
        msg.set_type(CMessage::MsgAppResp);
        msg.set_index(1);
        r->Step(msg);
    }
    CPPUNIT_ASSERT(r->GetLog()->GetCommitted() == 1);

    // Also consume the CMessage::MsgApp messages that update Commit on the followers.
    pFrame->ReadMessages(msgs);

    // A new command is now proposed on node 1.
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *entry = msg.add_entries();
        r->Step(msg);
    }

    // The command is broadcast to all nodes not in the wait state.
    // Node 2 left the wait state due to its CMessage::MsgAppResp, but node 3 is still waiting.
    pFrame->ReadMessages(msgs);
    CPPUNIT_ASSERT(msgs.size() == 1);
    CMessage *msg = msgs[0];
    CPPUNIT_ASSERT(!(msg->type() != CMessage::MsgApp || msg->to() != 2));
    CPPUNIT_ASSERT(!(msg->entries_size() != 1 || msg->entries(0)->index() != 2));

    // Now Node 3 acks the first entry. This releases the wait and entry 2 is sent.
    {
        CMessage msg;
        msg.set_from(3);
        msg.set_type(CMessage::MsgAppResp);
        msg.set_index(1);
        r->Step(msg);
    }

    pFrame->ReadMessages(msgs);
    CPPUNIT_ASSERT(msgs.size()== 1);
    msg = msgs[0];
    CPPUNIT_ASSERT(!(msg->type() != CMessage::MsgApp || msg->to() != 3));
    CPPUNIT_ASSERT(!(msg->entries_size() != 1 || msg->entries(0)->index() != 2));
    pFrame->FreeMessages(msgs);
    pFrame->Uninit();
    delete pFrame;
}

void testRecvMsgVote(CMessage::EMessageType type)
{
    struct tmp
    {
        EStateType state;
        uint64_t i, term;
        uint32_t voteFor;
        bool reject;

        tmp(EStateType t, uint64_t i, uint64_t term, uint32_t vote, bool reject)
            : state(t), i(i), term(term), voteFor(vote), reject(reject)
        {
        }
    };

    vector<tmp> tests;

    tests.push_back(tmp(eStateFollower, 0, 0, None, true));
    tests.push_back(tmp(eStateFollower, 0, 1, None, true));
    tests.push_back(tmp(eStateFollower, 0, 2, None, true));
    tests.push_back(tmp(eStateFollower, 0, 3, None, false));

    tests.push_back(tmp(eStateFollower, 1, 0, None, true));
    tests.push_back(tmp(eStateFollower, 1, 1, None, true));
    tests.push_back(tmp(eStateFollower, 1, 2, None, true));
    tests.push_back(tmp(eStateFollower, 1, 3, None, false));

    tests.push_back(tmp(eStateFollower, 2, 0, None, true));
    tests.push_back(tmp(eStateFollower, 2, 1, None, true));
    tests.push_back(tmp(eStateFollower, 2, 2, None, false));
    tests.push_back(tmp(eStateFollower, 2, 3, None, false));

    tests.push_back(tmp(eStateFollower, 3, 0, None, true));
    tests.push_back(tmp(eStateFollower, 3, 1, None, true));
    tests.push_back(tmp(eStateFollower, 3, 2, None, false));
    tests.push_back(tmp(eStateFollower, 3, 3, None, false));

    tests.push_back(tmp(eStateFollower, 3, 2, 2, false));
    tests.push_back(tmp(eStateFollower, 3, 2, 1, true));

    tests.push_back(tmp(eStateLeader, 3, 3, 1, true));
    tests.push_back(tmp(eStatePreCandidate, 3, 3, 1, true));
    tests.push_back(tmp(eStateCandidate, 3, 3, 1, true));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        vector<uint32_t> peers;
        peers.push_back(1);
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;

        r->SetState(t.state);

        r->SetVoted(t.voteFor);

        CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
        EntryVec entries;

        entries.push_back(CRaftEntry());
        {
            CRaftEntry entry;
            entry.set_term(2);
            entry.set_index(1);
            entries.push_back(entry);
        }
        {
            CRaftEntry entry;
            entry.set_term(2);
            entry.set_index(2);
            entries.push_back(entry);
        }
        s->m_entries.clear();
        s->m_entries.insert(s->m_entries.end(), entries.begin(), entries.end());

        CRaftMemLog *log = new CRaftMemLog(s, &kDefaultLogger);
        log->m_unstablePart.m_u64Offset = 3;
        r->m_pRaftLog = log;
        {
            CMessage msg;
            msg.set_type(type);
            msg.set_from(2);
            msg.set_index(t.i);
            msg.set_logterm(t.term);
            r->Step(msg);
        }

        vector<CMessage*> msgs;
        pFrame->ReadMessages(msgs);

        CPPUNIT_ASSERT(msgs.size() == 1);
        CPPUNIT_ASSERT(msgs[0]->type() == VoteRespMsgType(type));
        CPPUNIT_ASSERT_EQUAL(msgs[0]->reject(), t.reject);
        pFrame->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
        delete log;
        delete s;
    }
}

void CTestRaftFixtrue::TestRecvMsgVote(void)
{
    testRecvMsgVote(CMessage::MsgVote);
}

// TODO
void CTestRaftFixtrue::TestStateTransition(void)
{
    struct tmp
    {
    };
}

void CTestRaftFixtrue::TestAllServerStepdown(void)
{
    struct tmp
    {
        EStateType state, wstate;
        uint64_t term, index;

        tmp(EStateType s, EStateType ws, uint64_t t, uint64_t i)
            : state(s), wstate(ws), term(t), index(i)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(eStateFollower, eStateFollower, 3, 0));
    tests.push_back(tmp(eStatePreCandidate, eStateFollower, 3, 0));
    tests.push_back(tmp(eStateCandidate, eStateFollower, 3, 0));
    tests.push_back(tmp(eStateLeader, eStateFollower, 3, 1));

    vector<CMessage::EMessageType> types;
    types.push_back(CMessage::MsgVote);
    types.push_back(CMessage::MsgApp);
    uint64_t term = 3;

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;

        switch (t.state)
        {
        case eStateFollower:
            r->BecomeFollower(1, None);
            break;
        case eStatePreCandidate:
            r->BecomePreCandidate();
            break;
        case eStateCandidate:
            r->BecomeCandidate();
            break;
        case eStateLeader:
            r->BecomeCandidate();
            r->BecomeLeader();
            break;
        }

        int j;
        for (j = 0; j < types.size(); ++j)
        {
            CMessage::EMessageType type = types[j];
            CMessage msg;
            msg.set_from(2);
            msg.set_type(type);
            msg.set_term(term);
            msg.set_logterm(term);
            r->Step(msg);

            CPPUNIT_ASSERT_EQUAL(r->GetState(), t.wstate);
            CPPUNIT_ASSERT_EQUAL(r->GetTerm(), t.term);
            CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetLastIndex(), t.index);
            EntryVec entries;
            (dynamic_cast<CRaftMemLog*>(r->GetLog()))->allEntries(entries);
            CPPUNIT_ASSERT_EQUAL(entries.size(), t.index);

            uint64_t leader = 2;
            if (type == CMessage::MsgVote)
            {
                leader = None;
            }
            CPPUNIT_ASSERT(r->GetLeader() == leader);
        }
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestLeaderStepdownWhenQuorumActive(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);
    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->m_pConfig->m_bCheckQuorum = true;

    r->BecomeCandidate();
    r->BecomeLeader();

    int i;
    for (i = 0; i < r->m_pConfig->m_nTicksElection + 1; ++i)
    {
        CMessage msg;
        msg.set_from(2);
        msg.set_type(CMessage::MsgHeartbeatResp);
        msg.set_term(r->GetTerm());
        r->Step(msg);
        r->OnTick();
    }

    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestLeaderStepdownWhenQuorumLost(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->m_pConfig->m_bCheckQuorum = true;

    r->BecomeCandidate();
    r->BecomeLeader();

    int i;
    for (i = 0; i < r->m_pConfig->m_nTicksElection + 1; ++i)
    {
        r->OnTick();
    }

    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestLeaderSupersedingWithCheckQuorum(void)
{
    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    vector<stateMachine*> peers;
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);

        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pConfig->m_bCheckQuorum = true;
    }
    peers.push_back(new raftStateMachine(aFrame));
    peers.push_back(new raftStateMachine(bFrame));
    peers.push_back(new raftStateMachine(cFrame));
    network *net = newNetwork(peers);

    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 1;

    int i;
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateFollower);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    // Peer b rejected c's vote since its electionElapsed had not reached to electionTimeout
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateCandidate);

    // Letting b's electionElapsed reach to electionTimeout
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateLeader);
    delete net;
}

void CTestRaftFixtrue::TestLeaderElectionWithCheckQuorum(void)
{
    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    vector<stateMachine*> peers;

    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);

        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);

        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pConfig->m_bCheckQuorum = true;
    }
    peers.push_back(new raftStateMachine(aFrame));
    peers.push_back(new raftStateMachine(bFrame));
    peers.push_back(new raftStateMachine(cFrame));
    network *net = newNetwork(peers);

    a->m_nTicksRandomizedElectionTimeout = a->m_pConfig->m_nTicksElection + 1;
    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 2;

    int i;
    // Immediately after creation, votes are cast regardless of the
    // election timeout.
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateFollower);

    // need to reset randomizedElectionTimeout larger than electionTimeout again,
    // because the value might be reset to electionTimeout since the last state changes
    a->m_nTicksRandomizedElectionTimeout = a->m_pConfig->m_nTicksElection + 1;
    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 2;

    for (i = 0; i < a->m_pConfig->m_nTicksElection; ++i)
    {
        a->OnTick();
    }
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateFollower);
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateLeader);
    delete net;
}

// TestFreeStuckCandidateWithCheckQuorum ensures that a candidate with a higher term
// can disrupt the leader even if the leader still "officially" holds the lease, The
// leader is expected to Step down and adopt the candidate's term
void CTestRaftFixtrue::TestFreeStuckCandidateWithCheckQuorum(void)
{
    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    vector<stateMachine*> peers;

    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);

        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pConfig->m_bCheckQuorum = true;
    }
    peers.push_back(new raftStateMachine(aFrame));
    peers.push_back(new raftStateMachine(bFrame));
    peers.push_back(new raftStateMachine(cFrame));
    network *net = newNetwork(peers);

    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 2;
    int i;
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(1);

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(b->GetState(), eStateFollower);
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateCandidate);
    CPPUNIT_ASSERT_EQUAL(c->GetTerm(), b->GetTerm() + 1);

    // Vote again for safety
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(3);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(b->GetState(), eStateFollower);
    CPPUNIT_ASSERT_EQUAL(c->GetState(), eStateCandidate);
    CPPUNIT_ASSERT_EQUAL(c->GetTerm(), b->GetTerm() + 2);

    net->recover();
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(3);
        msg.set_type(CMessage::MsgHeartbeat);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // Disrupt the leader so that the stuck peer is freed
    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateFollower);
    CPPUNIT_ASSERT_EQUAL(a->GetTerm(), c->GetTerm());
    delete net;
}

void CTestRaftFixtrue::TestNonPromotableVoterWithCheckQuorum(void)
{
    CTestRaftFrame *aFrame, *bFrame;
    CRaft *a, *b;
    vector<stateMachine*> peers;

    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);

        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pConfig->m_bCheckQuorum = true;
    }
    peers.push_back(new raftStateMachine(aFrame));
    peers.push_back(new raftStateMachine(bFrame));
    network *net = newNetwork(peers);

    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 1;
    // Need to remove 2 again to make it a non-promotable node since newNetwork overwritten some internal states
    b->DelProgress(2);
    CPPUNIT_ASSERT(!(b->IsPromotable()));
    int i;
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);
    CPPUNIT_ASSERT_EQUAL(b->GetState(), eStateFollower);
    CPPUNIT_ASSERT(b->GetLeader() == 1);
    delete net;
}

void CTestRaftFixtrue::TestReadOnlyOptionSafe(void)
{
    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    vector<stateMachine*> peers;

    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pConfig->m_bCheckQuorum = true;
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pConfig->m_bCheckQuorum = true;
    }
    peers.push_back(new raftStateMachine(aFrame));
    peers.push_back(new raftStateMachine(bFrame));
    peers.push_back(new raftStateMachine(cFrame));
    network *net = newNetwork(peers);

    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 2;
    int i;
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);
    struct tmp
    {
        CTestRaftFrame *r;
        int proposals;
        uint64_t wri;
        string ctx;

        tmp(CTestRaftFrame *r, int proposals, uint64_t wri, string ctx)
            : r(r), proposals(proposals), wri(wri), ctx(ctx)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(aFrame, 10, 11, "ctx1"));
    tests.push_back(tmp(bFrame, 10, 21, "ctx2"));
    tests.push_back(tmp(cFrame, 10, 31, "ctx3"));
    tests.push_back(tmp(aFrame, 10, 41, "ctx4"));
    tests.push_back(tmp(bFrame, 10, 51, "ctx5"));
    tests.push_back(tmp(cFrame, 10, 61, "ctx6"));

    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        int j;
        for (j = 0; j < t.proposals; ++j)
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgProp);
            msg.add_entries();
            msgs.push_back(msg);
            net->send(&msgs);
        }
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(t.r->m_pConfig->m_nRaftID);
            msg.set_to(t.r->m_pConfig->m_nRaftID);
            msg.set_type(CMessage::MsgReadIndex);
            CRaftEntry *entry = msg.add_entries();
            entry->set_data(t.ctx);
            msgs.push_back(msg);
            net->send(&msgs);
        }

        CTestRaftFrame *rFrame = t.r;
        vector<CLogOperation*> opts;
        rFrame->ReadLogOpt(opts);
        CPPUNIT_ASSERT(!opts.empty());
        CReadState *state = opts[0]->m_pOperation;
        CPPUNIT_ASSERT(state->m_u64Index == t.wri);
        CPPUNIT_ASSERT(state->m_strRequestCtx == t.ctx);
        rFrame->FreeLogOpt(opts);
    }
    delete net;
}

void CTestRaftFixtrue::TestReadOnlyOptionLease(void)
{
    vector<uint32_t> peers;
    vector<stateMachine*> sts;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    {
        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;
        a->m_pConfig->m_bCheckQuorum = true;
        sts.push_back(new raftStateMachine(aFrame));
    }
    {
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;
        b->m_pConfig->m_bCheckQuorum = true;
        sts.push_back(new raftStateMachine(bFrame));
    }
    {
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;

        c->m_pConfig->m_bCheckQuorum = true;
        sts.push_back(new raftStateMachine(cFrame));
    }

    network *net = newNetwork(sts);
    b->m_nTicksRandomizedElectionTimeout = b->m_pConfig->m_nTicksElection + 1;
    int i;
    for (i = 0; i < b->m_pConfig->m_nTicksElection; ++i)
    {
        b->OnTick();
    }

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);

    struct tmp
    {
        CTestRaftFrame *r;
        int proposals;
        uint64_t wri;
        string wctx;

        tmp(CTestRaftFrame *r, int proposals, uint64_t wri, string ctx)
            : r(r), proposals(proposals), wri(wri), wctx(ctx)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(aFrame, 10, 11, "ctx1"));
    tests.push_back(tmp(bFrame, 10, 21, "ctx2"));
    tests.push_back(tmp(cFrame, 10, 31, "ctx3"));
    tests.push_back(tmp(aFrame, 10, 41, "ctx4"));
    tests.push_back(tmp(bFrame, 10, 51, "ctx5"));
    tests.push_back(tmp(cFrame, 10, 61, "ctx6"));

    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        int j;
        for (j = 0; j < t.proposals; ++j)
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgProp);
            msg.add_entries();
            msgs.push_back(msg);
            net->send(&msgs);
        }
        {
            vector<CMessage> msgs;
            CMessage msg;
            msg.set_from(t.r->m_pConfig->m_nRaftID);
            msg.set_to(t.r->m_pConfig->m_nRaftID);
            msg.set_type(CMessage::MsgReadIndex);
            msg.add_entries()->set_data(t.wctx);
            msgs.push_back(msg);
            net->send(&msgs);
        }

        CTestRaftFrame *rFrame = t.r;
        vector<CLogOperation*> opts;
        rFrame->ReadLogOpt(opts);

        CReadState *s = opts[0]->m_pOperation;
        CPPUNIT_ASSERT(s->m_u64Index == t.wri);
        CPPUNIT_ASSERT(s->m_strRequestCtx == t.wctx);
        rFrame->FreeLogOpt(opts);
    }
    delete net;
}

void CTestRaftFixtrue::TestReadOnlyOptionLeaseWithoutCheckQuorum(void)
{
    vector<uint32_t> peers;
    vector<stateMachine*> sts;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    {
        aFrame = newTestRaft(1, peers, 10, 1);
        a = aFrame->m_pRaftNode;
        a->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;
        sts.push_back(new raftStateMachine(aFrame));
    }
    {
        bFrame = newTestRaft(2, peers, 10, 1);
        b = bFrame->m_pRaftNode;
        b->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;
        sts.push_back(new raftStateMachine(bFrame));
    }
    {
        cFrame = newTestRaft(3, peers, 10, 1);
        c = cFrame->m_pRaftNode;
        c->m_pReadOnly->m_optMode = ReadOnlyLeaseBased;
        sts.push_back(new raftStateMachine(cFrame));
    }

    network *net = newNetwork(sts);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    string ctx = "ctx1";

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgReadIndex);
        msg.add_entries()->set_data(ctx);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    vector<CLogOperation*> opts;
    bFrame->ReadLogOpt(opts);
    CReadState *s = opts[0]->m_pOperation;
    CPPUNIT_ASSERT(s->m_u64Index == None);
    CPPUNIT_ASSERT(s->m_strRequestCtx == ctx);
    bFrame->FreeLogOpt(opts);
    delete net;
}

// TestReadOnlyForNewLeader ensures that a leader only accepts CMessage::MsgReadIndex message
// when it commits at least one log entry at it term.
// TODO
void CTestRaftFixtrue::TestReadOnlyForNewLeader(void)
{
    vector<uint32_t> peers;
    vector<stateMachine*> sts;
    peers.push_back(1);
    peers.push_back(2);
    peers.push_back(3);

    CTestRaftFrame *aFrame, *bFrame, *cFrame;
    CRaft *a, *b, *c;
    {
        vector<CRaftEntry> entries;

        entries.push_back(CRaftEntry());

        CRaftEntry entry;
        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(1);
        entries.push_back(entry);

        aFrame = newTestRaft(1, peers, 10, 1, entries, 1, 1, 1);
        a = aFrame->m_pRaftNode;
        sts.push_back(new raftStateMachine(aFrame));
    }
    {
        vector<CRaftEntry> entries;

        CRaftEntry entry;
        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(1);
        entries.push_back(entry);

        bFrame = newTestRaft(2, peers, 10, 1, entries, 1, 2, 2);
        b = bFrame->m_pRaftNode;
        sts.push_back(new raftStateMachine(bFrame));
    }
    {
        vector<CRaftEntry> entries;

        CRaftEntry entry;
        entry.set_index(1);
        entry.set_term(1);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(1);
        entries.push_back(entry);

        cFrame = newTestRaft(3, peers, 10, 1, entries,1,2,2);
        c = cFrame->m_pRaftNode;

        sts.push_back(new raftStateMachine(cFrame));
    }

    network *net = newNetwork(sts);

    // Drop CMessage::MsgApp to forbid peer a to commit any log entry at its term after it becomes leader.
    net->ignore(CMessage::MsgApp);
    // Force peer a to become leader.
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CPPUNIT_ASSERT_EQUAL(a->GetState(), eStateLeader);

    // Ensure peer a drops read only request.
    uint64_t index = 4;
    string ctx = "ctx";
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgReadIndex);
        CRaftEntry *entry = msg.add_entries();
        entry->set_data(ctx);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    vector<CLogOperation*> opts;
    aFrame->ReadLogOpt(opts);
    CPPUNIT_ASSERT(opts.size() == 0);

    net->recover();

    // Force peer a to commit a log entry at its term
    int i;
    for (i = 0; i < a->m_pConfig->m_nTicksHeartbeat; ++i)
    {
        a->OnTick();
    }

    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(a->GetLog()->GetCommitted() == 4);

    uint64_t term;
    int err = a->m_pRaftLog->GetTerm(a->GetLog()->GetCommitted(), term);
    uint64_t lastLogTerm = a->m_pRaftLog->ZeroTermOnErrCompacted(term, err);
    CPPUNIT_ASSERT_EQUAL(lastLogTerm, a->GetTerm());

    // Ensure peer a accepts read only request after it commits a entry at its term.
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgReadIndex);
        msg.add_entries()->set_data(ctx);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    
    aFrame->ReadLogOpt(opts);
    CPPUNIT_ASSERT(opts.size() == 1);
    CReadState *rs = opts[0]->m_pOperation;
    CPPUNIT_ASSERT(rs->m_u64Index == index);
    CPPUNIT_ASSERT_EQUAL(rs->m_strRequestCtx, ctx);
    aFrame->FreeLogOpt(opts);
    delete net;
}

void CTestRaftFixtrue::TestLeaderAppResp(void)
{
    // initial progress: match = 0; next = 3
    struct tmp
    {
        uint64_t index;
        bool reject;
        // progress
        uint64_t match;
        uint64_t next;
        // message
        int msgNum;
        uint64_t windex;
        uint64_t wcommit;

        tmp(uint64_t i, bool reject, uint64_t match, uint64_t next, int num, uint64_t index, uint64_t commit)
            : index(i), reject(reject), match(match), next(next), msgNum(num), windex(index), wcommit(commit)
        {
        }
    };

    vector<tmp> tests;
    // stale resp; no replies
    tests.push_back(tmp(3, true, 0, 3, 0, 0, 0));
    // denied resp; leader does not commit; decrease next and send probing msg
    tests.push_back(tmp(2, true, 0, 2, 1, 1, 0));
    // accept resp; leader commits; broadcast with commit index
    tests.push_back(tmp(2, false, 2, 4, 2, 2, 2));
    // ignore heartbeat replies
    tests.push_back(tmp(0, false, 0, 3, 0, 0, 0));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        // sm term is 1 after it becomes the leader.
        // thus the last log term must be 1 to be committed.
        tmp &t = tests[i];

        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;
        CRaftMemStorage *s = NULL;
        CRaftMemLog* pMemLog = NULL;
        {
            s = new CRaftMemStorage(&kDefaultLogger);
            EntryVec entries;
            entries.push_back(CRaftEntry());

            CRaftEntry entry;
            entry.set_index(1);
            entry.set_term(0);
            entries.push_back(entry);

            entry.set_index(2);
            entry.set_term(1);
            entries.push_back(entry);
            s->m_entries = entries;
            pMemLog = newLog(s, &kDefaultLogger);
            pMemLog->m_unstablePart.m_u64Offset = 3;
            r->m_pRaftLog = pMemLog;
        }

        r->BecomeCandidate();
        r->BecomeLeader();
        vector<CMessage*> msgs;
        r->ReadMessages(msgs);
        {
            CMessage msg;
            msg.set_from(2);
            msg.set_type(CMessage::MsgAppResp);
            msg.set_index(t.index);
            msg.set_term(r->GetTerm());
            msg.set_reject(t.reject);
            msg.set_rejecthint(t.index);
            r->Step(msg);
        }

        CProgress *p = r->m_mapProgress[2];
        CPPUNIT_ASSERT_EQUAL(p->m_u64MatchLogIndex, t.match);
        CPPUNIT_ASSERT_EQUAL(p->m_u64NextLogIndex, t.next);

        r->ReadMessages(msgs);

        CPPUNIT_ASSERT(msgs.size() == t.msgNum);
        int j;
        for (j = 0; j < msgs.size(); ++j)
        {
            CMessage *msg = msgs[j];
            CPPUNIT_ASSERT_EQUAL(msg->index(), t.windex);
            CPPUNIT_ASSERT_EQUAL(msg->commit(), t.wcommit);
        }
        r->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
        delete pMemLog;
        delete s;
    }
}

// When the leader receives a heartbeat tick, it should
// send a CMessage::MsgApp with m.Index = 0, m.LogTerm=0 and empty entries.
void CTestRaftFixtrue::TestBcastBeat(void)
{
    uint64_t offset = 1000;
    // make a state machine with log.offset = 1000
    CSnapshot ss;
    ss.mutable_metadata()->set_index(offset);
    ss.mutable_metadata()->set_term(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(2);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(3);
    vector<uint32_t> peers;
    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1,ss);
    CRaft *r = pFrame->m_pRaftNode;
    r->SetTerm(1);

    r->BecomeCandidate();
    r->BecomeLeader();

    EntryVec entries;
    int i;
    for (i = 0; i < 10; ++i)
    {
        CRaftEntry entry;
        entry.set_index(i + 1);
        entries.push_back(entry);
    }
    r->AppendEntry(entries);
    // slow follower
    r->m_mapProgress[2]->m_u64MatchLogIndex = 5;
    r->m_mapProgress[2]->m_u64NextLogIndex = 6;
    // normal follower
    r->m_mapProgress[3]->m_u64MatchLogIndex = r->m_pRaftLog->GetLastIndex();
    r->m_mapProgress[3]->m_u64NextLogIndex = r->m_pRaftLog->GetLastIndex() + 1;

    {
        CMessage msg;
        msg.set_type(CMessage::MsgBeat);
        r->Step(msg);
    }

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);

    CPPUNIT_ASSERT(msgs.size() == 2);
    map<uint64_t, uint64_t> wantCommitMap;
    wantCommitMap[2] = min(r->GetLog()->GetCommitted(), r->m_mapProgress[2]->m_u64MatchLogIndex);
    wantCommitMap[3] = min(r->GetLog()->GetCommitted(), r->m_mapProgress[3]->m_u64MatchLogIndex);

    for (i = 0; i < msgs.size(); ++i)
    {
        CMessage *msg = msgs[i];
        CPPUNIT_ASSERT_EQUAL(msg->type(), CMessage::MsgHeartbeat);
        CPPUNIT_ASSERT(msg->index() == 0);
        CPPUNIT_ASSERT(msg->logterm() == 0);
        CPPUNIT_ASSERT(wantCommitMap[msg->to()] != 0);
        CPPUNIT_ASSERT_EQUAL(msg->commit(), wantCommitMap[msg->to()]);
        CPPUNIT_ASSERT(msg->entries_size()== 0);
    }
    pFrame->FreeMessages(msgs);
    pFrame->Uninit();
    delete pFrame;
}

// tests the output of the state machine when receiving CMessage::MsgBeat
void CTestRaftFixtrue::TestRecvMsgBeat(void)
{
    struct tmp
    {
        EStateType state;
        int msg;

        tmp(EStateType s, int msg)
            : state(s), msg(msg)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(eStateLeader, 2));
    // candidate and follower should ignore CMessage::MsgBeat
    tests.push_back(tmp(eStateCandidate, 0));
    tests.push_back(tmp(eStateFollower, 0));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;

        EntryVec entries;
        entries.push_back(CRaftEntry());

        CRaftEntry entry;
        entry.set_index(1);
        entry.set_term(0);
        entries.push_back(entry);

        entry.set_index(2);
        entry.set_term(1);
        entries.push_back(entry);

        CRaftMemStorage *s = new CRaftMemStorage(&kDefaultLogger);
        s->m_entries = entries;
        CRaftMemLog* pMemLog = newLog(s, &kDefaultLogger);
        r->m_pRaftLog = pMemLog;

        r->SetTerm(1);
        r->SetState(t.state);

        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgBeat);
        r->Step(msg);

        vector<CMessage*> msgs;
        r->ReadMessages(msgs);

        CPPUNIT_ASSERT(msgs.size() == t.msg);

        int j;
        for (j = 0; j < msgs.size(); ++j)
        {
            CPPUNIT_ASSERT_EQUAL(msgs[j]->type(), CMessage::MsgHeartbeat);
        }
        r->FreeMessages(msgs);
        pFrame->Uninit();
        delete pFrame;
        delete s;
        delete pMemLog;
    }
}

void CTestRaftFixtrue::TestLeaderIncreaseNext(void)
{
    EntryVec prevEntries;
    {
        CRaftEntry entry;

        entry.set_term(1);
        entry.set_index(1);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(2);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(3);
        prevEntries.push_back(entry);
    }
    struct tmp
    {
        ProgressState state;
        uint64_t next, wnext;
        tmp(ProgressState s, uint64_t n, uint64_t wn)
            : state(s), next(n), wnext(wn)
        {
        }
    };
    vector<tmp> tests;
    // state replicate, optimistically increase next
    // previous entries + noop entry + propose + 1
    tests.push_back(tmp(ProgressStateReplicate, 2, prevEntries.size() + 1 + 1 + 1));
    // state probe, not optimistically increase next
    tests.push_back(tmp(ProgressStateProbe, 2, 2));

    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp& t = tests[i];

        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);

        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;
        r->m_pRaftLog->Append(prevEntries);
        r->BecomeCandidate();
        r->BecomeLeader();
        r->m_mapProgress[2]->ResetState(t.state);
        r->m_mapProgress[2]->m_u64NextLogIndex = t.next;

        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries()->set_data("somedata");
        r->Step(msg);

        CPPUNIT_ASSERT_EQUAL(r->m_mapProgress[2]->m_u64NextLogIndex, t.wnext);
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestSendAppendForProgressProbe(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);
    r->m_mapProgress[2]->BecomeProbe();
    r->FreeMessages(msgs);
    // each round is a heartbeat
    int i;
    for (i = 0; i < 3; ++i)
    {
        if (i == 0)
        {
            // we expect that CRaft will only send out one msgAPP on the first
            // loop. After that, the follower is paused until a heartbeat response is
            // received.
            EntryVec entries;
            CRaftEntry entry;
            entry.set_data("somedata");
            entries.push_back(entry);
            r->AppendEntry(entries);
            r->SendAppend(2);
            r->ReadMessages(msgs);
            CPPUNIT_ASSERT(msgs.size() == 1);
            CPPUNIT_ASSERT(msgs[0]->index() == 0);
            r->FreeMessages(msgs);
        }

        CPPUNIT_ASSERT(r->m_mapProgress[2]->m_bPaused);

        int j;
        // do a heartbeat
        for (j = 0; j < r->m_pConfig->m_nTicksHeartbeat; ++j)
        {
            CMessage msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(CMessage::MsgBeat);
            r->Step(msg);
        }
        CPPUNIT_ASSERT(r->m_mapProgress[2]->m_bPaused);

        // consume the heartbeat
        vector<CMessage*> msgs;
        r->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgHeartbeat);

        // a heartbeat response will allow another message to be sent
        {
            CMessage msg;
            msg.set_from(2);
            msg.set_to(1);
            msg.set_type(CMessage::MsgHeartbeatResp);
            r->Step(msg);
        }
        r->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        CPPUNIT_ASSERT(msgs[0]->index() == 0);
        CPPUNIT_ASSERT(r->m_mapProgress[2]->m_bPaused);
        r->FreeMessages(msgs);
    }
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestSendAppendForProgressReplicate(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);
    r->m_mapProgress[2]->BecomeReplicate();
    r->FreeMessages(msgs);
    int i;
    for (i = 0; i < 10; ++i)
    {
        EntryVec entries;
        CRaftEntry entry;
        entry.set_data("somedata");
        entries.push_back(entry);
        r->AppendEntry(entries);
        r->SendAppend(2);
        r->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 1);
        r->FreeMessages(msgs);
    }
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestSendAppendForProgressSnapshot(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);
    r->m_mapProgress[2]->BecomeSnapshot(10);
    r->FreeMessages(msgs);
    int i;
    for (i = 0; i < 10; ++i)
    {
        EntryVec entries;
        CRaftEntry entry;
        entry.set_data("somedata");
        entries.push_back(entry);
        r->AppendEntry(entries);
        r->SendAppend(2);
        r->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 0);
        r->FreeMessages(msgs);
    }
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestRecvMsgUnreachable(void)
{
    EntryVec prevEntries;
    {
        CRaftEntry entry;

        entry.set_term(1);
        entry.set_index(1);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(2);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(3);
        prevEntries.push_back(entry);
    }
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1, prevEntries);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    // set node 2 to state replicate
    r->m_mapProgress[2]->m_u64MatchLogIndex = 3;
    r->m_mapProgress[2]->BecomeReplicate();
    r->m_mapProgress[2]->OptimisticUpdate(5);

    {
        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgUnreachable);
        r->Step(msg);
    }

    CPPUNIT_ASSERT(r->m_mapProgress[2]->GetState() == ProgressStateProbe);
    CPPUNIT_ASSERT_EQUAL(r->m_mapProgress[2]->m_u64NextLogIndex, r->m_mapProgress[2]->m_u64MatchLogIndex + 1);

    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestRestore(void)
{
    CSnapshot ss;
    ss.mutable_metadata()->set_index(11);
    ss.mutable_metadata()->set_term(11);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(2);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(3);

    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    CPPUNIT_ASSERT(r->Restore(ss));

    CPPUNIT_ASSERT_EQUAL(r->m_pRaftLog->GetLastIndex(), ss.metadata().index());
    uint64_t term;
    r->m_pRaftLog->GetTerm(ss.metadata().index(), term);
    CPPUNIT_ASSERT_EQUAL(term, ss.metadata().term());

    CPPUNIT_ASSERT(!(r->Restore(ss)));
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestRestoreIgnoreSnapshot(void)
{
    EntryVec prevEntries;
    {
        CRaftEntry entry;

        entry.set_term(1);
        entry.set_index(1);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(2);
        prevEntries.push_back(entry);

        entry.set_term(1);
        entry.set_index(3);
        prevEntries.push_back(entry);
    }
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->m_pRaftLog->Append(prevEntries);
    uint64_t commit = 1;
    r->m_pRaftLog->CommitTo(commit);

    CSnapshot ss;
    ss.mutable_metadata()->set_index(commit);
    ss.mutable_metadata()->set_term(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(2);

    // ignore snapshot
    CPPUNIT_ASSERT(!r->Restore(ss));

    CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetCommitted(), commit);

    // ignore snapshot and fast forward commit
    ss.mutable_metadata()->set_index(commit + 1);
    CPPUNIT_ASSERT(!r->Restore(ss));
    CPPUNIT_ASSERT_EQUAL(r->GetLog()->GetCommitted(), commit + 1);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestProvideSnap(void)
{
    // restore the state machine from a snapshot so it has a compacted log and a snapshot
    CSnapshot ss;
    ss.mutable_metadata()->set_index(11);
    ss.mutable_metadata()->set_term(11);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(2);

    vector<uint32_t> peers;
    peers.push_back(1);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->Restore(ss);

    r->BecomeCandidate();
    r->BecomeLeader();

    // force set the next of node 2, so that node 2 needs a snapshot
    r->m_mapProgress[2]->m_u64NextLogIndex = r->m_pRaftLog->GetFirstIndex();
    {
        CMessage msg;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgAppResp);
        msg.set_index(r->m_mapProgress[2]->m_u64NextLogIndex - 1);
        msg.set_reject(true);
        r->Step(msg);
    }

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);
    CPPUNIT_ASSERT(msgs.size() == 1);
    CPPUNIT_ASSERT_EQUAL(msgs[0]->type(), CMessage::MsgSnap);
    r->FreeMessages(msgs);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestIgnoreProvidingSnap(void)
{
    // restore the state machine from a snapshot so it has a compacted log and a snapshot
    CSnapshot ss;
    ss.mutable_metadata()->set_index(11);
    ss.mutable_metadata()->set_term(11);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(1);
    ss.mutable_metadata()->mutable_conf_state()->add_nodes(2);

    vector<uint32_t> peers;
    peers.push_back(1);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->Restore(ss);

    r->BecomeCandidate();
    r->BecomeLeader();

    // force set the next of node 2, so that node 2 needs a snapshot
    // change node 2 to be inactive, expect node 1 ignore sending snapshot to 2
    r->m_mapProgress[2]->m_u64NextLogIndex = r->m_pRaftLog->GetFirstIndex() - 1;
    r->m_mapProgress[2]->m_bRecentActive = false;
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries()->set_data("somedata");
        r->Step(msg);
    }

    vector<CMessage*> msgs;
    r->ReadMessages(msgs);
    CPPUNIT_ASSERT(msgs.size() ==0);
    r->FreeMessages(msgs);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestRestoreFromSnapMsg(void)
{
}

void CTestRaftFixtrue::TestSlowNodeRestore(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    int i;
    for (i = 0; i <= 100; ++i)
    {
        vector<CMessage> msgs;
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();
    EntryVec entries;
    nextEnts(leader, net->storage[1], &entries);
    CConfState cs;
    vector<uint32_t> nodes;
    leader->GetNodes(nodes);
    int j;
    for (j = 0; j < nodes.size(); ++j)
    {
        cs.add_nodes(nodes[j]);
    }
    net->storage[1]->CreateSnapshot(leader->GetLog()->GetApplied(), &cs, "", NULL);
    net->storage[1]->Compact(leader->GetLog()->GetApplied());

    net->recover();
    // send heartbeats so that the leader can learn everyone is active.
    // node 3 will only be considered as active when node 1 receives a reply from it.
    do
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgBeat);
        vector<CMessage> msgs;
        msgs.push_back(msg);
        net->send(&msgs);
    } while (leader->m_mapProgress[3]->m_bRecentActive == false);

    // trigger a snapshot
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        vector<CMessage> msgs;
        net->send(&msgs);
    }

    CRaft *follower = (CRaft*)net->peers[3]->data();

    // trigger a commit
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        vector<CMessage> msgs;
        net->send(&msgs);
    }
    CPPUNIT_ASSERT_EQUAL(follower->GetLog()->GetCommitted(), leader->GetLog()->GetCommitted());
    delete net;
}

// TestStepConfig tests that when CRaft Step msgProp in CRaftEntry::eConfChange type,
// it appends the entry to log and sets pendingConf to be true.
void CTestRaftFixtrue::TestStepConfig(void)
{
    // a CRaft that cannot make progress
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    uint64_t index = r->m_pRaftLog->GetLastIndex();
    CMessage msg;
    msg.set_from(1);
    msg.set_to(1);
    msg.set_type(CMessage::MsgProp);
    msg.add_entries()->set_type(CRaftEntry::eConfChange);
    r->Step(msg);

    CPPUNIT_ASSERT_EQUAL(r->m_pRaftLog->GetLastIndex(), index + 1);
    CPPUNIT_ASSERT(r->m_bPendingConf);
    pFrame->Uninit();
    delete pFrame;
}

// TestStepIgnoreConfig tests that if CRaft Step the second msgProp in
// CRaftEntry::eConfChange type when the first one is uncommitted, the node will set
// the proposal to noop and keep its original state.
void CTestRaftFixtrue::TestStepIgnoreConfig(void)
{
    // a CRaft that cannot make progress
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries()->set_type(CRaftEntry::eConfChange);
        r->Step(msg);
    }
    uint64_t index = r->m_pRaftLog->GetLastIndex();
    bool pendingConf = r->m_bPendingConf;

    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries()->set_type(CRaftEntry::eConfChange);
        r->Step(msg);
    }
    EntryVec wents, ents;
    {
        CRaftEntry entry;
        entry.set_type(CRaftEntry::eNormal);
        entry.set_term(1);
        entry.set_index(3);
        wents.push_back(entry);
    }
    CRaftMemLog *pMemLog = dynamic_cast<CRaftMemLog*> (r->m_pRaftLog);
    int err = pMemLog->GetEntries(index + 1, noLimit, ents);

    CPPUNIT_ASSERT(err == CRaftErrNo::eOK);
    CPPUNIT_ASSERT(isDeepEqualEntries(wents, ents));
    CPPUNIT_ASSERT_EQUAL(r->m_bPendingConf, pendingConf);
    pFrame->Uninit();
    delete pFrame;
}

// TestRecoverPendingConfig tests that new leader recovers its pendingConf flag
// based on uncommitted entries.
void CTestRaftFixtrue::TestRecoverPendingConfig(void)
{
    struct tmp
    {     
        CRaftEntry::EType type;
        bool pending;

        tmp(CRaftEntry::EType t, bool pending)
            : type(t), pending(pending)
        {
        }
    };

    vector<tmp> tests;
    tests.push_back(tmp(CRaftEntry::eNormal, false));
    tests.push_back(tmp(CRaftEntry::eConfChange, true));
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);

        CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;

        EntryVec entries;
        CRaftEntry entry;
        entry.set_type(t.type);
        entries.push_back(entry);
        r->AppendEntry(entries);
        r->BecomeCandidate();
        r->BecomeLeader();
        CPPUNIT_ASSERT_EQUAL(t.pending, r->m_bPendingConf);
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestRecoverDoublePendingConfig(void)
{
}

// TestAddNode tests that addNode could update pendingConf and nodes correctly.
void CTestRaftFixtrue::TestAddNode(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->AddNode(2);
    CPPUNIT_ASSERT(!r->m_bPendingConf);
    vector<uint32_t> nodes, wnodes;
    r->GetNodes(nodes);

    std::sort(std::begin(nodes), std::end(nodes));
    wnodes.push_back(1);
    wnodes.push_back(2);
    CPPUNIT_ASSERT(isDeepEqualNodes(nodes, wnodes));
    pFrame->Uninit();
    delete pFrame;
}

// TestRemoveNode tests that removeNode could update pendingConf, nodes and
// and removed list correctly.
void CTestRaftFixtrue::TestRemoveNode(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 10, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->RemoveNode(2);
    CPPUNIT_ASSERT(!r->m_bPendingConf);
    vector<uint32_t> nodes, wnodes;
    r->GetNodes(nodes);

    wnodes.push_back(1);
    CPPUNIT_ASSERT(isDeepEqualNodes(nodes, wnodes));

    r->RemoveNode(1);
    wnodes.clear();
    r->GetNodes(nodes);
    CPPUNIT_ASSERT(isDeepEqualNodes(nodes, wnodes));
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestPromotable(void)
{
    struct tmp
    {
        vector<uint32_t> peers;
        bool wp;

        tmp(vector<uint32_t> p, bool wp)
            : peers(p), wp(wp)
        {
        }
    };
    vector<tmp> tests;
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        tests.push_back(tmp(peers, true));
    }
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        tests.push_back(tmp(peers, true));
    }
    {
        vector<uint32_t> peers;
        tests.push_back(tmp(peers, false));
    }
    {
        vector<uint32_t> peers;
        peers.push_back(2);
        peers.push_back(3);
        tests.push_back(tmp(peers, false));
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];

        CTestRaftFrame *pFrame = newTestRaft(1, t.peers, 5, 1);
        CRaft *r = pFrame->m_pRaftNode;

        CPPUNIT_ASSERT_EQUAL(r->IsPromotable(), t.wp);
        pFrame->Uninit();
        delete pFrame;
    }
}

void CTestRaftFixtrue::TestRaftNodes(void)
{
    struct tmp
    {
        vector<uint32_t> ids, wids;

        tmp(vector<uint32_t> p, vector<uint32_t> wp)
            : ids(p), wids(wp)
        {
        }
    };
    vector<tmp> tests;
    {
        vector<uint32_t> peers;
        peers.push_back(1);
        peers.push_back(2);
        peers.push_back(3);
        tests.push_back(tmp(peers, peers));
    }
    {
        vector<uint32_t> peers, wps;
        peers.push_back(3);
        peers.push_back(2);
        peers.push_back(1);
        wps.push_back(1);
        wps.push_back(2);
        wps.push_back(3);
        tests.push_back(tmp(peers, wps));
    }
    int i;
    for (i = 0; i < tests.size(); ++i)
    {
        tmp &t = tests[i];

        CTestRaftFrame *pFrame = newTestRaft(1, t.ids, 10, 1);
        CRaft *r = pFrame->m_pRaftNode;
        vector<uint32_t> nodes;
        r->GetNodes(nodes);
        std::sort(std::begin(nodes), std::end(nodes));
        CPPUNIT_ASSERT(isDeepEqualNodes(nodes, t.wids));
        pFrame->Uninit();
        delete pFrame;
    }
}

void testCampaignWhileLeader(bool prevote)
{
    vector<uint32_t> peers;
    peers.push_back(1);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->m_pConfig->m_bPreVote = prevote;
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);

    // We don't call campaign() directly because it comes after the check
    // for our current state.
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        r->Step(msg);
    }
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);

    uint64_t term = r->GetTerm();
    {
        CMessage msg;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        r->Step(msg);
    }
    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateLeader);
    CPPUNIT_ASSERT_EQUAL(r->GetTerm(), term);
    pFrame->Uninit();
    delete pFrame;
}

void CTestRaftFixtrue::TestCampaignWhileLeader(void)
{
    testCampaignWhileLeader(false);
}

void CTestRaftFixtrue::TestPreCampaignWhileLeader(void)
{
    testCampaignWhileLeader(true);
}

// TestCommitAfterRemoveNode verifies that pending commands can become
// committed when a config change reduces the quorum requirements.
void CTestRaftFixtrue::TestCommitAfterRemoveNode(void)
{
    // Create a cluster with two nodes.
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;

    r->BecomeCandidate();
    r->BecomeLeader();

    // Begin to remove the second node.
    CConfChange cc;
    cc.set_type(CConfChange::eConfChangeRemoveNode);
    cc.set_nodeid(2);
    string ccdata;
    cc.SerializeToString(ccdata);

    {
        CMessage msg;
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *ent = msg.add_entries();
        ent->set_type(CRaftEntry::eConfChange);
        ent->set_data(ccdata);
        r->Step(msg);
    }
    CRaftMemLog *pMemLog = dynamic_cast<CRaftMemLog *>(r->m_pRaftLog);
    CRaftMemStorage *s = dynamic_cast<CRaftMemStorage *>(pMemLog->m_pStorage);
    EntryVec entries;
    nextEnts(r, s, &entries);
    CPPUNIT_ASSERT(entries.size() == 0);

    uint64_t ccIndex = r->m_pRaftLog->GetLastIndex();

    // While the config change is pending, make another proposal.
    {
        CMessage msg;
        msg.set_type(CMessage::MsgProp);
        CRaftEntry *ent = msg.add_entries();
        ent->set_type(CRaftEntry::eNormal);
        ent->set_data("hello");
        r->Step(msg);
    }

    // Node 2 acknowledges the config change, committing it.
    {
        CMessage msg;
        msg.set_type(CMessage::MsgAppResp);
        msg.set_from(2);
        msg.set_index(ccIndex);
        r->Step(msg);
    }

    nextEnts(r, s, &entries);
    CPPUNIT_ASSERT(entries.size() == 2);
    CPPUNIT_ASSERT(!(entries[0].type() != CRaftEntry::eNormal || !entries[0].data().empty()));
    CPPUNIT_ASSERT(!(entries[1].type() != CRaftEntry::eConfChange));

    // Apply the config change. This reduces quorum requirements so the
    // pending command can now commit.
    r->RemoveNode(2);
    nextEnts(r, s, &entries);
    CPPUNIT_ASSERT(!(entries.size() != 1 || entries[0].type() != CRaftEntry::eNormal || entries[0].data() != "hello"));
    pFrame->Uninit();
    delete pFrame;
}

void checkLeaderTransferState(CRaft *r, EStateType state, uint64_t leader)
{
    CPPUNIT_ASSERT(!(r->GetState() != state || r->GetLeader() != leader));
    CPPUNIT_ASSERT(r->m_nLeaderTransfereeID == None);
}

// TestLeaderTransferToUpToDateNode verifies transferring should succeed
// if the transferee has the most up-to-date log entries when transfer starts.
void CTestRaftFixtrue::TestLeaderTransferToUpToDateNode(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(leader->GetLeader() == 1);

    // Transfer leadership to 2.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    checkLeaderTransferState(leader, eStateFollower, 2);
    // After some log replication, transfer leadership back to 1.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(2);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

// TestLeaderTransferToUpToDateNodeFromFollower verifies transferring should succeed
// if the transferee has the most up-to-date log entries when transfer starts.
// Not like TestLeaderTransferToUpToDateNode, where the leader transfer message
// is sent to the leader, in this test case every leader transfer message is sent
// to the follower.
void CTestRaftFixtrue::TestLeaderTransferToUpToDateNodeFromFollower(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(leader->GetLeader()== 1);

    // Transfer leadership to 2.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    checkLeaderTransferState(leader, eStateFollower, 2);
    // After some log replication, transfer leadership back to 1.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

// TestLeaderTransferWithCheckQuorum ensures transferring leader still works
// even the current leader is still under its leader lease
void CTestRaftFixtrue::TestLeaderTransferWithCheckQuorum(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);
    int i;
    for (i = 1; i < 4; ++i)
    {
        CRaft *r = (CRaft*)net->peers[i]->data();
        r->m_pConfig->m_bCheckQuorum = true;
        r->m_nTicksRandomizedElectionTimeout = r->m_pConfig->m_nTicksElection + i;
    }

    CRaft *r = (CRaft*)net->peers[2]->data();
    for (i = 0; i < r->m_pConfig->m_nTicksElection; ++i)
    {
        r->OnTick();
    }

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(leader->GetLeader() == 1);

    // Transfer leadership to 2.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    checkLeaderTransferState(leader, eStateFollower, 2);
    // After some log replication, transfer leadership back to 1.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(2);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferToSlowFollower(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->recover();
    CRaft *leader = (CRaft*)net->peers[1]->data();
    CPPUNIT_ASSERT(leader->m_mapProgress[3]->m_u64MatchLogIndex == 1);

    // Transfer leadership to 3 when node 3 is lack of log.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateFollower, 3);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferAfterSnapshot(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();
    EntryVec entries;
    nextEnts(leader, net->storage[1], &entries);
    CConfState cs;
    vector<uint32_t> nodes;
    leader->GetNodes(nodes);
    int j;
    for (j = 0; j < nodes.size(); ++j)
    {
        cs.add_nodes(nodes[j]);
    }
    net->storage[1]->CreateSnapshot(leader->GetLog()->GetApplied(), &cs, "", NULL);
    net->storage[1]->Compact(leader->GetLog()->GetApplied());

    net->recover();
    CPPUNIT_ASSERT(leader->m_mapProgress[3]->m_u64MatchLogIndex == 1);

    // Transfer leadership to 3 when node 3 is lack of log.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    // Send pb.CMessage::MsgHeartbeatResp to leader to trigger a snapshot for node 3.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHeartbeatResp);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateFollower, 3);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferToSelf(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();

    // Transfer leadership to self, there will be noop.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferToNonExistingNode(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    CRaft *leader = (CRaft*)net->peers[1]->data();

    // Transfer leadership to non-existing node, there will be noop.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(4);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferTimeout(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    // Transfer leadership to isolated node, wait for timeout.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->GetLeaderTransferee() == 3);
    int i;
    for (i = 0; i < leader->m_pConfig->m_nTicksHeartbeat; ++i)
    {
        leader->OnTick();
    }
    CPPUNIT_ASSERT(leader->GetLeaderTransferee() == 3);
    for (i = 0; i < leader->m_pConfig->m_nTicksElection - leader->m_pConfig->m_nTicksHeartbeat; ++i)
    {
        leader->OnTick();
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferIgnoreProposal(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    // Transfer leadership to isolated node to let transfer pending, then send proposal.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->GetLeaderTransferee() == 3);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgProp);
        msg.add_entries();
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->m_mapProgress[1]->m_u64MatchLogIndex == 1);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferReceiveHigherTermVote(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    // Transfer leadership to isolated node to let transfer pending.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->GetLeaderTransferee() == 3);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(2);
        msg.set_to(2);
        msg.set_type(CMessage::MsgHup);
        msg.set_index(1);
        msg.set_term(2);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    checkLeaderTransferState(leader, eStateFollower, 2);
    delete net;
}

void CTestRaftFixtrue::TestLeaderTransferRemoveNode(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->ignore(CMessage::MsgTimeoutNow);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    // The leadTransferee is removed when leadship transferring.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->GetLeaderTransferee() == 3);

    leader->RemoveNode(3);
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

// TestLeaderTransferBack verifies leadership can transfer back to self when last transfer is pending.
void CTestRaftFixtrue::TestLeaderTransferBack(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    // The leadTransferee is removed when leadship transferring.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->m_nLeaderTransfereeID == 3);

    // Transfer leadership back to self.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

// TestLeaderTransferSecondTransferToAnotherNode verifies leader can transfer to another node
// when last transfer is pending.
void CTestRaftFixtrue::TestLeaderTransferSecondTransferToAnotherNode(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->m_nLeaderTransfereeID == 3);

    // Transfer leadership to another node.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    checkLeaderTransferState(leader, eStateFollower, 2);
    delete net;
}

// TestLeaderTransferSecondTransferToSameNode verifies second transfer leader request
// to the same node should not extend the timeout while the first one is pending.
void CTestRaftFixtrue::TestLeaderTransferSecondTransferToSameNode(void)
{
    vector<stateMachine*> peers;
    peers.push_back(NULL);
    peers.push_back(NULL);
    peers.push_back(NULL);

    network *net = newNetwork(peers);

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(1);
        msg.set_to(1);
        msg.set_type(CMessage::MsgHup);
        msgs.push_back(msg);
        net->send(&msgs);
    }

    net->isolate(3);
    CRaft *leader = (CRaft*)net->peers[1]->data();

    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    CPPUNIT_ASSERT(leader->m_nLeaderTransfereeID == 3);

    int i;
    for (i = 0; i < leader->m_pConfig->m_nTicksHeartbeat; ++i)
    {
        leader->OnTick();
    }

    // Second transfer leadership request to the same node.
    {
        CMessage msg;
        vector<CMessage> msgs;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTransferLeader);
        msgs.push_back(msg);
        net->send(&msgs);
    }
    for (i = 0; i < leader->m_pConfig->m_nTicksElection - leader->m_pConfig->m_nTicksHeartbeat; ++i)
    {
        leader->OnTick();
    }
    checkLeaderTransferState(leader, eStateLeader, 1);
    delete net;
}

// TestTransferNonMember verifies that when a CMessage::MsgTimeoutNow arrives at
// a node that has been removed from the group, nothing happens.
// (previously, if the node also got votes, it would panic as it
// transitioned to eStateLeader)
void CTestRaftFixtrue::TestTransferNonMember(void)
{
    vector<uint32_t> peers;
    peers.push_back(2);
    peers.push_back(3);
    peers.push_back(4);

    CTestRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;

    {
        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgTimeoutNow);
        r->Step(msg);
    }

    {
        CMessage msg;
        msg.set_from(2);
        msg.set_to(1);
        msg.set_type(CMessage::MsgVoteResp);
        r->Step(msg);
    }
    {
        CMessage msg;
        msg.set_from(3);
        msg.set_to(1);
        msg.set_type(CMessage::MsgVoteResp);
        r->Step(msg);
    }

    CPPUNIT_ASSERT_EQUAL(r->GetState(), eStateFollower);
    pFrame->Uninit();
    delete pFrame;
}


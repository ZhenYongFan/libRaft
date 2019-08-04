#pragma once
#include <cppunit/extensions/HelperMacros.h>

class CTestRaftPaperFixture :public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(CTestRaftPaperFixture);
    
    CPPUNIT_TEST(TestFollowerUpdateTermFromMessage);
    CPPUNIT_TEST(TestCandidateUpdateTermFromMessage);
    CPPUNIT_TEST(TestLeaderUpdateTermFromMessage);

    CPPUNIT_TEST(TestRejectStaleTermMessage);
    CPPUNIT_TEST(TestStartAsFollower);
    CPPUNIT_TEST(TestLeaderBcastBeat);
    CPPUNIT_TEST(TestFollowerStartElection);

    CPPUNIT_TEST(TestCandidateStartNewElection);
    CPPUNIT_TEST(TestLeaderElectionInOneRoundRPC);
    CPPUNIT_TEST(TestFollowerVote);
    CPPUNIT_TEST(TestCandidateFallback);

    CPPUNIT_TEST(TestFollowerElectionTimeoutRandomized);
    CPPUNIT_TEST(TestCandidateElectionTimeoutRandomized);
    CPPUNIT_TEST(TestFollowersElectioinTimeoutNonconflict);
    CPPUNIT_TEST(TestCandidatesElectionTimeoutNonconflict);

    CPPUNIT_TEST(TestLeaderStartReplication);
    CPPUNIT_TEST(TestLeaderCommitEntry);
    CPPUNIT_TEST(TestLeaderAcknowledgeCommit);
    CPPUNIT_TEST(TestLeaderCommitPrecedingEntries);

    CPPUNIT_TEST(TestFollowerCommitEntry);
    CPPUNIT_TEST(TestFollowerCheckMsgApp);
    CPPUNIT_TEST(TestFollowerAppendEntries);
    CPPUNIT_TEST(TestLeaderSyncFollowerLog);

    CPPUNIT_TEST(TestVoteRequest);
    CPPUNIT_TEST(TestVoter);
    CPPUNIT_TEST(TestLeaderOnlyCommitsLogFromCurrentTerm);

    CPPUNIT_TEST_SUITE_END();
public:
    CTestRaftPaperFixture();

    ~CTestRaftPaperFixture();
    
    void setUp(void);

    void tearDown(void);
    void TestFollowerUpdateTermFromMessage(void);
    void TestCandidateUpdateTermFromMessage(void);
    void TestLeaderUpdateTermFromMessage(void);

    void TestRejectStaleTermMessage(void);
    void TestStartAsFollower(void);
    void TestLeaderBcastBeat(void);
    void TestFollowerStartElection(void);
    void TestCandidateStartNewElection(void);
    void TestLeaderElectionInOneRoundRPC(void);
    void TestFollowerVote(void);
    void TestCandidateFallback(void);
    void TestFollowerElectionTimeoutRandomized(void);
    void TestCandidateElectionTimeoutRandomized(void);
    void TestFollowersElectioinTimeoutNonconflict(void);
    void TestCandidatesElectionTimeoutNonconflict(void);
    void TestLeaderStartReplication(void);
    void TestLeaderCommitEntry(void);
    void TestLeaderAcknowledgeCommit(void);
    void TestLeaderCommitPrecedingEntries(void);
    void TestFollowerCommitEntry(void);
    void TestFollowerCheckMsgApp(void);
    void TestFollowerAppendEntries(void);
    void TestLeaderSyncFollowerLog(void);
    void TestVoteRequest(void);
    void TestVoter(void);
    void TestLeaderOnlyCommitsLogFromCurrentTerm(void);
};


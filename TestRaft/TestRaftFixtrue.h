#pragma once
#include <cppunit/extensions/HelperMacros.h>

// CTestRaftFixtrue 文档

class CTestRaftFixtrue :public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( CTestRaftFixtrue );

	CPPUNIT_TEST( TestExample);

	CPPUNIT_TEST_SUITE_END();

public:
	CTestRaftFixtrue();

	virtual ~CTestRaftFixtrue();

	void setUp(void);

	void tearDown(void);

	void TestExample(void);
public:
    void TestProgressBecomeProbe(void);
    void TestProgressBecomeReplicate(void);
    void TestProgressBecomeSnapshot(void);
    void TestProgressUpdate(void);
    void TestProgressMaybeDecr(void);
    void TestProgressIsPaused(void);
    void TestProgressResume(void);
    void TestProgressResumeByHeartbeatResp(void);
    void TestProgressPaused(void);
    void TestLeaderElection(void);
    void TestLeaderElectionPreVote(void);
    void TestLeaderCycle(void);
    void TestLeaderCyclePreVote(void);
    void TestLeaderElectionOverwriteNewerLogs(void);
    void TestLeaderElectionOverwriteNewerLogsPreVote(void);
    void TestVoteFromAnyState(void);
    void TestPreVoteFromAnyState(void);
    void TestLogReplication(void);
    void TestSingleNodeCommit(void);
    void TestCannotCommitWithoutNewTermEntry(void);
    void TestCommitWithoutNewTermEntry(void);
    void TestDuelingCandidates(void);
    void TestDuelingPreCandidates(void);
    void TestCandidateConcede(void);
    void TestSingleNodeCandidate(void);
    void TestSingleNodePreCandidate(void);
    void TestOldMessages(void);
    void TestProposal(void);

};

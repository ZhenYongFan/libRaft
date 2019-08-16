#pragma once
#include <cppunit/extensions/HelperMacros.h>

// CTestRaftFixtrue 文档

class CTestRaftFixtrue :public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE( CTestRaftFixtrue );

    CPPUNIT_TEST(TestProgressBecomeProbe);
    CPPUNIT_TEST(TestProgressBecomeReplicate);
    CPPUNIT_TEST(TestProgressBecomeSnapshot);

    CPPUNIT_TEST(TestProgressUpdate);
    CPPUNIT_TEST(TestProgressMaybeDecr);
    CPPUNIT_TEST(TestProgressIsPaused);
    CPPUNIT_TEST(TestProgressResume);
    CPPUNIT_TEST(TestProgressResumeByHeartbeatResp);
    CPPUNIT_TEST(TestProgressPaused);

    CPPUNIT_TEST(TestLeaderElection);
    CPPUNIT_TEST(TestLeaderElectionPreVote);
    CPPUNIT_TEST(TestLeaderCycle);
    CPPUNIT_TEST(TestLeaderCyclePreVote);
    CPPUNIT_TEST(TestLeaderElectionOverwriteNewerLogs);
    CPPUNIT_TEST(TestLeaderElectionOverwriteNewerLogsPreVote);

    // ok 0x000007FEFD4EBE0D 处(位于 TestRaft.exe 中)引发的异常: Microsoft C++ 异常: std::invalid_argument，位于内存位置 0x000000000029D2A0 处。
    CPPUNIT_TEST(TestVoteFromAnyState);
    CPPUNIT_TEST(TestPreVoteFromAnyState);
    CPPUNIT_TEST(TestLogReplication);
    CPPUNIT_TEST(TestSingleNodeCommit);
    CPPUNIT_TEST(TestCannotCommitWithoutNewTermEntry);
    CPPUNIT_TEST(TestCommitWithoutNewTermEntry);

    CPPUNIT_TEST(TestDuelingCandidates);
    CPPUNIT_TEST(TestDuelingPreCandidates);
    CPPUNIT_TEST(TestCandidateConcede);
    CPPUNIT_TEST(TestSingleNodeCandidate);
    CPPUNIT_TEST(TestSingleNodePreCandidate);
    CPPUNIT_TEST(TestOldMessages);

    CPPUNIT_TEST(TestProposal);
    CPPUNIT_TEST(TestProposalByProxy);
    CPPUNIT_TEST(TestCommit);
    CPPUNIT_TEST(TestPastElectionTimeout);
    CPPUNIT_TEST(TestHandleMsgApp);

    CPPUNIT_TEST(TestHandleHeartbeat);
    CPPUNIT_TEST(TestHandleHeartbeatResp);

    CPPUNIT_TEST(TestRaftFreesReadOnlyMem);
    CPPUNIT_TEST(TestMsgAppRespWaitReset);
    CPPUNIT_TEST(TestRecvMsgVote);
    CPPUNIT_TEST(TestStateTransition);

    CPPUNIT_TEST(TestAllServerStepdown);
    CPPUNIT_TEST(TestLeaderStepdownWhenQuorumActive);
    CPPUNIT_TEST(TestLeaderStepdownWhenQuorumLost);
    CPPUNIT_TEST(TestLeaderSupersedingWithCheckQuorum);
    CPPUNIT_TEST(TestLeaderElectionWithCheckQuorum);
    CPPUNIT_TEST(TestFreeStuckCandidateWithCheckQuorum);
    CPPUNIT_TEST(TestNonPromotableVoterWithCheckQuorum);

    CPPUNIT_TEST(TestReadOnlyOptionSafe);
    CPPUNIT_TEST(TestReadOnlyOptionLease);
    CPPUNIT_TEST(TestReadOnlyOptionLeaseWithoutCheckQuorum);
    CPPUNIT_TEST(TestReadOnlyForNewLeader);

    CPPUNIT_TEST(TestLeaderAppResp);
    CPPUNIT_TEST(TestBcastBeat);
    CPPUNIT_TEST(TestRecvMsgBeat);
    CPPUNIT_TEST(TestLeaderIncreaseNext);
    CPPUNIT_TEST(TestSendAppendForProgressProbe);
    CPPUNIT_TEST(TestSendAppendForProgressReplicate);
    CPPUNIT_TEST(TestSendAppendForProgressSnapshot);

    CPPUNIT_TEST(TestRecvMsgUnreachable);
    CPPUNIT_TEST(TestRestore);
    CPPUNIT_TEST(TestRestoreIgnoreSnapshot);
    CPPUNIT_TEST(TestProvideSnap);
    CPPUNIT_TEST(TestIgnoreProvidingSnap);
    CPPUNIT_TEST(TestRestoreFromSnapMsg);
    CPPUNIT_TEST(TestSlowNodeRestore);
    CPPUNIT_TEST(TestStepConfig);
    CPPUNIT_TEST(TestStepIgnoreConfig);
    CPPUNIT_TEST(TestRecoverPendingConfig);
    CPPUNIT_TEST(TestRecoverDoublePendingConfig);
    CPPUNIT_TEST(TestAddNode);
    CPPUNIT_TEST(TestRemoveNode);
    CPPUNIT_TEST(TestPromotable);
    CPPUNIT_TEST(TestRaftNodes);
    CPPUNIT_TEST(TestCampaignWhileLeader);
    CPPUNIT_TEST(TestPreCampaignWhileLeader);
    CPPUNIT_TEST(TestCommitAfterRemoveNode);

    CPPUNIT_TEST(TestLeaderTransferToUpToDateNode);
    CPPUNIT_TEST(TestLeaderTransferToUpToDateNodeFromFollower);
    CPPUNIT_TEST(TestLeaderTransferWithCheckQuorum);
    CPPUNIT_TEST(TestLeaderTransferToSlowFollower);

    CPPUNIT_TEST(TestLeaderTransferAfterSnapshot);
    CPPUNIT_TEST(TestLeaderTransferToSelf);
    CPPUNIT_TEST(TestLeaderTransferToNonExistingNode);
    CPPUNIT_TEST(TestLeaderTransferTimeout);
    CPPUNIT_TEST(TestLeaderTransferIgnoreProposal);
    CPPUNIT_TEST(TestLeaderTransferReceiveHigherTermVote);
    CPPUNIT_TEST(TestLeaderTransferRemoveNode);
    CPPUNIT_TEST(TestLeaderTransferBack);
    CPPUNIT_TEST(TestLeaderTransferSecondTransferToAnotherNode);
    CPPUNIT_TEST(TestLeaderTransferSecondTransferToSameNode);
    CPPUNIT_TEST(TestTransferNonMember);

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

    void TestProposalByProxy(void);
    void TestCommit(void);

    void TestPastElectionTimeout(void);
    void TestHandleMsgApp(void);
    void TestHandleHeartbeat(void);
    void TestHandleHeartbeatResp(void);
    void TestRaftFreesReadOnlyMem(void);
    void TestMsgAppRespWaitReset(void);
    void TestRecvMsgVote(void);
    void TestStateTransition(void);
    void TestAllServerStepdown(void);
    void TestLeaderStepdownWhenQuorumActive(void);
    void TestLeaderStepdownWhenQuorumLost(void);
    void TestLeaderSupersedingWithCheckQuorum(void);
    void TestLeaderElectionWithCheckQuorum(void);
    void TestFreeStuckCandidateWithCheckQuorum(void);
    void TestNonPromotableVoterWithCheckQuorum(void);

    void TestReadOnlyOptionSafe(void);
    void TestReadOnlyOptionLease(void);
    void TestReadOnlyOptionLeaseWithoutCheckQuorum(void);
    void TestReadOnlyForNewLeader(void);

    void TestLeaderAppResp(void);
    void TestBcastBeat(void);
    void TestRecvMsgBeat(void);
    void TestLeaderIncreaseNext(void);

    void TestSendAppendForProgressProbe(void);
    void TestSendAppendForProgressReplicate(void);
    void TestSendAppendForProgressSnapshot(void);

    void TestRecvMsgUnreachable(void);
    void TestRestore(void);
    void TestRestoreIgnoreSnapshot(void);
    void TestProvideSnap(void);
    void TestIgnoreProvidingSnap(void);
    void TestRestoreFromSnapMsg(void);
    void TestSlowNodeRestore(void);
    void TestStepConfig(void);
    void TestStepIgnoreConfig(void);
    void TestRecoverPendingConfig(void);
    void TestRecoverDoublePendingConfig(void);
    void TestAddNode(void);
    void TestRemoveNode(void);
    void TestPromotable(void);
    void TestRaftNodes(void);
    void TestCampaignWhileLeader(void);
    void TestPreCampaignWhileLeader(void);
    void TestCommitAfterRemoveNode(void);

    void TestLeaderTransferToUpToDateNode(void);
    void TestLeaderTransferToUpToDateNodeFromFollower(void);
    void TestLeaderTransferWithCheckQuorum(void);
    void TestLeaderTransferToSlowFollower(void);
    void TestLeaderTransferAfterSnapshot(void);
    void TestLeaderTransferToSelf(void);
    void TestLeaderTransferToNonExistingNode(void);
    void TestLeaderTransferTimeout(void);
    void TestLeaderTransferIgnoreProposal(void);
    void TestLeaderTransferReceiveHigherTermVote(void);
    void TestLeaderTransferRemoveNode(void);
    void TestLeaderTransferBack(void);
    void TestLeaderTransferSecondTransferToAnotherNode(void);
    void TestLeaderTransferSecondTransferToSameNode(void);
    void TestTransferNonMember(void);
};

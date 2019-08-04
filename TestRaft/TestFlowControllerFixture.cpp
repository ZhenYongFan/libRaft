#include "stdafx.h"
#include "raft.pb.h"
using namespace raftpb;
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TestFlowControllerFixture.h"
#include "TestRaftFrame.h"

CPPUNIT_TEST_SUITE_REGISTRATION(CTestFlowControllerFixture);

CTestFlowControllerFixture::CTestFlowControllerFixture()
{
}


CTestFlowControllerFixture::~CTestFlowControllerFixture()
{
}

void CTestFlowControllerFixture::setUp(void)
{
}

void CTestFlowControllerFixture::tearDown(void)
{

}

void CTestFlowControllerFixture::TestBase(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    pFrame->Uninit();
    delete pFrame;
}

// TestMsgAppFlowControlFull ensures:
// 1. msgApp can fill the sending window until full
// 2. when the window is full, no more msgApp can be sent.
void CTestFlowControllerFixture::TestMsgAppFlowControlFull(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);

    CRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();

    CProgress *pr2 = r->m_mapProgress[2];

    // force the progress to be in replicate state
    pr2->BecomeReplicate();
    // fill in the inflights window
    int i;
    for (i = 0; i < r->m_pConfig->m_nMaxInfilght; ++i)
    {
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            Entry *entry = msg.add_entries();
            entry->set_data("somedata");

            r->Step(msg);
        }

        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size()== 1);
        pFrame->FreeMessages(msgs);
    }

    // ensure 1
    CPPUNIT_ASSERT(pr2->ins_.IsFull());

    // ensure 2
    for (i = 0; i < 10; ++i)
    {
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            Entry *entry = msg.add_entries();
            entry->set_data("somedata");

            r->Step(msg);
        }

        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size() == 0);
        pFrame->FreeMessages(msgs);
    }

    pFrame->Uninit();
    delete pFrame;
}

// TestMsgAppFlowControlMoveForward ensures msgAppResp can move
// forward the sending window correctly:
// 1. valid msgAppResp.index moves the windows to pass all smaller or equal index.
// 2. out-of-dated msgAppResp has no effect on the sliding window.
void CTestFlowControllerFixture::TestMsgAppFlowControlMoveForward(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    CRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();
    CProgress *pr2 = r->m_mapProgress[2];

    // force the progress to be in replicate state
    pr2->BecomeReplicate();
    // fill in the inflights window
    int i;
    for (i = 0; i < r->m_pConfig->m_nMaxInfilght; ++i)
    {
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            Entry *entry = msg.add_entries();
            entry->set_data("somedata");

            r->Step(msg);
        }

        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        pFrame->FreeMessages(msgs);
    }

    // 1 is noop, 2 is the first proposal we just sent.
    // so we start with 2.
    for (i = 2; i < r->m_pConfig->m_nMaxInfilght; ++i)
    {
        {
            Message msg;
            msg.set_from(2);
            msg.set_to(1);
            msg.set_type(MsgAppResp);
            msg.set_index(i);

            r->Step(msg);
        }

        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        pFrame->FreeMessages(msgs);
        // fill in the inflights window again
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            Entry *entry = msg.add_entries();
            entry->set_data("somedata");

            r->Step(msg);
        }
        pFrame->ReadMessages(msgs);
        CPPUNIT_ASSERT(msgs.size()== 1);
        pFrame->FreeMessages(msgs);
        // ensure 1
        CPPUNIT_ASSERT(pr2->ins_.IsFull());

        // ensure 2
        int j;
        for (j = 0; j < i; ++j)
        {
            {
                Message msg;
                msg.set_from(2);
                msg.set_to(1);
                msg.set_index(j);
                msg.set_type(MsgAppResp);

                r->Step(msg);
            }

            CPPUNIT_ASSERT(pr2->ins_.IsFull());
        }
    }
    pFrame->Uninit();
    delete pFrame;
}

// TestMsgAppFlowControlRecvHeartbeat ensures a heartbeat response
// frees one slot if the window is full.
void CTestFlowControllerFixture::TestMsgAppFlowControlRecvHeartbeat(void)
{
    vector<uint32_t> peers;
    peers.push_back(1);
    peers.push_back(2);
    CRaftFrame *pFrame = newTestRaft(1, peers, 5, 1);
    CRaft *r = pFrame->m_pRaftNode;
    r->BecomeCandidate();
    r->BecomeLeader();
    CProgress *pr2 = r->m_mapProgress[2];

    // force the progress to be in replicate state
    pr2->BecomeReplicate();
    // fill in the inflights window
    int i;
    for (i = 0; i < r->m_pConfig->m_nMaxInfilght; ++i)
    {
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            Entry *entry = msg.add_entries();
            entry->set_data("somedata");

            r->Step(msg);
        }

        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        pFrame->FreeMessages(msgs);
    }

    for (i = 1; i < 5; ++i)
    {
        CPPUNIT_ASSERT(pr2->ins_.IsFull());

        // recv tt msgHeartbeatResp and expect one free slot
        int j;
        for (j = 0; j < i; ++j)
        {
            {
                Message msg;
                msg.set_from(2);
                msg.set_to(1);
                msg.set_type(MsgHeartbeatResp);

                r->Step(msg);
            }
            vector<Message*> msgs;
            pFrame->ReadMessages(msgs);
            CPPUNIT_ASSERT(!(pr2->ins_.IsFull()));
            pFrame->FreeMessages(msgs);
        }

        // one slot
        {
            Message msg;
            msg.set_from(1);
            msg.set_to(1);
            msg.set_type(MsgProp);
            msg.add_entries()->set_data("somedata");

            r->Step(msg);
            vector<Message*> msgs;
            pFrame->ReadMessages(msgs);
            CPPUNIT_ASSERT(msgs.size()== 1);
            pFrame->FreeMessages(msgs);
        }

        // and just one slot
        for (j = 0; j < 10; ++j)
        {
            {
                Message msg;
                msg.set_from(1);
                msg.set_to(1);
                msg.set_type(MsgProp);
                msg.add_entries()->set_data("somedata");

                r->Step(msg);
            }
            vector<Message*> msgs;
            pFrame->ReadMessages(msgs);
            CPPUNIT_ASSERT(msgs.size()== 0);
            pFrame->FreeMessages(msgs);
        }

        // clear all pending messages.
        {
            Message msg;
            msg.set_from(2);
            msg.set_to(1);
            msg.set_type(MsgHeartbeatResp);

            r->Step(msg);
        }
        vector<Message*> msgs;
        pFrame->ReadMessages(msgs);
        pFrame->FreeMessages(msgs);
    }
    pFrame->Uninit();
    delete pFrame;
}

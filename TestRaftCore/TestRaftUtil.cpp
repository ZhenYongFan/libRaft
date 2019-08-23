#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "RaftMemLog.h"
#include "RaftMemStorage.h"
#include "Raft.h"
#include "RaftUtil.h"
#include "TestRaftUtil.h"
#include "NullLogger.h"
extern CNullLogger kDefaultLogger;

CTestRaftUtil::CTestRaftUtil()
{
}

CTestRaftUtil::~CTestRaftUtil()
{
}
// #include <stdlib.h>
// #include <time.h>
// #include "raft_test_util.h"
// #include "raft.h"
// #include "default_logger.h"
// #include "util.h"

// nextEnts returns the appliable entries and updates the applied index
void nextEnts(CRaft *r, CRaftStorage *s, EntryVec *entries)
{
    entries->clear();
    // Transfer all unstable entries to "stable" storage.
    EntryVec tmp;
    CRaftMemLog *pLog = dynamic_cast<CRaftMemLog *>(r->GetLog());
    pLog->unstableEntries(tmp);
    s->Append(tmp);
    pLog->StableTo(pLog->GetLastIndex(), pLog->GetLastTerm());

    pLog->nextEntries(*entries);
    pLog->AppliedTo(pLog->GetCommitted());
}

string raftLogString(CRaftMemLog *log)
{
    char buf[1024] = { '\0' };
    string str = "";

    snprintf(buf, sizeof(buf), "committed: %llu\n", log->GetCommitted());
    str += buf;

    snprintf(buf, sizeof(buf), "applied: %llu\n", log->GetApplied());
    str += buf;

    EntryVec entries;
    log->allEntries(entries);

    snprintf(buf, sizeof(buf), "entries size: %d\n", int(entries.size()));
    str += buf;

    int i;
    for (i = 0; i < entries.size(); ++i)
    {
        str += CRaftUtil::EntryString(entries[i]);
    }

    return str;
}

bool operator < (const connem& c1, const connem& c2)
{
    if (c1.from < c2.from)
    {
        return true;
    }
    if (c1.from == c2.from)
    {
        return c1.to < c2.to;
    }
    return false;
}

raftStateMachine::raftStateMachine(CTestRaftFrame *pFrame)
{
    m_pFrame = pFrame;
    raft = m_pFrame->m_pRaftNode;
}

raftStateMachine::raftStateMachine(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, EntryVec &ents, ConfigFun funCfg)
{
    m_pFrame = newTestRaft(id,peers, election, hb, ents, funCfg);
    raft = m_pFrame->m_pRaftNode;
}

raftStateMachine::raftStateMachine(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, CHardState &hs, ConfigFun funCfg)
{
    m_pFrame = newTestRaft(id, peers, election, hb, hs, funCfg);
    raft = m_pFrame->m_pRaftNode;
}

raftStateMachine::~raftStateMachine()
{
    if (m_pFrame != NULL)
    {
        m_pFrame->Uninit();
        delete m_pFrame;
        m_pFrame = NULL;
    }
    raft = NULL;
}

int raftStateMachine::step(const CMessage& msg)
{
    return raft->Step(msg);
}

void raftStateMachine::readMessages(vector<CMessage*> &msgs)
{
    raft->ReadMessages(msgs);
}

// newNetworkWithConfig is like newNetwork but calls the given func to
// modify the configuration of any state machines it creates.
network* newNetworkWithConfig(ConfigFun fun, const vector<stateMachine*>& peers)
{
    srand(time(NULL));
    int size = peers.size();
    vector<uint32_t> peerAddrs;
    idsBySize(size, &peerAddrs);
    network *net = new network();
    raftStateMachine *r;
    CRaft *rf;

    int i, j;
    for (i = 0; i < size; ++i)
    {
        stateMachine *p = peers[i];
        uint32_t id = peerAddrs[i];

        if (!p)
        {
//             s = new CRaftMemStorage(&kDefaultLogger);
//             net->storage[id] = s;
//             c = newTestConfig(id, peerAddrs, 10, 1, s);
//             if (fun != NULL)
//             {
//                 fun(c);
//             }
//             r = new raftStateMachine(c);
            EntryVec ents;
            r = new raftStateMachine(id, peerAddrs, 10, 1, &kDefaultLogger, ents, fun);
            CRaftMemLog *pLog = dynamic_cast<CRaftMemLog *>(r->raft->GetLog());
            net->storage[id] = dynamic_cast<CRaftMemStorage *>(pLog->m_pStorage);
            net->peers[id] = r;
            continue;
        }

        switch (p->type())
        {
        case raftType:
            rf = (CRaft *)p->data();
            rf->m_pConfig->m_nRaftID = id;
            for (j = 0; j < size; ++j)
            {
                delete rf->m_mapProgress[peerAddrs[j]];
                rf->m_mapProgress[peerAddrs[j]] = new CProgress(0, 256, &kDefaultLogger);
            }
            rf->Reset(rf->GetTerm());
            net->peers[id] = p;
            break;
        case blackHoleType:
            net->peers[id] = p;
            break;
        }
    }

    return net;
}

network* newNetwork(const vector<stateMachine*>& peers)
{
    return newNetworkWithConfig(NULL, peers);
}

void network::send(vector<CMessage> *msgs)
{
    while (!msgs->empty())
    {
        const CMessage& msg = (*msgs)[0];
        stateMachine *sm = peers[msg.to()];
        sm->step(msg);
        vector<CMessage> out;
        vector<CMessage*> readMsgs;
        msgs->erase(msgs->begin(), msgs->begin() + 1);
        sm->readMessages(readMsgs);
        filter(readMsgs, &out);
        for (auto pMsg:readMsgs)
        {
            delete pMsg;
        }
        msgs->insert(msgs->end(), out.begin(), out.end());
    }
}

void network::drop(uint64_t from, uint64_t to, int perc)
{
    dropm[connem(from, to)] = perc;
}

void network::cut(uint64_t one, uint64_t other)
{
    drop(one, other, 10);
    drop(other, one, 10);
}

void network::isolate(uint64_t id)
{
    int i;
    for (i = 0; i < peers.size(); ++i)
    {
        uint64_t nid = i + 1;
        if (nid != id)
        {
            drop(id, nid, 10);
            drop(nid, id, 10);
        }
    }
}

void network::ignore(CMessage::EMessageType t)
{
    ignorem[t] = true;
}

void network::recover()
{
    dropm.clear();
    ignorem.clear();
}

void network::filter(const vector<CMessage *>& msgs, vector<CMessage> *out)
{
    int i;
    for (i = 0; i < msgs.size(); ++i)
    {
        CMessage *msg = msgs[i];
        if (ignorem[msg->type()])
        {
            continue;
        }

        int perc;
        switch (msg->type())
        {
        case CMessage::MsgHup:
            break;
        default:
            perc = dropm[connem(msg->from(), msg->to())];
            if (rand() % 10 < perc)
            {
                continue;
            }
        }

        out->push_back(*msg);
    }
}

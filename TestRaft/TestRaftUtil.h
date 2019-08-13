#pragma once
#include <map>
using namespace std;
#include "TestRaftFrame.h"

class CTestRaftUtil
{
public:
    CTestRaftUtil();
    ~CTestRaftUtil();
};

enum stateMachineType
{
    raftType = 0,
    blackHoleType = 1
};

class stateMachine
{
public:
    virtual ~stateMachine()
    {
    }

    virtual int step(const Message&) = 0;
    virtual void readMessages(std::vector<Message*> &) = 0;

    virtual int type() = 0;
    virtual void* data() = 0;
};

struct connem
{
    uint64_t from, to;

    bool operator == (const connem& c)
    {
        return from == c.from && to == c.to;
    }

    void operator = (const connem& c)
    {
        from = c.from;
        to = c.to;
    }

    connem(uint64_t from, uint64_t to)
        : from(from), to(to)
    {
    }
};

struct raftStateMachine : public stateMachine
{
    raftStateMachine(CRaftFrame *pFrame);
    raftStateMachine(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, EntryVec &ents, ConfigFun funCfg);
    raftStateMachine(uint32_t id, const vector<uint32_t>& peers, int election, int hb, CLogger *pLogger, HardState &hs, ConfigFun funCfg);

    virtual ~raftStateMachine();

    virtual int step(const Message&);
    virtual void readMessages(vector<Message*> &);

    virtual int type()
    {
        return raftType;
    }
    virtual void* data()
    {
        return raft;
    }

    CRaft *raft;
    CRaftFrame *m_pFrame;
};

struct blackHole : public stateMachine
{
    blackHole()
    {
    }
    virtual ~blackHole()
    {
    }

    int step(const Message&)
    {
        return OK;
    }
    void readMessages(vector<Message*> &)
    {
    }

    int type()
    {
        return blackHoleType;
    }
    void* data()
    {
        return NULL;
    }
};

struct network
{
    map<uint64_t, stateMachine*> peers;
    map<uint64_t, CRaftMemStorage*> storage;
    map<connem, int> dropm;
    map<MessageType, bool> ignorem;

    void send(vector<Message>* msgs);
    void drop(uint64_t from, uint64_t to, int perc);
    void cut(uint64_t one, uint64_t other);
    void isolate(uint64_t id);
    void ignore(MessageType t);
    void recover();
    void filter(const vector<Message *>& msg, vector<Message> *out);
    ~network(void)
    {
        for (auto peer : peers)
        {
            if (peer.second != NULL)
            {
                stateMachine * second = dynamic_cast<blackHole*>(peer.second);
                if (second == NULL)
                    delete peer.second;
            }
        }
    }
};
 
 //extern Config* newTestConfig(uint64_t id, const vector<uint64_t>& peers, int election, int hb, Storage *s);
// extern raft* newTestRaft(uint64_t id, const vector<uint64_t>& peers, int election, int hb, Storage *s);
 extern network* newNetworkWithConfig(ConfigFun fun, const vector<stateMachine*>& peers);
 extern network* newNetwork(const vector<stateMachine*>& peers);
 extern void nextEnts(CRaft *r, CRaftStorage *s, EntryVec *entries);
 extern string raftLogString(CRaftMemLog *log);
// extern void idsBySize(int size, vector<uint64_t>* ids);

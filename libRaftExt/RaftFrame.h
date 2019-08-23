#pragma once

class CRaft;
class CRaftStorage;
class CRaftLog;
class CRaftQueue;
class CLogger;
class CKvService;
class CRaftSerializer;
class CRaftConfig;

class CRaftFrame
{
public:
    CRaftFrame(void);

    virtual ~CRaftFrame(void);

    virtual bool Init(std::string &strConfigPath, std::string &strErrMsg);

    virtual void Uninit(void);

    inline CRaftConfig * GetRaftConfig(void)
    {
        return m_pConfig;
    }

    inline CRaft * GetRaft(void)
    {
        return m_pRaft;
    }

    inline CRaftLog * GetRaftLog(void)
    {
        return m_pRaftLog;
    }

    inline CRaftQueue * GetMsgQueue(void)
    {
        return m_pMsgQueue;
    }

    inline CRaftQueue * GetIoQueue(void)
    {
        return m_pIoQueue;
    }

    inline CLogger * GetLogger(void)
    {
        return m_pLogger;
    }

    inline CKvService * GetKvService(void)
    {
        return m_pKvService;
    }

    inline CRaftSerializer *GetSerializer(void)
    {
        return m_pSerializer;
    }

protected:
    bool InitCfg(std::string &strCfgFile, std::string &strErrMsg);
    bool InitLogger(std::string &strLogCfgFile, std::string &strErrMsg);
    bool InitRaft(std::string &strErrMsg);
    bool InitRaftLog(std::string &strErrMsg);
protected:
    CRaftConfig *m_pConfig;
    CRaft *m_pRaft;
    CRaftStorage *m_pRaftStorage;
    CRaftLog *m_pRaftLog;
    CRaftQueue *m_pMsgQueue;
    CRaftQueue *m_pIoQueue;
    CLogger *m_pLogger;
    CKvService *m_pKvService;
    CRaftSerializer *m_pSerializer;
};


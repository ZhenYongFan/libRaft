#pragma once
#include "libRaftCore.h"
class CRaftEntry;
class CMessage;

///\brief Raft对象的串行化工具
class LIBRAFTCORE_API CRaftSerializer
{
public:
    CRaftSerializer();

    virtual ~CRaftSerializer();

    size_t ByteSize(const CRaftEntry &entry) const;
    
    void SerializeEntry(const CRaftEntry &entry, std::string &strValue);

    bool ParseEntry(CRaftEntry &entry,const std::string &strValue);

    void SerializeMessage(const CMessage &msg, std::string &strValue);

    bool ParseMessage(CMessage &msg, const std::string &strValue);
};


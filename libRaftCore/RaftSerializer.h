#pragma once
#include "libRaftCore.h"
class CRaftEntry;

///\brief Raft对象的串行化工具
class LIBRAFTCORE_API CRaftSerializer
{
public:
    CRaftSerializer();

    virtual ~CRaftSerializer();

    size_t ByteSize(const CRaftEntry &entry) const;
    void SerializeAsString(const CRaftEntry &entry, std::string &strValue);
    void ParseFromString(CRaftEntry &entry,const std::string &strValue);
};


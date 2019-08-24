#pragma once
#include "libRaftCore.h"
class CRaftEntry;
class CMessage;

///\brief Raft对象的串行化工具
class LIBRAFTCORE_API CRaftSerializer
{
public:
    ///\brief 构造函数
    CRaftSerializer(void);

    ///\brief 析构函数，支持派生
    virtual ~CRaftSerializer(void);

    ///\brief 计算一个Raft日志串行化后的大小
    ///\param entry Raft日志
    ///\return 串行化后的大小,一般用于限制内存占用
    virtual size_t ByteSize(const CRaftEntry &entry) const;

    ///\brief Raft日志编码为string
    ///\param entry Raft日志
    ///\param strValue 编码生成的string
    virtual void SerializeEntry(const CRaftEntry &entry, std::string &strValue);

    ///\brief 将string解析为Raft日志
    ///\param entry Raft日志
    ///\param strValue 要解码的string
    ///\return 成功标志 true 解码成功；false 失败
    virtual bool ParseEntry(CRaftEntry &entry,const std::string &strValue);

    ///\brief Raft消息编码为string
    ///\param msg Raft消息
    ///\param strValue 编码生成的string
    virtual void SerializeMessage(const CMessage &msg, std::string &strValue);

    ///\brief 将string解析为Raft消息
    ///\param msg Raft消息
    ///\param strValue 要解码的string
    ///\return 成功标志 true 解码成功；false 失败
    virtual bool ParseMessage(CMessage &msg, const std::string &strValue);
};

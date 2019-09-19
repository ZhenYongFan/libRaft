#pragma once
#include "RaftSerializer.h"
class CRequestOp;
class CResponseOp;

///\brief 使用Protobuffer作为串行化核心
class CRaftProtoBufferSerializer : public CRaftSerializer
{
public:
    ///\brief 构造函数
    CRaftProtoBufferSerializer(void);

    ///\brief 析构函数
    virtual ~CRaftProtoBufferSerializer(void);

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
    virtual bool ParseEntry(CRaftEntry &entry, const std::string &strValue);

    ///\brief Raft消息编码为string
    ///\param msg Raft消息
    ///\param strValue 编码生成的string
    virtual void SerializeMessage(const CMessage &msg, std::string &strValue);

    ///\brief 将string解析为Raft消息
    ///\param msg Raft消息
    ///\param strValue 要解码的string
    ///\return 成功标志 true 解码成功；false 失败
    virtual bool ParseMessage(CMessage &msg, const std::string &strValue);

    ///\brief 将配置变化项编码为string
    ///\param conf 配置变化项
    ///\param strValue 编码生成的string
    virtual void SerializeConfChangee(const CConfChange &conf, std::string &strValue);

    ///\brief 将string解析为配置变化项
    ///\param msg 配置变化项
    ///\param strValue 要解码的string
    ///\return 成功标志 true 解码成功；false 失败
    virtual bool ParseConfChange(CConfChange &conf, const std::string &strValue);

    void SerializeRequestOp(const CRequestOp &opRequest, std::string &strValue);

    bool ParseRequestOp(CRequestOp &opRequest, const std::string &strValue);

    void SerializeResponseOp(const CResponseOp &opResponse, std::string &strValue);

    bool ParseResponseOp(CResponseOp &opResponse, const std::string &strValue);
};

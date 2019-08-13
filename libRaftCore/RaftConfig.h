#pragma once
#include "libRaftCore.h"

///\brief 读模式，Linearizable Read 通俗来讲，就是读请求需要读到最新的已经commit的数据，不会读到老数据。
enum ReadOnlyOption
{
    ReadOnlySafe = 0,      ///< 通过与quorum通信来保证只读请求的线性化。这是默认的和建议的选项。
    ReadOnlyLeaseBased = 1 ///< 通过Leader租期保证只读请求的线性化，缺点是受时钟影响。
};

///\brief 节点信息
class LIBRAFTCORE_API CRaftInfo
{
public:
    uint32_t m_nNodeId;          ///< 节点编号
    std::string m_strHost;       ///< 域名或者ID
    unsigned short m_nPort;      ///< 对客户端服务的端口号
    unsigned short m_nPeerPort;  ///< 与兄弟节点交互的端口号
};

///\brief 当前Raft状态机的配置
class LIBRAFTCORE_API CRaftConfig
{
public:
    ///\brief 构造函数
    CRaftConfig(void);

    ///\brief 析构函数
    virtual ~CRaftConfig();

    ///\brief 检查参数的正确性
    ///\param strErrMsg 如果不正确返回的错误信息
    ///\return 正确性标志 true 正确；false 不正确
    bool Validate(std::string &strErrMsg) const;

    ///\brief 取得所有节点的ID
    ///\param peers 取回的节点ID
    void GetPeers(std::vector<uint32_t> &peers) const;

    ///\brief 取得本节点配置信息
    const CRaftInfo & GetSelf(void);

protected:
    ///\brief 检查节点定义的正确性
    ///\param strErrMsg 如果不正确返回的错误信息
    ///\return 正确性标志 true 正确；false 不正确
    bool ValidateNodes(std::string &strErrMsg) const;

public:
    uint32_t m_nRaftID;    ///< 当前节点ID,不能为0
    int m_nMsPerTick;      ///< 每个Tick的毫秒数,最小值100
    int m_nTicksHeartbeat; ///< Leader和Follower之间的心跳Tick数
    int m_nTicksElection;  ///< 选举Tick数，建议是10倍的心跳Tick数
    int m_nMaxInfilght;    ///< 在乐观复制阶段用于限制追加消息的最大数量
    int m_nMaxMsgSize;     ///< 限制每个追加消息的最大值。0表示为至少为1
    uint64_t m_u64Applied; ///< 应用索引号
    ReadOnlyOption m_optionReadOnly;  ///< ReadOnly的策略 ReadOnlySafe或者ReadOnlyLeaseBased
    bool m_bCheckQuorum;              ///< 是否检查leader和follower之间联通性
    bool m_bPreVote;                  ///< 是否有预选环节
    std::vector<CRaftInfo> m_aNodes;  ///< 全部节点
    std::string m_strDataPath;        ///< 数据目录
};

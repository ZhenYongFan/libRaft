# libRaft
<p>封装Raft算法 </p>
1、将etcd核心代码由go语言转换为C++语言，方便主语言是C++的同好在项目中使用或者平时研究学习
代码基础：https://github.com/lichuang/libraft
文档基础：etcd的图书和系列解读文章，如：
https://item.jd.com/29992600690.html
《etcd技术内幕》
https://blog.csdn.net/skh2015java/article/details/86711109
etcd中raft协议的消息(一) —— 简述raft中的所有类型消息
https://blog.csdn.net/skh2015java/article/details/86711338
etcd中raft协议的消息(二) —— 选举相关的消息(MsgHup、MsgPreVote/MsgPreVoteResp、MsgVote/MsgVoteResp）
https://blog.csdn.net/skh2015java/article/details/86711973
etcd中raft协议的消息(三) ——MspApp和MsgAppResp消息
https://blog.csdn.net/skh2015java/article/details/86712668
etcd中raft协议的消息(四) —— 心跳相关的消息(MsgBeat、MsgHeartbeat、MsgHeartbeatResp和MsgCheckQuorum）
https://blog.csdn.net/skh2015java/article/details/86713745
etcd中raft协议的消息(五)—— 客户端只读相关的消息(MsgReadIndex和MsgReadIndexResp消息)
https://blog.csdn.net/skh2015java/article/details/86713864
etcd中raft协议的消息(六)——客户端的写请求相关的消息(MsgProp消息)
https://blog.csdn.net/skh2015java/article/details/86713924
etcd中raft协议的消息(七)——快照复制消息(MsgSnap消息)
https://www.jianshu.com/p/1496228df9a9
Raft的PreVote实现机制
https://cloud.tencent.com/developer/article/1394643
深入浅出etcd之raft实现
https://www.jianshu.com/p/27329f87c104
Raft在etcd中的实现（二）节点发送消息和接收消息流程
https://www.jianshu.com/p/16b5567c50a1
Raft在etcd中的实现（三）选举流程
https://www.infoq.cn/article/coreos-analyse-etcd/
CoreOS 实战：剖析 etcd
https://blog.csdn.net/xxb249/article/details/80787501
Etcd源码分析-Raft实现
https://blog.csdn.net/weixin_34409822/article/details/87800713
Raft在etcd中的实现（五）snapshot相关
https://blog.csdn.net/weixin_44534991/article/details/86495997
开源一个千万级多组Raft库 - Dragonboat
https://github.com/lni/dragonboat
https://www.jianshu.com/p/be9eceb31656
[转]让Raft变快100倍 - Dragonboat的写优化 

封装Raft算法
1、将etcd核心代码由go语言转换为C++语言，方便主语言是C++的同好在项目中使用或者研究学习
代码基础：https://github.com/lichuang/libraft
文档基础：etcd的图书和系列解读文章

2、学习版的特点
对效率不敏感：虚函数风格
注释：doxygen，中文
第三方库：除Raft核心外多用第三方库

3、环境
开发环境： Windows7 + vs2015
调试运行环境： Windows7，Centos 7.6
C++：支持C++11

4、在https://github.com/lichuang/libraft 基础上主要的工作
修改代码风格
添加中文注释
将gtest框架替换为cppunit，因为我的IDE不支持gtest的类与方法的解析
添加KV服务的实现
（即将）添加锁服务的实现

5、解决方案的组成
libRaftCore Raft核心算法，不依赖任何第三方库
TestRaftCore 采用cppunit的libRaftCore测试用例，实现network类模仿接口调用
libRaftExt  Raft扩展库，
1、采用Protobuffer实现协议串行化；
2、采用libevent实现网络；
3、采用Log4cxx实现日志；
4、采用Rocksdb实现日志持久化存储
5、采用RocksDb实现一个KV服务
6、采用libconfig++实现配置文件的读取
TestRaft 测试libRaftExt的用例
TestServer 实现简单的Raft KV Service
TestCliebt 实现简单的Raft Kv Client



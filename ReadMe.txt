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

5、Todo
将核心代码与Protobuffer解耦

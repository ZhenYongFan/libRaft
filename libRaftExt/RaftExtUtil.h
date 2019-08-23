#pragma once
#include "libRaftExt.h"
class CRaftConfig;

class LIBRAFTEXT_API CRaftExtUtil
{
public:
    static CRaftConfig * InitCfg(std::string &strCfgFile, std::string &strErrMsg);
private:
    CRaftExtUtil(void)
    {

    }

    ///\brief 禁用的复制型构造函数
    CRaftExtUtil(const CRaftExtUtil &)
    {
    }

    ///\brief 禁用的赋值运算符
    const CRaftExtUtil &operator = (const CRaftExtUtil &)
    {
        return *this;
    }
};


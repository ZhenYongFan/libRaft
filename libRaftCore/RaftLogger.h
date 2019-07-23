#pragma once
#include "libRaftCore.h"

class LIBRAFTCORE_API CLogger
{
public:
    virtual void Debugf(const char *file, int line, const char *fmt, ...) = 0;
    virtual void Infof(const char *file, int line, const char *fmt, ...) = 0;
    virtual void Warningf(const char *file, int line, const char *fmt, ...) = 0;
    virtual void Errorf(const char *file, int line, const char *fmt, ...) = 0;
    virtual void Fatalf(const char *file, int line, const char *fmt, ...) = 0;
};

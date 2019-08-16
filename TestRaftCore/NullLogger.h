#pragma once
#include "RaftLogger.h"

class CNullLogger : public CLogger
{
public:
    CNullLogger();
    ~CNullLogger();
    virtual void Debugf(const char *file, int line, const char *fmt, ...);
    virtual void Infof(const char *file, int line, const char *fmt, ...);
    virtual void Warningf(const char *file, int line, const char *fmt, ...);
    virtual void Errorf(const char *file, int line, const char *fmt, ...);
    virtual void Fatalf(const char *file, int line, const char *fmt, ...);
};


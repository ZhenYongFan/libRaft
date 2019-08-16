#include "stdafx.h"
#include "NullLogger.h"

CNullLogger::CNullLogger()
{
}


CNullLogger::~CNullLogger()
{
}

void CNullLogger::Debugf(const char *file, int line, const char *fmt, ...)
{

}
void CNullLogger::Infof(const char *file, int line, const char *fmt, ...)
{

}
void CNullLogger::Warningf(const char *file, int line, const char *fmt, ...)
{

}
void CNullLogger::Errorf(const char *file, int line, const char *fmt, ...)
{

}
void CNullLogger::Fatalf(const char *file, int line, const char *fmt, ...)
{

}
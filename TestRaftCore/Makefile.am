AUTOMAKE_OPTIONS = subdir-objects foreign

noinst_PROGRAMS = \
	TestRaftCore

TestRaftCore_CXXFLAGS =-DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -DLIBRAFTCORE_DLL -I ../libRaftCore

TestRaftCore_SOURCES = \
	NullLogger.cpp \
	TestFlowControllerFixture.cpp \
	TestMemStorageFixture.cpp \
	TestProgressFixture.cpp \
	TestRaftCore.cpp \
	TestRaftFixtrue.cpp \
	TestRaftFrame.cpp \
	TestRaftLogFixture.cpp \
	TestRaftPaperFixture.cpp \
	TestRaftUtil.cpp \
	TestUnstableLogFixture.cpp

TestRaftCore_LDADD = \
	-L../libRaftCore/.libs -lRaftCore -lcppunit
TestServer_CXXFLAGS =-I ../libRaftCore


AM_CPPFLAGS = 
AM_CXXFLAGS = -Wall -Wextra -pedantic
AM_LDFLAGS = 


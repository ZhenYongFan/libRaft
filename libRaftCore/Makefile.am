AUTOMAKE_OPTIONS=foreign subdir-objects
lib_LTLIBRARIES = libRaftCore.la

libRaftCore_la_CXXFLAGS =-fvisibility=hidden -fvisibility-inlines-hidden -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -DLIBRAFTCORE_DLL -DLIBRAFTCORE_EXPORTS
libRaftCore_la_CFLAGS =-fvisibility=hidden -DLINUX=2 -D_REENTRANT -D_GNU_SOURCE -DLIBRAFTCORE_DLL -DLIBRAFTCORE_EXPORTS
libRaftCore_la_LDFLAGS = -version-number 1:0:0 -lrt -lcrypt  -lpthread

libRaftCore_la_SOURCES = \
	Progress.cpp \
	Raft.cpp \
	RaftConfig.cpp \
	RaftEntry.cpp \
	RaftMemLog.cpp \
	RaftMemStorage.cpp \
	RaftQueue.cpp \
	RaftSemaphore.cpp \
	RaftSerializer.cpp \
	RaftUtil.cpp \
	ReadOnly.cpp \
	UnstableLog.cpp

// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#ifdef _WIN64
	#include "targetver.h"
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

    #define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
    // Windows 头文件: 
    #include <windows.h>

    #pragma warning(disable:4290)
    #pragma warning(disable:4275)
    #pragma warning(disable:4251)
    #include <stdio.h>
    #include <tchar.h>
    #include <WinSock2.h>
#else
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//stl
#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>

//cxx11
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>

//libevent
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

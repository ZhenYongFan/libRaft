#include "stdafx.h"

#include "network/RaftServer.h"
#include "RaftFrame.h"

int Test(void)
{
    int nTest = 0;
    std::string strConfigPath ;
#ifdef _WIN64
#else
    strConfigPath = get_current_dir_name();
#endif
    std::string strErrMsg;
    CRaftFrame frame;
    if (frame.Init(strConfigPath, strErrMsg))
    {
        CRaftServer server;
        if (server.Init(&frame))
        {
            if (0 == server.Start())
            {
                int nInput = 0;
                while (nInput != 1)
                {
                    printf("Input a char : ");
                    scanf("%d", &nInput);
                    if (2 == nInput)
                    {

                    }
                }
                server.NotifyStop();
                server.Stop();
            }
            else
                nTest = 3;
            server.Uninit();
        }
        else
            nTest = 2;
        frame.Uninit();
    }
    else
        nTest = 1;
    return 0;
}





int main(void)
{
#ifndef _WIN64
    evthread_use_pthreads();
#endif
    Test();
    return 0;
}

#include "stdafx.h"
#include <time.h>
#include "RaftSemaphore.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRaftSemaphore::CRaftSemaphore(void)
{
    m_hSem = NULL;
}

CRaftSemaphore::~CRaftSemaphore(void)
{
    Destroy();
}

bool CRaftSemaphore::Create(int nInitValue)
{
#ifdef WIN32
    m_hSem = CreateSemaphore(
        NULL,           // default security attributes
        nInitValue,     // initial count
        0xFFFF,         // maximum count
        NULL);          // unnamed semaphore
#else
    m_hSem = (sem_t *)malloc(sizeof(sem_t));
    if (0 != sem_init(m_hSem, 0, nInitValue))
        m_hSem = NULL;

#endif
    return (NULL != m_hSem);
}

void CRaftSemaphore::Destroy(void)
{
    if (NULL != m_hSem)
    {
#ifdef WIN32
        CloseHandle(m_hSem);
#else
        free(m_hSem);
        sem_destroy(m_hSem);
#endif
        m_hSem = NULL;
    }
}

uint32_t CRaftSemaphore::Wait(uint32_t dwMilliseconds)
{
#ifdef WIN32
    return WaitForSingleObject(m_hSem, dwMilliseconds);
#else
    uint32_t nWait;

    do
    {
        if (0 == dwMilliseconds)
            nWait = sem_trywait(m_hSem);
        else if (INFINITE == dwMilliseconds)
            nWait = sem_wait(m_hSem);
        else
        {
            struct timespec abs_timeout; //绝对时间
            if (0 == clock_gettime(CLOCK_REALTIME, &abs_timeout))
            {
                abs_timeout.tv_sec = time(NULL) + dwMilliseconds / 1000;
                abs_timeout.tv_nsec += (dwMilliseconds % 1000) * 1000 * 1000;
                if (abs_timeout.tv_nsec > 1000000000)
                {
                    abs_timeout.tv_nsec -= 1000000000;
                    abs_timeout.tv_sec++;
                }
                nWait = sem_timedwait(m_hSem, &abs_timeout);
            }
            else
                nWait = WAIT_ABANDONED;
        }
        //Restart when interrupted by handler
    } while ((-1 == nWait) && (EINTR == errno));

    if (0 == nWait)
        nWait = WAIT_OBJECT_0;
    else
    {
        if (ETIMEDOUT == errno)
            nWait = WAIT_TIMEOUT;
        else
            nWait = WAIT_ABANDONED;
    }

    return nWait;
#endif
}

bool CRaftSemaphore::Post(int nCount)
{
    bool bOK = false;
#ifdef WIN32
    bOK = (TRUE == ReleaseSemaphore(m_hSem, nCount, NULL));
#else

    for (int nIndex = 0; nIndex < nCount; nIndex++)
    {
        if (sem_post(m_hSem) >= 0)
            bOK = true;
        else
        {
            bOK = false;
            break;
        }
    }

#endif
    return bOK;
}

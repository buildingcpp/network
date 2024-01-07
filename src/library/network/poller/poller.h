#pragma once

#if defined(_WIN32)
    #if !defined (USE_KQUEUE)
        #define USE_KQUEUE
    #endif
#endif

#if defined(USE_KQUEUE)
    #include "./kpoller.h"
#else 
    #include "./epoller.h"
#endif


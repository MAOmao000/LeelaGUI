#ifndef TIMING_H_INCLUDED
#define TIMING_H_INCLUDED

#include "config.h"

#include <time.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

class Time {    
public:    
    /*
        sets to current time
    */            
    Time(void);
    
    /*  
        time difference in centiseconds
    */        
    static int timediff(Time start, Time end);
    
private:
    rtime_t m_time;    
};

#endif

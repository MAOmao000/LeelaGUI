#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED

/*  Timing code. Define one or none of:
 *
 *  GETTICKCOUNT, GETTIMEOFDAY
 */
#ifdef _WIN32
#define GETTICKCOUNT
#undef HAVE_SELECT
#define NOMINMAX
#else
#define HAVE_SELECT
#define GETTIMEOFDAY
#endif

/* Hard limits */

/* Features */
//#define KGS
#define USE_OPTIONS
//#define USE_BLAS
//#define USE_OPENBLAS
//#define USE_MKL
//#define USE_CAFFE
#define USE_OPENCL
//#define USE_TUNER
#define USE_SEARCH

#define PROGRAM_NAME "Leela"
#ifdef KGS
#define PROGRAM_VERSION "0.11.0. I will resign when I have lost. If you are sure you are winning but I haven't resigned yet, the status of some groups is not yet clear to me. I will pass out the game when I have won. You can download the latest stable version at https://sjeng.org/leela"
#else
#define PROGRAM_VERSION "0.11.0"
#endif

// OpenBLAS limitation
#if defined(USE_BLAS) && defined(USE_OPENBLAS)
#define MAX_CPUS 64
#else
#define MAX_CPUS 128
#endif

/* Integer types */

typedef int int32;
typedef short int16;
typedef signed char int8;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

/* Data type definitions */

#ifdef _WIN32
typedef __int64 int64 ;
typedef unsigned __int64 uint64;
#else
typedef long long int int64 ;
typedef  unsigned long long int uint64;
#endif

#if (_MSC_VER >= 1400) /* VC8+ Disable all deprecation warnings */
    #pragma warning(disable : 4996)
#endif /* VC8+ */

#ifdef GETTICKCOUNT
    typedef int rtime_t;
#else
    #if defined(GETTIMEOFDAY)
        #include <sys/time.h>
        #include <time.h>
        typedef struct timeval rtime_t;
    #else
        typedef time_t rtime_t;
    #endif
#endif

#endif

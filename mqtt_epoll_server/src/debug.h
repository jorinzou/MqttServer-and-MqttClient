#ifndef _DEBUG_H_
#define _DEBUG_H_
#include <stdint.h>
#include <sys/syscall.h>
#include <stdarg.h>


#define USE_DEBUG
#ifdef USE_DEBUG
#define DEBUG_LINE()            printf("[%s:%s] line=%d\r\n",__FILE__, __FUNCTION__, __LINE__)
#define DEBUG_ERR(fmt, args...) printf("\033[46;31m[%s:%d]\033[0m "#fmt" errno=%d, %m\r\n", __FUNCTION__, __LINE__, ##args, errno, errno)
#define DEBUG_INFO(fmt, args...) printf("\033[33m[%s:%s:%d]\033[0m "#fmt"\r\n", __FILE__,__FUNCTION__, __LINE__, ##args)
#else	
#define DEBUG_LINE()
#define DEBUG_ERR(fmt,...)
#define DEBUG_INFO(fmt,...)
#endif

#endif


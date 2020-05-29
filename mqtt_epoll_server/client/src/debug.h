#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdio.h>


#define DEBUG_INFO(fmt, args...) printf("\033[33m[%s:%d]\033[0m "#fmt"\r\n", __FUNCTION__, __LINE__, ##args)


#endif

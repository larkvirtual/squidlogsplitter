#ifndef __types_h__
#define __types_h__

#include <sys/types.h>

#ifdef __linux__
/* используется не-ISO тип, т.к. GNU/Linux по данному типу не совместим с ISO */
#define my_uint32_t u_int32_t
#else
/* используется ISO тип, т.к. FreeBSD по данному типу совместима с ISO */
#define my_uint32_t uint32_t
#endif

#endif

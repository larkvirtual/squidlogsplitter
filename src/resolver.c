/* Copyright (C) Vladimir <virtual.lark@gmail.com>, 2004-2017

    SLS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2, or (at your option) any later
    version. */

#include <sys/types.h>	/* FreeBSD 4 */
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>	/* memset() */
#include <stdio.h>	/* printf() */
#include <arpa/inet.h>	/* inet_addr() */
#include "resolver.h"

/* рудимент, искореню как будет время */
int	resolve_to_uint32(char *url, my_uint32_t *ip){
struct hostent *hp;

hp = gethostbyname(url);
if(NULL == hp){
	*ip = inet_addr(url);
	if(-1 == (int)*ip) return 0;
	else {/* возможно тут надо htonl() при xx.xx.xx.xx */return 0;}
	}
memcpy(ip, hp->h_addr, 4);
*ip = htonl(*ip);

return 0;
}

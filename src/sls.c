/* Copyright (C) Vladimir <virtual.lark@gmail.com>, 2004-2017

    SLS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2, or (at your option) any later
    version. */

#include <stdio.h>	/* puts() */
#include <string.h>	/* strchr() */
#include <stdlib.h>	/* exit() */
#include "types.h"	/* my_uint32_t */
#include "resolver.h"

#define NAME "sls"
#define BRANCH "1.1"
#define PATCH "7" /* 1.1p7 */

/* (C) Vladimir, 2004 */
/*************************************/
/* WARMMMMING: dirty hacked programm */
/* написана до наступления тёмного времени суток за очень короткое время в
 * серверной папы карло на череме :) */
/*************************************/

/* тип сеть/маска */
typedef struct{
my_uint32_t	net;	/* вида 217.29.80.0 */
my_uint32_t	mask;	/* вида 0xffffff00 */
}net_mask_t;
/* тип СЕТЬ */

#define MAX_NETWORKS 512
/* хватит на ближайшее будущее, при необходимости
увеличить */
typedef struct{
net_mask_t	net_mask[MAX_NETWORKS];
size_t		n_networks; /* число сетей */
}network_t;
/* тип НАБОР СЕТЕЙ */

int	nw_load(network_t *nw, char *networks_file);
/* Заполняет набор сетей из файла
< 0 при ошибке */

void	load_net_mask(net_mask_t *nmask, char *buf);
/* Заполняет запись сети по строке вида сеть/маска */

int	split_core(network_t *networks, char *squid_log, char *inet_outlog, char *local_outlog);
/* Ядро программы, гоняет строки по файлам в зависимости от принадлежности ip */

int	is_from_netlist(my_uint32_t ip, network_t *networks);
/* true если ip принадлежит одной из сетей
false в противном случае */

#ifdef DEBUG
void	print_ip_dec(my_uint32_t ip);
/* печатает ip в формате xxx.xxx.xxx.xxx */
#endif

int main(int argc, char **argv){
int	retval;
network_t	nw;
if(argc != 5){
	printf("%s %sp%s\n", NAME, BRANCH, PATCH);
	printf("Usage: %s <squid_log> <local_nets> <local_outlog> <inet_outlog>\n", NAME);
	return -2;
	}
retval = nw_load(&nw, argv[2]);
if(retval < 0){printf("Local nets (%s) file loading error\n", argv[2]);
	return retval;
	}

 /* входной, выходной-local, выходной-inet */
retval = split_core(&nw, argv[1], argv[3], argv[4]);
if(retval < 0){
	puts("Splitting error");
	return -1;
	}

return 0;
}

int	nw_load(network_t *nw, char *networks_file){
FILE	*file;
#define NWORK_BUF_LEN 40
char	buf[NWORK_BUF_LEN];
char	*tmprez;
int	done;

nw->n_networks = 0;
done = 0;

file = fopen(networks_file, "rb");
if(NULL == file){perror("fopen()"); return -1;}

while(0 == done){
	tmprez = fgets(buf, NWORK_BUF_LEN, file);
	if(NULL == tmprez){/* конец файла или типа того (C) fgets() */
		done = 1;
		if(0 == strlen(buf)){
			nw->n_networks++;
			break;
			}
		}
	load_net_mask(&nw->net_mask[nw->n_networks], buf);
	nw->n_networks++;
	if(nw->n_networks >= MAX_NETWORKS){
puts("Fatal error. Actual number of networks bigger, then hardcoded!");
puts("You should increase MAX_NETWORKS value!");
		fclose(file);
		return -2;
		}
	}
nw->n_networks--; /* т.к. при done = 1 происходит nw->n_networks++ */
#ifdef DEBUG
printf("%s(): networks loaded: %zd\n", __FUNCTION__, nw->n_networks);
#endif

fclose(file);
return 0;
}

void	load_net_mask(net_mask_t *nmask, char *buf){
my_uint32_t	first, second, theird, fourth;
my_uint32_t		mlen; /* /маска */

sscanf(buf, "%u.%u.%u.%u", &first, &second, &theird, &fourth);
nmask->net = (first << 24) | (second << 16) | (theird << 8) | fourth;
sscanf(strchr(buf, '/'), "/%u\n", &mlen);
nmask->mask = (1 << (32 - mlen)) - 1;
nmask->mask ^= (my_uint32_t)(-1);

#ifdef DEBUG
printf("DEBUG: %u.%u.%u.%u\n", first, second, theird, fourth);
printf("DEBUG: /%u\n", mlen);
printf("DEBUG: 0x%08X\n", nmask->mask);
#endif
}

int	split_core(network_t *networks, char *squid_log, char *local_outlog, char *inet_outlog){
FILE	*squid, *tomsk, *internet;
#define SQUID_BUF 10240 /* думается хватит */
#define URL_BUF 5120 /* думается хватит */
char	squid_line[SQUID_BUF];
char	url_line[URL_BUF];
char	*tmpstr;
int	done;
int	retval;
my_uint32_t ip;
size_t	write_size;

done = 0;

squid = fopen(squid_log, "rb");
if(NULL == squid){perror("fopen()"); return -1;}

tomsk = fopen(local_outlog, "wb");
if(NULL == tomsk){perror("fopen()"); fclose(squid); return -1;}

internet = fopen(inet_outlog, "wb");
if(NULL == internet){perror("fopen()"); fclose(squid); fclose(tomsk); return -1;}

while(0 == done){
	tmpstr = fgets(squid_line, SQUID_BUF, squid);
	if(NULL == tmpstr){/* конец лога или типа того (C) fgets() */
		fclose(squid); fclose(tomsk); fclose(internet);
		return 0;}

#define DIRECT_TAG " DIRECT/"
#define DEFAULT_PARENT_TAG " DEFAULT_PARENT/"
#define ANY_PARENT_TAG " ANY_PARENT/"
	tmpstr = strstr(squid_line, DIRECT_TAG);
	if(NULL == tmpstr){
		tmpstr = strstr(squid_line, DEFAULT_PARENT_TAG);
		if(NULL == tmpstr){
			tmpstr = strstr(squid_line, ANY_PARENT_TAG);
			if(NULL == tmpstr){/* нет никаких подстрок
				просто игнорируем её */ goto end;
				}
			else{tmpstr += strlen(ANY_PARENT_TAG);}
			}
		else{tmpstr += strlen(DEFAULT_PARENT_TAG);}
		}
	else{tmpstr += strlen(DIRECT_TAG);}
		
		/* url -> ip -> check ip to zone -> fwrite() to ... END */
		sscanf(tmpstr, "%s \n", url_line);
		retval = resolve_to_uint32(url_line, &ip);
		if(retval){
			printf("*** NYI %s:%u\n", __FILE__, __LINE__); exit(-1);
			}
		retval = is_from_netlist(ip, networks);
		if(retval){
#ifdef DEBUG
printf("%s (ip) from tomsk\n\n", url_line);
#endif
			write_size = fwrite(squid_line, strlen(squid_line), 1,
			tomsk);
			if(0 == write_size){
				perror("fwrite()");
				fclose(squid); fclose(tomsk); fclose(internet);
				return -1;
				}
			}
		else	{
#ifdef DEBUG
printf("%s (ip) from internet\n\n", url_line);
#endif
			write_size = fwrite(squid_line, strlen(squid_line), 1,
			internet);
			if(0 == write_size){
				perror("fwrite()");
				fclose(squid); fclose(tomsk); fclose(internet);
				return -1;
				}
			}
	end:{}
	}

fclose(squid);
fclose(tomsk);
fclose(internet);

return 0;
}

int	is_from_netlist(my_uint32_t ip, network_t *networks){
size_t	t;

for(t = 0; t < networks->n_networks; t++){
#ifdef DEBUG
printf("ip = ");print_ip_dec(ip);
printf(" & ");print_ip_dec(networks->net_mask[t].mask);
printf(" = ");print_ip_dec(networks->net_mask[t].mask & ip);
puts("");
#endif
	if((networks->net_mask[t].mask & ip) == networks->net_mask[t].net)
		{
#ifdef DEBUG
print_ip_dec(ip); printf(" from ");
print_ip_dec(networks->net_mask[t].net);
puts("");
#endif
		return 1;
		}
#ifdef DEBUG
	else	{
		print_ip_dec(networks->net_mask[t].mask & ip);
		printf(" != ");
		print_ip_dec(networks->net_mask[t].net);
		puts("");
		}
#endif
	}
#ifdef DEBUG
print_ip_dec(ip); printf(" not from TOMSK\n");
#endif
return 0;
}

#ifdef DEBUG
void	print_ip_dec(my_uint32_t ip){
	unsigned char a, b, c, d;
	
	a = (unsigned char)(ip >> 24);
	b = (unsigned char)(ip >> 16);
	c = (unsigned char)(ip >> 8);
	d = (unsigned char)(ip);
	
	printf("%u.%u.%u.%u", a, b, c, d);
}
#endif

#ifndef XPWN_H
#define XPWN_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <stdint.h>

typedef struct{
	pid_t pid;
	int stdin;
	int stdout;
	int stderr;
}PROC;

/* Swaps order of bytes (16 bit integers) */
uint16_t swap_b16(uint16_t n);

/* Swaps order of bytes (32 bit integers)*/
uint32_t swap_b32(uint32_t n);

/* Swaps order of bytes (64 bit integers)*/
uint64_t swap_b64(uint64_t n);

/* Simple hexdump. Width refers to how many bytes to print per line */
void hexdump(unsigned char *buf, int len, int width);

/* Forks() a new process specified by path.   */
/* nonblocking is for the process descriptors */
PROC *process(char *path, char **args, char **envp, unsigned int nonblocking);

/* Kill process and free its PROC structure */
int reap_process(PROC *proc);

typedef struct{
	char *hostname;
	int domain;
	char *ipv4;
	char *ipv6;
	int type;
	int sock;
}REMOTE;

/* Closes socket and frees REMOTE structure */
void remote_close(REMOTE *r);

/* Opens a new connection to target:port.           */
/* Arguments domain and type are passed to socket() */
REMOTE *remote(char *target, unsigned short port, int domain, int type);

void shell(int fdin, int fdout, int fderr);
#endif

#ifndef EXDEV_H
#define EXDEV_H

#define _GNU_SOURCE

#include <sys/types.h>
#include <stdint.h>

typedef struct{
	pid_t pid;
	int stdin;
	int stdout;
	int stderr;
}process_t;

uint16_t swap_b16(uint16_t n);

uint32_t swap_b32(uint32_t n);

uint64_t swap_b64(uint64_t n);

void hexdump(unsigned char *buf, int len, int width);

process_t *process(char *path, char **args, char **envp, unsigned int nonblocking);

int reap_process(process_t *proc);

typedef struct{
	char *hostname;
	int domain;
	char *ipv4;
	char *ipv6;
	int type;
	int sock;
}remote_t;

void remote_close(remote_t *r);

remote_t *remote(char *target, unsigned short port, int domain, int type);

void shell(int fdin, int fdout, int fderr);

long readline(int fd, char *buf, long size);

long pread_fd(int fd, void *buf, long size);

#endif

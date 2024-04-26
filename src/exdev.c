#include "../include/exdev.h"

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <termios.h>

#define TIMEOUT 5

process_t *process(char *path, char **args, char **envp, unsigned int nonblocking){
	process_t *ret = NULL;
	int out[2] = {0,0};
	int in[2] = {0,0};
	int err[2] = {0,0};

	if(!(ret = (process_t *)calloc(1, sizeof(process_t)))){
		fprintf(stderr, "%s:%d:calloc(): %s\n", __FILE__, __LINE__, strerror(errno));
		return NULL;
	}

	if(pipe2(err, nonblocking) || pipe2(out, O_NONBLOCK) || pipe2(in, nonblocking)){
		fprintf(stderr, "%s:%d:calloc(): %s\n", __FILE__, __LINE__, strerror(errno));
		free(ret);
		return NULL;
	}

	switch((ret->pid = fork())){
		case -1:
			fprintf(stderr, "%s:%d:fork(): %s\n", __FILE__, __LINE__, strerror(errno));
			free(ret);
			return NULL;
		case 0:
			dup2(err[1], 2);
			dup2(out[1], 1);
			dup2(in[0], 0);
			execve(path, args, envp);
			_exit(0);
	}
	close(err[1]);
	close(out[1]);
	close(in[0]);
	ret->stderr = err[0];
	ret->stdout = out[0];
	ret->stdin = in[1];
	return ret;
}

int reap_process(process_t *proc){
	int status = 0;
	if(proc && proc->pid > 0){
		close(proc->stdout);
		close(proc->stdin);
		close(proc->stderr);
		kill(proc->pid, SIGTERM);
		waitpid(proc->pid, &status, 0);
		memset(proc, 0, sizeof(process_t));
		free(proc);
	}
	return WEXITSTATUS(status);
}

void remote_close(remote_t *r){
	if(!r) return;
	if(r->ipv4) free(r->ipv4);
	if(r->ipv6) free(r->ipv6);
	if(r->hostname) free(r->hostname);
	close(r->sock);
	memset(r, 0, sizeof(remote_t));
	free(r);
	return;
}

remote_t *remote(char *target, unsigned short port, int domain, int type){
	remote_t *r;
	struct hostent *hst;
	struct sockaddr_in dst;
	struct sockaddr_in6 dst6;

	if(!target || port == 0) return NULL;
	memset(&dst, 0, sizeof(struct sockaddr_in));
	memset(&dst6, 0, sizeof(struct sockaddr_in6));	

	r = (remote_t *)calloc(1, sizeof(remote_t));
	if(r){
		r->sock = socket(domain, type, 0);
		if(r->sock < 0){
			free(r);
			return NULL;
		}
		if(!(hst = gethostbyname2(target, domain))) goto REMOTE_ERR1;
		if(!(r->hostname = strdup(target))) goto REMOTE_ERR1;

		switch(domain){
			case AF_INET:
				r->ipv4 = (char *)malloc(INET_ADDRSTRLEN+1);
				if(!r->ipv4) goto REMOTE_ERR1;

				memset(r->ipv4, 0, INET_ADDRSTRLEN+1);
				inet_ntop(AF_INET, hst->h_addr_list[0], r->ipv4, INET_ADDRSTRLEN);	
				if(type != SOCK_STREAM) return r;
	
				dst.sin_family = AF_INET;
				dst.sin_port = htons(port);
				dst.sin_addr.s_addr = *(unsigned int *)(hst->h_addr_list[0]);
				
				if(connect(r->sock, (struct sockaddr *)&dst, sizeof(struct sockaddr)) == 0) return r;
				remote_close(r);
				return NULL;
			case AF_INET6:
				r->ipv6 = (char *)malloc(INET6_ADDRSTRLEN+1);
				if(!r->ipv6) goto REMOTE_ERR1;

				memset(r->ipv6, 0, INET6_ADDRSTRLEN+1);
				inet_ntop(AF_INET6, hst->h_addr_list[0], r->ipv6, INET6_ADDRSTRLEN);
				if(type != SOCK_STREAM) return r;

				dst6.sin6_family = AF_INET6;
				dst6.sin6_port = htons(port);
				memcpy(&dst6.sin6_addr, hst->h_addr_list[0], 16);

				if(connect(r->sock, (struct sockaddr *)&dst6, sizeof(struct sockaddr_in6)) == 0) return r;
				remote_close(r);
				return NULL;
			default:
				fprintf(stderr, "%s:%d:remote(): Invalid domain provided\n", __FILE__, __LINE__);
REMOTE_ERR1:
				if(errno) fprintf(stderr, "%s:%d:remote(): %s\n", __FILE__, __LINE__, strerror(errno));
				if(r->hostname) free(r->hostname);
				close(r->sock);
				free(r);
				return NULL;
		}
	}
	return NULL;
}

long readline(int fd, char *buf, long size){
	struct timeval tv;
	fd_set rfd;
	long index = 0;

	if(!buf || size == 0) return 0;
	memset(&tv, 0, sizeof(struct timeval));
	FD_ZERO(&rfd);

	FD_SET(fd, &rfd);
	tv.tv_sec = TIMEOUT;

	while(index < size){
		switch(select((fd+1), &rfd, NULL, NULL, &tv)){
			case -1:
				fprintf(stderr, "%s:%d:readline(): %s\n", __FILE__, __LINE__, strerror(errno));
				return -1;
			case 0:
				return index;
			default:
				if(read(fd, &buf[index], 1) == 1){
					if(buf[index] == '\n') return ++index;
					index++;
				}
				break;
		}
	}
	return index;
}

long pread_fd(int fd, void *buf, long size){
	struct timeval timeout = {0,150000};
	long index = 0;
	fd_set fset;
	int r;
	
	if(!buf || size <= 0) return -1;
	
	FD_ZERO(&fset);
	while(index < size){
		FD_SET(fd, &fset);
		if((r = select(fd+1, &fset, NULL, NULL, &timeout)) == 1 && FD_ISSET(fd, &fset)){
			if(read(fd, &((char *)(buf))[index++], 1) == 0) return index;
		}else if(r <= 0) return index;
	}
	return index;
}

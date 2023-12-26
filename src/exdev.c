#include "../include/exdev.h"

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <termios.h>

PROC *process(char *path, char **args, char **envp, unsigned int nonblocking){
	PROC *ret = NULL;
	int stdout_p[2] = {0,0};
	int stdin_p[2] = {0,0};
	int stderr_p[2] = {0,0};
	int pipe_flag = 0;
	pid_t pid = 0;

	if(nonblocking) pipe_flag = O_NONBLOCK;

	ret = (PROC *)malloc(sizeof(PROC));
	if(ret){
		memset(ret, 0, sizeof(PROC));
		if(pipe2(stdout_p, pipe_flag) < 0 || pipe2(stdin_p, pipe_flag) < 0 || pipe2(stderr_p, pipe_flag) < 0){
			fprintf(stderr, "%s:%d:pipe(): %s\n", __FILE__, __LINE__, strerror(errno));
			free(ret);
			return NULL;
		}
		pid = fork();
		if(pid == 0){
			dup2(stdout_p[1], STDOUT_FILENO);
			dup2(stdin_p[0], STDIN_FILENO);
			dup2(stderr_p[1], STDERR_FILENO);
			execve(path, args, envp);
			_exit(0);
		}
		else if(pid < 0){
			fprintf(stderr, "%s:%d:fork(): %s\n", __FILE__, __LINE__, strerror(errno));
			free(ret);
			return NULL;
		}
		else{
			ret->pid = pid;
			close(stdout_p[1]);
			close(stdin_p[0]);
			close(stderr_p[1]);
			ret->stdin = stdin_p[1];
			ret->stdout = stdout_p[0];
			ret->stderr = stderr_p[0];
			return ret;
		}

	}
	return NULL;
}

int reap_process(PROC *proc){
	int status = 0;
	if(proc && proc->pid > 0){
		close(proc->stdout);
		close(proc->stdin);
		close(proc->stderr);
		kill(proc->pid, SIGTERM);
		waitpid(proc->pid, &status, 0);
		memset(proc, 0, sizeof(PROC));
		free(proc);
	}
	return WEXITSTATUS(status);
}

void remote_close(REMOTE *r){
	if(r){
		if(r->ipv4) free(r->ipv4);
		if(r->ipv6) free(r->ipv6);
		if(r->hostname) free(r->hostname);
		close(r->sock);
		memset(r, 0, sizeof(REMOTE));
		free(r);
	}
	return;
}

REMOTE *remote(char *target, unsigned short port, int domain, int type){
	REMOTE *r = NULL;
	struct hostent *hst = NULL;
	struct sockaddr_in dst;
	struct sockaddr_in6 dst6;

	if(!target || port == 0) return NULL;
	memset(&dst, 0, sizeof(struct sockaddr_in));
	memset(&dst6, 0, sizeof(struct sockaddr_in6));	

	r = (REMOTE *)malloc(sizeof(REMOTE));
	if(r){
		memset(r, 0, sizeof(REMOTE));
		r->sock = socket(domain, type, 0);
		if(r->sock < 0){
			free(r);
			return NULL;
		}
		hst = gethostbyname2(target, domain);
		if(!hst) goto REMOTE_ERR1;
		r->hostname = strdup(target);
		if(!r->hostname) goto REMOTE_ERR1;

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

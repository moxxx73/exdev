# exdev
C library for developing exploits/CTF solutions

## Installing ExDev
You can build and install ExDev with `make`:

```
make && sudo make install
```

This also builds a static library which (as well as the shared library) are stored in a newly created `build/` directory. From there `make` creates 
**symlinks** instead of moving the libraries. feel free to change this

## Functions
Process related functions:
```C
PROC *process(char *path, char **args, char **envp, unsigned int nonblocking);
int reap_process(PROC *proc);
```
The `process()` function spawns a new process and executes a target binary specified by `path`. The `reap_process()` function kills the child process
and frees the `PROC` structure allocated and returned by `process()`  

Network related functions:
```C
REMOTE *remote(char *target, unsigned short port, int domain, int type);
void remote_close(REMOTE *r);
```
The `remote()` functions opens a connection to `target:port` and the `remote_close()` function frees the `REMOTE` structure returned by `remote()`  

Misc. functions:
```C
uint16_t swap_b16(uint16_t n);
uint32_t swap_b32(uint32_t n);
uint64_t swap_b64(uint64_t n);
void hexdump(unsigned char *buf, int len, int width);
```

## Data Types
Created by `process()`:
```C
typedef struct{
	pid_t pid;
	int stdin;
	int stdout;
	int stderr;
}PROC;
```

Created by `remote()`:
```C
typedef struct{
	char *hostname;
	int domain;
	char *ipv4;
	char *ipv6;
	int type;
	int sock;
}REMOTE;
```

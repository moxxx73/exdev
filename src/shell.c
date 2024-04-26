#include "../include/exdev.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <termios.h>
#include <sys/select.h>

struct termios save_term;
int set_restore = 0;

void restore_term(void){
	tcsetattr(1, TCSAFLUSH, &save_term);
	return;
}

void init_term(void){
	struct termios raw;

	memset(&raw, 0, sizeof(struct termios));
	tcgetattr(1, &raw);
	memcpy(&save_term, &raw, sizeof(struct termios));

	raw.c_lflag &= ~(ECHO|ICANON);

	tcsetattr(1, TCSAFLUSH, &raw);
	if(!set_restore){
		atexit(restore_term);
		set_restore = 1;
	}
	return;
}

char read_char(void){
	char c = 0;
	if(read(1, &c, 1) == 1) return c;
	return 0;
}

#define CLR_LINE write(1, "\x1b[2K", 4)
#define CUR_SOL write(1, "\x1b[1000D", 7)
#define HIDE_CURSOR write(1, "\x1b[?25l", 6)
#define SHOW_CURSOR write(1, "\x1b[?25h", 6)
char *get_cmd(char *prompt){
	char setcur[32];
	char input[256], c = 0;
	int index = 0;
	int cur = 0;

	memset(input, 0, 256);

	for(;;){
		memset(setcur, 0, 32);

		HIDE_CURSOR;
		CLR_LINE;
		CUR_SOL;
		printf("%s%s", prompt, input);
		SHOW_CURSOR;
		fflush(stdout);
		CUR_SOL;
		snprintf(setcur, 31, "\x1b[%ldC", (strlen(prompt)+cur));
		write(1, setcur, strlen(setcur));
		fflush(stdout);

		c = read_char();
		if(c == '\n'){
			if(index < 255){
				input[index] = '\n';
				input[++index] = '\n';
				break;
			}
		}
		else if(c >= 32 && c < 127){
			if(cur <= index && index < 255){
				index++;
				if((cur+2) <= index) memmove(&input[cur+1], &input[cur], 1+((index > cur) ? index-cur : cur-index));
				input[cur++] = c;
			}
		}
		else if(c == 127 && (index > 0 && cur > 0)){
			memmove(&input[cur-1], &input[cur], 1+((index > cur) ? index-cur : cur-index));
			cur--;
			index--;
		}
		else if(c == '\x1b'){
			c = read_char();
			if(c == '['){
				c = read_char();
				if(c == 'D'){
					if(cur > 0) cur--;
				}else if(c == 'C'){
					if(cur < index) cur++;
				}
			}
		}
		else if(c == 12){
			write(1, "\x1b[2J", 4);
			write(1, "\x1b[0;0H", 6);
		}
	}

	return strdup(input);
}

void shell(int fdin, int fdout, int fderr){
	struct timeval timeout = {0, 150000}; // 150000
	fd_set rd_fds;
	char out[256];
	char *cmd;
	int nfds, index;

	(void)fderr;
	init_term();

	for(;;){
		cmd = get_cmd("$ ");
		write(1, "\n", 1);
		if(!cmd){
			fprintf(stderr, "%s:%d:get_cmd(): %s\n", __FILE__, __LINE__, strerror(errno));
			return;
		}
		if(!strncmp(cmd, "kill_shell", strlen("kill_shell"))){
			restore_term();
			free(cmd);
			return;
		}

		FD_ZERO(&rd_fds);
		do{
			FD_SET(fdin, &rd_fds);
		}while(select(fdin+1, NULL, &rd_fds, NULL, &timeout) == 0);
		write(fdin, cmd, strlen(cmd));
		FD_ZERO(&rd_fds);
		FD_SET(fdout, &rd_fds);
		while(select(fdout+1, &rd_fds, NULL, NULL, &timeout) == 0) FD_SET(fdout, &rd_fds);

		FD_ZERO(&rd_fds);
		memset(out, 0, 256);
		for(index = 0;;){
			FD_SET(fdout, &rd_fds);
			if((nfds = select(fdout+1, &rd_fds, NULL, NULL, &timeout)) > -1){
				if(nfds == 0) break;
				if(index == 255){
					write(1, out, 255);
					memset(out, 0, 255);
					index = 0;
				}
				if(read(fdout, &out[index++], 1) == 0) break;
			}else if(nfds == 0) break;
		}
		write(1, out, index+1);

		free(cmd);
		cmd = NULL;
	}
	restore_term();
	return;
}

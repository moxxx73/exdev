#include "../include/exdev.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>
#include <termios.h>

struct termios save_term;

void init_term(void){
    struct termios raw;

    memset(&raw, 0, sizeof(struct termios));
    tcgetattr(1, &raw);
    memcpy(&save_term, &raw, sizeof(struct termios));

    raw.c_lflag &= ~(ECHO|ICANON);

    tcsetattr(1, TCSAFLUSH, &raw);
    return;
}

void restore_term(void){
    tcsetattr(1, TCSAFLUSH, &save_term);
    return;
}

char read_char(void){
    char c = 0;
    if(read(1, &c, 1) == 1) return c;
    return 0;
}

#define CLR_LINE() (write(1, "\x1b[2K", 4))
#define CUR_SOL() (write(1, "\x1b[1000D", 7))
#define HIDE_CURSOR() (write(1, "\x1b[?25l", 6))
#define SHOW_CURSOR() (write(1, "\x1b[?25h", 6))
#define PROMPT "host> "
char *get_cmd(void){
    char setcur[32];
    char input[256], c = 0;
    int index = 0;
    int cur = 0;
    
    memset(input, 0, 256);

    for(;;){
        memset(setcur, 0, 32);
        
        HIDE_CURSOR();
        CLR_LINE();
        CUR_SOL();
        printf("%s%s%s%s", "\x1b[31;1m", PROMPT, "\x1b[0m", input);
        SHOW_CURSOR();
        fflush(stdout);
        CUR_SOL();
        snprintf(setcur, 31, "\x1b[%ldC", (strlen(PROMPT)+cur));
        write(1, setcur, strlen(setcur));
        fflush(stdout);

        c = read_char();
        if(c == '\n'){
            if(index < 255) input[index] = '\n';
            break;
        }
        else if(c >= 32 && c < 127){
            //if(index < 255) input[index++] = c;
            if(cur <= index){
                if(index < 255) index++;
                if((cur+2) <= index) memmove(&input[cur+1], &input[cur], 255-strlen(&input[cur+1])-1);
                input[cur++] = c;
            }
        }
        else if(c == 127){
            if(index > 0 && cur > 0){
                memmove(&input[cur-1], &input[cur], 254-strlen(&input[cur])-1);
                cur--;
                index--;
            }
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
    printf("\x1b[0m\n");
    
    return strdup(input);
}

void shell(int fdin, int fdout, int fderr){
    char *cmd = NULL;
    char out[256];

    init_term();
    atexit(restore_term);

    for(;;){
        memset(out, 0, 256);
        cmd = get_cmd();
        if(!cmd){
            fprintf(stderr, "%s:%d:get_cmd(): %s\n", __FILE__, __LINE__, strerror(errno));
            return;
        }
        if(!strncmp(cmd, "kill_shell", strlen("kill_shell"))){
            free(cmd);
            return;
        }
        write(fdin, cmd, strlen(cmd));
        /* Add polling so we can recursively grab more than 254 bytes */
        read(fdout, out, 254);
        printf("%s", out);

        free(cmd);
        cmd = NULL;
    }

    return;
}

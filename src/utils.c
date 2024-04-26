#include "../include/exdev.h"

#include <stdio.h>

uint16_t swap_b16(uint16_t n){
	return (n&0xff)<<8 | ((n&0xff00)>>8);
}

uint32_t swap_b32(uint32_t n){
	return (n&0xff)<<24 | ((n&0xff00)>>8)<<16 | ((n&0xff0000)>>16)<<8 | (n&0xff000000)>>24; 
}

uint64_t swap_b64(uint64_t n){
	return (n&0xff)<<56 | ((n&0xff00)>>8)<<48 | ((n&0xff0000)>>16)<<40 | ((n&0xff000000)>>24)<<32 | ((n&0xff00000000)>>32)<<24 | ((n&0xff0000000000)>>40)<<16 | ((n&0xff000000000000)>>48)<<8 | ((n&0xff00000000000000)>>56);
}

void hexdump(unsigned char *buf, int len, int width){
	int x = 0, y = 0, l = 0, li = 0;

	for(; l < (len/width); l++){
		li = 0;
		for(; x < len; x++){
			if(li == width) break;
			printf("%02x ", buf[x]);
			li++;
		}
		printf("| ");
		li = 0;
		for(; y < len; y++){
			if(li == width){
				printf("\n");
				break;
			}
			if(32 <= buf[y] && buf[y] < 127) printf("%c", buf[y]);
			else printf(".");
			li++;
		}
	}
	if(x < len){
		li = (len-x);
		l = width-li;
		for(; x < len; x++) printf("%02x ", buf[x]);
		for(x = 0; x < l; x++) printf("   ");
		printf("| ");
		for(; y < len; y++){
			if(32 <= buf[y] && buf[y] < 127) printf("%c", buf[y]);
			else printf(".");
		}
		printf("\n");
	}
	return;
}
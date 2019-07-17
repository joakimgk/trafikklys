#include <string.h>					/* Include string library */
#include <stdio.h>					/* Include standard IO library */
#include <stdlib.h>	/* Include standard library */

unsigned char program[150];
unsigned char rec_program[150];
unsigned char tmp[150];
int length = 0;
int rec_length = 0;
int step = 0;

void printBits(char a) {
    int i;
    for (i = 0; i < 8; i++) {
      printf("%d", !!((a << i) & 0x80));
    }
    printf("\n");
}

void swapArrays(char **a, char **b){
  char *temp = *a;
  *a = *b;
  *b = temp;
}

void handlePayload(char command, int len, char* payload) {
	printf("\033[34mPROGRAM (%d bytes):\n", length);
	for (int i = 0; i < length; i++) {
		printBits(program[i]);
	}
	printf("\033[0m");
			
    switch (command) {
        case 0x01:
            printf("\033[33mEndre TEMPO til %c\033[0m\n", payload[0]);
            break;
            
        case 0x02:
            printf("\033[33mRESTART program\033[0m\n");
            break;
            
		case 0x03:  // MOTTA PROGRAM  (dump _buffer inn i *program)
			printf("\033[33mLast inn nytt PROGRAM (%d bytes)\033[0m\n", len);

			memset(rec_program, 0, 150);
			memcpy(rec_program, payload, len);
			for (int i = 0; i < len; i++) {
				printBits(rec_program[i]);
			}
			rec_length = len;
			break;

		case 0x04: // BYTT PROGRAM
		    printf("\033[33mBytt PROGRAM\033[0m\n");
		
		    swapArrays(program, rec_program);
		    /*
			*tmp = *program;
			*program = *rec_program;
			*rec_program = *tmp;
			*/
			length = rec_length;
			
			step = 0;
			break;

        default:
            break;
    }
}

int main(void)
{
    
    program[0] = 0xFF;
	program[1] = 0x00;
	length = 2;
	step = 0;
	
	
	char _buffer[150];
	char * pbuffer_cmd;
	unsigned char payload[10];
	
	printf("Parse datapakke:\n");
	unsigned char nyttProgram[8] = {0x3F,0x9F,0xCF,0xE7,0xF3,0xF9,0xFC, 0};
	
	memset(_buffer, 0, 150);
	sprintf(_buffer, "\r\nRecv 30 bytes\r\n+IPD,5:%u%u%s\r\nSEND OK\r\n\r\n+IPD,3:%u%u2\r\n+IPD,3:%u%u\r\n\r\n+IPD,3:%u%u4\r\n", 
	                0x03, 0x07, nyttProgram, 
	                0x01, 0x01,
	                0x04, 0x00,
	                0x01, 0x01);
	pbuffer_cmd = _buffer;
	int t = 1;
	while (1)
	{
		pbuffer_cmd = strstr(pbuffer_cmd, "+IPD");
		if (pbuffer_cmd == NULL) {
		    break;
		}
		
		char *pbuffer_len = strstr(pbuffer_cmd, ":");

		int startpos = (int)(pbuffer_cmd - _buffer);
		int endpos = (int)(pbuffer_len - _buffer);
		
		// hent ut lengde av pakke (IPD)
		strncpy(payload, pbuffer_len-1, endpos-(startpos+5));
		int len = 0;
		sscanf(payload, "%d", &len);	
	
		strncpy(payload, pbuffer_len+1, len);
		
		char dKommando = payload[0] - '0';
		int dLengde = payload[1] - '0';
		strncpy(payload, pbuffer_len+1 +2, dLengde);
		payload[dLengde] = 0;
		
		printf("\nPakke %d:\n\tKommando: %u\n\tLengde: %d\n\tPayload: %s\n", t++, dKommando, dLengde, payload);
	
		handlePayload(dKommando, dLengde, payload);
		pbuffer_cmd = pbuffer_len;
	}
	
	printf("\nDone. Leste %d pakker.\n", t-1);
	
  return 0;
}

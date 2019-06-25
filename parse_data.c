#include <string.h>					/* Include string library */
#include <stdio.h>					/* Include standard IO library */
#include <stdlib.h>	/* Include standard library */

int main(void)
{
	char _buffer[150];
	char * pbuffer_cmd;
	char payload[10];
	
	printf("Parse datapakke:\n\n");
	
	memset(_buffer, 0, 150);
	sprintf(_buffer, "\r\nRecv 30 bytes\r\n+IPD,5:%u%uABC\r\nSEND OK\r\n\r\n+IPD,3:%u%u2\r\n", 0x02, 0x03, 0x01, 0x01);
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
	
	    // hent ut data (våre bytes). TODO: Kunne lese ut N>1 bytes...
		strncpy(payload, pbuffer_len+1, len);
		
		char dKommando = payload[0];
		int dLengde = payload[1] - '0';
		strncpy(payload, pbuffer_len+1 +2, dLengde);
		payload[dLengde] = 0;
		
		printf("\nPakke %d:\n\tKommando: %u\n\tLengde: %d\n\tPayload: %s\n", t++, dKommando, dLengde, payload);
	
		//handlePayload(dKommando, dLengde, payload);
		pbuffer_cmd = pbuffer_len;
	}
	
	printf("\nDone. Leste %d pakker.\n", t-1);
	
  return 0;
}

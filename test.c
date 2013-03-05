#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* helper program to dump state from gzipped traces.  */

int main()
{
	uint instr_addr = 0xdeadbeef;
	uint next_addr = 0xdeadbeef;
	uint actual_addr = 0xdeadbeef;
	uint status = 0x3f;
	while (scanf("%08x%08x%08x%02x\n", 
		     &instr_addr,
		     &next_addr,
		     &actual_addr,
		     &status) == 4) {

		printf("instr: %x", instr_addr);
		printf("\tnext: %x", next_addr);
		printf("\tactual: %x", actual_addr);
		printf("\t");
		if(status & 0b10000) printf("i", status); /* is indirect */
		if(status & 0b1000) printf("b", status);  /* is conditional */
		if(status & 0b100) printf("c", status);	  /* is call */
		if(status & 0b10) printf("r", status);	  /* is return */
		if(status & 0b1) printf("t", status);	  /* was taken */
		printf("\n");
		
	}
}

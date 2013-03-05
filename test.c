#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "predictor.h"
/* helper program to dump state from gzipped traces.  */

int main()
{
	uint instr_addr = 0xdeadbeef;
	uint next_addr = 0xdeadbeef;
	uint actual_addr = 0xdeadbeef;
	uint status = 0x3f;
	long int num_branches = 0;
	long int missed_predictions = 0;
	long int missed_dest = 0;
	while (scanf("%08x%08x%08x%02x\n", 
		     &instr_addr,
		     &next_addr,
		     &actual_addr,
		     &status) == 4) {

		num_branches++;
		branch_record_c br;
		br.instruction_addr = instr_addr;
		br.instruction_next_addr = next_addr;
		br.is_indirect = status & 0b10000;
		br.is_conditional = status & 0b1000;
		br.is_call = status & 0b100;
		br.is_return = status & 0b10;
		uint predicted_addr;
		bool taken = status & 0b1;
		PREDICTOR p;
		bool thought_taken = p.get_prediction(&br, NULL, &predicted_addr);

		if((predicted_addr != actual_addr) && !(taken || !br.is_conditional))
			missed_dest++;
		
		if(br.is_conditional && (thought_taken != taken))
			missed_predictions++;

		p.update_predictor(&br, NULL, taken, actual_addr);
	}
	printf("missed destinations per thousand transfers: %f\n", 
	       1000.0*(double)missed_dest/num_branches);
	printf("missed predictions per thousand transfers: %f\n", 
	       1000.0*(double)missed_predictions/num_branches);
}

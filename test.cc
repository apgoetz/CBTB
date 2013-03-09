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
	long int num_cond = 0;
	long int num_uncond = 0;
	long int num_returns = 0;
	long int num_calls = 0;
	long int num_branches = 0;
	long int missed_calls = 0;
	long int missed_returns = 0;
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


		if(predicted_addr != actual_addr) {
			missed_dest++;

			if(br.is_call)
				missed_calls++;
			if(br.is_return)
				missed_returns++;
		}

		if(br.is_conditional && (thought_taken != taken))
			missed_predictions++;

		p.update_predictor(&br, NULL, taken, actual_addr);

		if (br.is_conditional) num_cond++;
		else num_uncond++;
		
		if (br.is_return) {
			num_returns++;
		}
		if (br.is_call) {
			num_calls++;
		}

	}
	printf("tot_branches:       %8ld\n", num_branches);
	printf("cond_branches:      %8ld\n", num_cond);
	printf("uncond_branches:    %8ld\n", num_uncond);
	printf("calls:              %8ld\n", num_calls);
	printf("returns:            %8ld\n", num_returns);
	printf("missed_predictions: %8ld\n", missed_predictions);
	printf("missed_targets:     %8ld\n", missed_dest);
	printf("missed t per 1000:  %7.3f\n", 1000.0*(double)missed_dest/(double)num_branches);
	printf("missed_calls:       %8ld\n", missed_calls);
	printf("missed_returns:     %8ld\n", missed_returns);
}

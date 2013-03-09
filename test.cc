#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "predictor.h"
/* helper program to dump state from gzipped traces.  */


long int num_branches = 0;


double perc(long int dividend, long int divisor){
	return 100.0*(double)dividend/(double)divisor;
}
double percb(long int val) {
	return perc(val, num_branches);
}
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
	long int num_indir = 0;
	long int missed_calls = 0;
	long int missed_returns = 0;
	long int missed_predictions = 0;
	long int missed_dest = 0;
	long int missed_cond = 0;
	long int missed_uncond = 0;
	long int missed_indir = 0;
	PREDICTOR p;
	branch_record_c br;
	uint predicted_addr;
	p.get_prediction(&br, NULL, &predicted_addr);
	while (scanf("%08x%08x%08x%02x\n", 
		     &instr_addr,
		     &next_addr,
		     &actual_addr,
		     &status) == 4) {

		num_branches++;

		br.instruction_addr = instr_addr;
		br.instruction_next_addr = next_addr;
		br.is_indirect = status & 0b10000;
		br.is_conditional = status & 0b1000;
		br.is_call = status & 0b100;
		br.is_return = status & 0b10;

		bool taken = status & 0b1;

		bool thought_taken = p.get_prediction(&br, NULL, &predicted_addr);


		if(predicted_addr != actual_addr) {
			missed_dest++;

			if(br.is_call)
				missed_calls++;
			if(br.is_return)
				missed_returns++;
			if(br.is_conditional)
				missed_cond++;
			else
				missed_uncond++;
			if(br.is_indirect)
				missed_indir++;
	
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
		if(br.is_indirect) {
			num_indir++;
		}

	}
	printf("tot_branches:       %8ld   %3.3f\n", 
		num_branches, percb(num_branches));
	printf("cond_branches:      %8ld   %3.3f\n", 
		num_cond, percb(num_cond));
	printf("uncond_branches:    %8ld   %3.3f\n", 
		num_uncond, percb(num_uncond));
	printf("calls:              %8ld   %3.3f\n", 
		num_calls, percb(num_calls));
	printf("returns:            %8ld   %3.3f\n", 
		num_returns, percb(num_returns));
	printf("indir:              %8ld   %3.3f\n", 
		num_indir, percb(num_indir));
	printf("missed_predictions: %8ld   %3.3f\n", 
		missed_predictions, percb(missed_predictions));
	printf("missed_targets:     %8ld   %3.3f\n", 
		missed_dest, percb(missed_dest));
	printf("missed_cond:        %8ld   %3.3f\n",
		missed_cond, perc(missed_cond, missed_dest));
	printf("missed_uncond:      %8ld   %3.3f\n",
		missed_uncond, perc(missed_uncond, missed_dest));
	printf("missed_indir:       %8ld   %3.3f\n",
		missed_indir, perc(missed_indir, missed_dest));
	printf("missed_calls:       %8ld   %3.3f\n", 
		missed_calls, perc(missed_calls, missed_dest));
	printf("missed_returns:     %8ld   %3.3f\n",
		missed_returns, perc(missed_returns, missed_dest));
}

#include <stdint.h>
#include "predictor.h"
#include <cassert>
#include <stdlib.h>
#include "alpha.cc"
#include "btb.cc"
static void on_exit(void);
static long int timescalled= 0;
bool PREDICTOR::get_prediction(
	const branch_record_c* br, 
	const op_state_c* os, 
	uint *predicted_target_address)
{
	timescalled++;
	static int hooked_exit = 0;
	if(!hooked_exit){
		hooked_exit++;
		atexit(on_exit);
	}
	assert(!br->is_indirect || !br->is_conditional);
	// bool prediction = true;
	
	if (br->is_conditional)
		return true;   // true for taken, false for not taken
	else
		return false;
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(
	const branch_record_c* br, 
	const op_state_c* os, bool taken, uint actual_target_address)
{
	uint status = br->is_indirect ? 1 : 0;
	status = status << 1;
	status = status + (br->is_conditional ? 1: 0);
	status = status << 1;
	status = status + (br->is_call ? 1: 0);
	status = status << 1;
	status = status + (br->is_return ? 1: 0);
	status = status << 1;
	status = status + (taken ? 1: 0);

}
static void on_exit(void)
{
	printf("I got called %ld\n", timescalled);
	// do nothing
}

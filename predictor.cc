#include <stdint.h>
#include "predictor.h"
#include <cassert>
#include <stdlib.h>
#include "alpha.cc"
#include "btb.cc"
static void on_exit(void);
static void init(void);
static long int timescalled= 0;


bool PREDICTOR::get_prediction(
	const branch_record_c* br, 
	const op_state_c* os, 
	uint *predicted_target_address)
{
	// need to only run this code once.
	static int initial_run = 1;
	if(!initial_run){
		initial_run = 0;
		init();
	}
	bool taken = true;

	if (br->is_conditional)
		taken = alpha_predict(br);

	// the predictor is only checked if the branch was taken, or
	// it was unconditional. Therefore, we only need to call the
	// target predictor if we think this was a taken branch, or if
	// the branch is unconditional
	if (taken)
		*predicted_target_address = target_predict(br);

	

	timescalled++;	
	return taken;
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(
	const branch_record_c* br, 
	const op_state_c* os, bool taken, uint actual_target_address)
{

	target_update(br, actual_target_address);

	if (br->is_conditional)
		alpha_update(br, taken);

}
static void on_exit(void)
{
	fprintf(stderr, "I got called %ld\n", timescalled);
	// do nothing
}

static void init(void)
{
	atexit(on_exit);
}

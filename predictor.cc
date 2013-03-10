#include <stdint.h>
#include "predictor.h"
#include <cassert>
#include <stdlib.h>
#include "alpha.cc"
#include "btb.cc"
#include "util.cc"
#include <set>
// These functions are called once at the begining, and once at the
// end of the trace
static void on_exit(void);
static void init(void);

static long int timescalled= 0;
static FILE *oraclefd = NULL;
static std::set<uint> addr_hist;
bool PREDICTOR::get_prediction(
	const branch_record_c* br, 
	const op_state_c* os, 
	uint *predicted_target_address)
{
	// need to only run this code once.
	static int initial_run = 1;
	if(initial_run){
		initial_run = 0;
		init();
	}
	bool taken = true;

	if(oraclefd == NULL) {
		if (br->is_conditional)
			taken = alpha_predict(br);
	} else {
		uint instr_addr = 0xdeadbeef;
		uint next_addr = 0xdeadbeef;
		uint actual_addr = 0xdeadbeef;
		uint status = 0x3f;
		fscanf(oraclefd, "%08x%08x%08x%02x\n", 
			&instr_addr,
			&next_addr,
			&actual_addr,
			&status);
		assert(instr_addr == br->instruction_addr);
		taken = status & 1;
		if(addr_hist.count(br->instruction_addr)){
			*predicted_target_address = actual_addr;
			return taken;
		} else {
			*predicted_target_address = br->instruction_next_addr;
			addr_hist.insert(br->instruction_addr);
			return false;
		}
	}

	// the predictor is only checked if the branch was taken, or
	// it was unconditional. Therefore, we only need to call the
	// target predictor if we think this was a taken branch, or if
	// the branch is unconditional
	*predicted_target_address = btb_predict(br);

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

	btb_update(br, actual_target_address);

	if (br->is_conditional)
		alpha_update(br, taken);
}
static void on_exit(void)
{
	// if we are using an oracle, close the pipe.
	if (oraclefd)
		pclose(oraclefd);

	// run the  destroy functions:
	btb_destroy();
	alpha_destroy();
}

static void init(void)
{
	// determine if we should print debug messages...
	getparam("BTB_DEBUG", &util_debugmode);

	// If we decided to use an ORACLE, hook the oracle now.
	char* oraclefile = getenv("ORACLE");
	char  oraclecmd[1024];
	sprintf(oraclecmd, "xzcat %s",oraclefile);
	if (oraclefile){
		oraclefd = popen(oraclecmd, "r");
		debug("Hooking ORACLE... %s\n", oraclefile);
	}

	// hook the exit function, so that data structures can be
	// cleaned up. 
	atexit(on_exit);

	// call the setup functions:
	btb_setup();
	alpha_setup();
}

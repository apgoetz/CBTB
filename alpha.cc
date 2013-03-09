#ifndef __ALPHA_CC__
#define __ALPHA_CC__
#include "util.cc"
//Local history table, only the least significant 10 bits will be used
unsigned short local_hist_table[1024];
//Local prediction bits for the saturated counter for the local branch predictor
//Only uses least significant 3 bits
unsigned char local_predict[1024];

//Global prediction bits for the sturated counter for the global branch predictor
//Only uses least significant 2 bits
unsigned char global_predict[4096];

//Choice prediction bits for the saturated counter that chooses the
//predictor that will be used to do the predicting. Only uses least significant 2 bits
unsigned char choice_predict[4096];

//Path history stores the history of the last 12 branches.
//Only the least significant 12 bits are used, with the lsb
//being the most recent branch
unsigned short path_history;


//predicts taken/not taken branch based on local history
bool alpha_local_predict(const branch_record_c *br)
{
	
	return false;
}

//predicts taken/not taken branch based on global history
bool alpha_global_predict(const branch_record_c *br)
{
	return false;
}

//This is the predictor predictor AKA the Choice predictor
//uses the global history to determine which predictor to use
bool alpha_predict(const branch_record_c *br)
{
	return false;
}

//updates the path history to reflect whether the last branch
//was actually taken
void alpha_update(const branch_record_c *br, bool taken)
{

}

void alpha_setup(void) 
{
	int i = 0;
	debug("Setting up ALPHA predictor...\n");

	//initialize the tables for the local predictor
	for(i = 0; i < 1024; i++)
	{
		local_hist_table[i] = 0;
		local_predict[i] = 0;
	}

	//initialize the global prediction table
	//and the choice prediction table
	for(i = 0; i < 4096; i++)
	{
		global_predict[i] = 0;
		//sets the default choice prediction to weakly not taken
		choice_predict[i] = 2;
	}

	//initializes the path history to all not taken by default
	path_history = 0;
	
}

void alpha_destroy(void)
{
	debug("Destroying ALPHA predictor...\n");
}
#endif

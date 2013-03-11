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

//predictor chosen for the previous prediction
// true means global, false means local
bool current_predictor;

//predicts taken/not taken branch based on local history
bool alpha_local_predict(const branch_record_c *br)
{
	debug("alpha_local_predict called\n");

	//grabs bits 0-9 of the PC to index the table
	unsigned int PC = ((br->instruction_addr) & 0x3FF);

	//grabs the history in the table
	unsigned int history = local_hist_table[PC];
	//printf("PC = %u\nhistory = %u\n", PC, history);

	//predict true if the counter is more than 4
	//i.e. (weakly|strongly) taken
	if(local_predict[history] >= 4)
		return true;

	else
		return false;
	
}

//predicts taken/not taken branch based on global history
bool alpha_global_predict(const branch_record_c *br)
{
	debug("alpha_global_predict called\n");

	if(global_predict[path_history] >= 2)
		return true;

	else
		return false;
}

//This is the predictor predictor AKA the Choice predictor
//uses the global history to determine which predictor to use
bool alpha_predict(const branch_record_c *br)
{
	bool taken;

	if(choice_predict[path_history] >= 2)
	{
		taken = alpha_global_predict(br);
		current_predictor = true;
	}
	else
	{
		taken = alpha_local_predict(br);
		current_predictor = false;
	}

	return taken;
}

//updates the path history to reflect whether the last branch
//was actually taken
void alpha_update(const branch_record_c *br, bool taken)
{

	unsigned int PC = (br->instruction_addr) & 0x3FF;
	unsigned int global = global_predict[path_history];
	unsigned int local = local_predict[local_hist_table[PC]];

	//Choice Predictor:
	//if the current predictor is global, see if the prediction was correct
	//same if it was the local predictor
	if(current_predictor)
	{
		if(choice_predict[path_history] == 2)
		{
			//only increment prediction matches
			if(taken && global >= 2)
				choice_predict[path_history]++;
			else if(!taken && global < 2)
				choice_predict[path_history]++;
		}
		//decrement if prediction was incorrect
		else
			choice_predict[path_history]--;
	}
	else
	{
		if(choice_predict[path_history] == 1)
		{
			//only decrement if prediction matches
			if(taken && local >= 4)
				choice_predict[path_history]--;
			else if(!taken && local < 4)
				choice_predict[path_history]--;
		}
		//increment if prediction was incorrect
		else
			choice_predict[path_history]++;

	}

	//Global Predictor:
	//updates the global predictor saturated counter
	if(taken && global < 3)
		global_predict[path_history]++;

	else if(!taken && global > 0)
		global_predict[path_history]--;

	//Local Predictor:
	//updates the local predictor saturated counter
	if(taken && local < 3)
		local_predict[local_hist_table[PC]]++;

	else if(!taken && local > 0)
		local_predict[local_hist_table[PC]]--;

	//Path History:
	//shift left by one and mask off the last 12 bits
	//so that any bits above the 12th will be zero
	path_history = (path_history << 1) & 0x0FFF;
	
	if(taken)
		path_history++;

	//update the local history table with the newest history
	local_hist_table[PC] = path_history & 0x3FF;
}



void alpha_setup(void) 
{
	int i = 0;
	debug("Setting up ALPHA predictor...\n");

	//initialize the tables for the local predictor
	for(i = 0; i < 1024; i++)
	{
		local_hist_table[i] = 0;
		local_predict[i] = 4;
	}

	//initialize the global prediction table
	//and the choice prediction table
	for(i = 0; i < 4096; i++)
	{
		global_predict[i] = 1;
		//sets the default choice prediction to weakly not taken
		choice_predict[i] = 2;
	}

	//initializes the path history to all not taken by default
	path_history = 0;

	//default predictor for choice predictor
	//true = global, false = local
	current_predictor = true;
	
}

void alpha_destroy(void)
{
	debug("Destroying ALPHA predictor...\n");
}
#endif

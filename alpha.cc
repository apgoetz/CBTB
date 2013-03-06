#ifndef __ALPHA_CC__
#define __ALPHA_CC__

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

bool alpha_predict(const branch_record_c *br)
{
	return false;
}

void alpha_update(const branch_record_c *br, bool taken)
{
}

#endif

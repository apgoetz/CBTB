#ifndef __ALPHA_CC__
#define __ALPHA_CC__

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

void update(const branch_record_c *br, bool taken)
{
	return false;
}

#endif

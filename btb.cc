#ifndef __BTB_CC__
#define __BTB_CC__
// this contains the btb preprocessor

uint target_predict(const branch_record_c *br)
{
	return br->instruction_next_addr;
}

void target_update(const branch_record_c *br, uint actual_addr)
{
}


// setup and destroy functions for the btb predictor
void target_setup(void)
{
}

void target_destroy(void)
{
}
#endif


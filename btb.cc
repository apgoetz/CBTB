#ifndef __BTB_CC__
#define __BTB_CC__
// this contains the btb preprocessor

uint btb_predict(const branch_record_c *br)
{
	return br->instruction_next_addr;
}

void btb_update(const branch_record_c *br, uint actual_addr)
{
}


// setup and destroy functions for the btb predictor
void btb_setup(void)
{
}

void btb_destroy(void)
{
}
#endif


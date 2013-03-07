#ifndef __BTB_CC__
#define __BTB_CC__

#include <stdint.h>
#include <stdlib.h>
int BTB_BITSIZE = 4;
int BTB_NUM_WAYS = 4;
int BTB_SIZE;
int BTB_TAG_SIZE;
uint *btb_buffer;
uint *btb_tags;
uint *btb_lru;
static uint lastcall;
int log2(int value)
{
	value--;
	if (value == 0)
		return 1;
	for(int i = 32; i >= 1; i--) {
		if(0x80000000 & value)
			return i;
		value = value << 1;
	}
	return 0;
}
uint btb_predict(const branch_record_c *br)
{
	uint addr = br->instruction_addr;
	uint index = addr & ((1 << BTB_BITSIZE) - 1);
	uint shiftedindex = index * BTB_NUM_WAYS;
	uint tag = addr >> BTB_BITSIZE;

	if(br->is_return) {
		return lastcall;
	}

	for(int i = 0; i < BTB_TAG_SIZE; i++) {
		if(btb_tags[shiftedindex+i] == tag)
			return btb_buffer[shiftedindex+i];		
	}
	return br->instruction_next_addr;
}

void btb_update(const branch_record_c *br, uint actual_addr)
{
	uint addr = br->instruction_addr;
	uint index = addr & ((1 << BTB_BITSIZE) - 1);
	uint shiftedindex = index * BTB_NUM_WAYS;
	uint tag = addr >> BTB_BITSIZE;

	if(br->is_call) {
		lastcall = br->instruction_next_addr;
	}

	for (int i = 0; i < BTB_NUM_WAYS; i++) {
		if (btb_tags[shiftedindex+i] == tag){

			return;
		}

	}
	
	// insert into table
	btb_tags[shiftedindex + btb_lru[index]] = tag;
	btb_buffer[shiftedindex+ btb_lru[index]] = actual_addr;
	btb_lru[index]++;
	btb_lru[index] = btb_lru[index] % BTB_NUM_WAYS;
}


// setup and destroy functions for the btb predictor
void btb_setup(void)
{
	char* str_bitsize = NULL;
	char* str_numways = NULL;
	str_bitsize = getenv("BTB_BITSIZE");
	str_numways = getenv("BTB_NUM_WAYS");
	if(str_bitsize)
		BTB_BITSIZE = atoi(str_bitsize);
	if(str_numways)
		BTB_NUM_WAYS = atoi(str_numways);

	BTB_SIZE = (1 << BTB_BITSIZE);
	BTB_TAG_SIZE = (32 - BTB_BITSIZE);
	fprintf(stderr, "%d entries by %d ways\n", BTB_SIZE, BTB_NUM_WAYS);

	btb_buffer = (uint*)malloc(BTB_SIZE*BTB_NUM_WAYS*sizeof(uint));
	btb_tags = (uint*)malloc(BTB_SIZE*BTB_NUM_WAYS*sizeof(uint));
	btb_lru = (uint*)malloc(BTB_SIZE*sizeof(uint));

	for(int i = 0; i < BTB_SIZE*BTB_NUM_WAYS; i++) {
		btb_tags[i] = 0xffffffff;		
	}
	for(int i = 0; i < BTB_SIZE; i++) {
		btb_lru[i] = 0;		
	}

	printf("BTB size: %d\n",
	       BTB_SIZE*BTB_NUM_WAYS*BTB_TAG_SIZE + 
	       BTB_SIZE*BTB_NUM_WAYS*32+
	       BTB_SIZE*log2(2));

}

void btb_destroy(void)
{
	free(btb_buffer);
	free(btb_tags);
	free(btb_lru);

}
#endif


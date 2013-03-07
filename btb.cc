#ifndef __BTB_CC__
#define __BTB_CC__

#include <stdint.h>
#include <stdlib.h>
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

class BTB_CACHE {
private:
	size_t indexbits;
	size_t numways;
	size_t btbsize;
	size_t tagsize;
	uint* btb_buffer;
	uint* btb_tags;
	uint* btb_lru;
public:
	BTB_CACHE(int indexbits = 4, int numways = 1) : indexbits(indexbits), numways(numways), btbsize(1 << indexbits), tagsize(32-indexbits) {
		btb_buffer = (uint*)malloc(btbsize*numways*sizeof(uint));
		btb_tags = (uint*)malloc(btbsize*numways*sizeof(uint));
		btb_lru = (uint*)malloc(btbsize*sizeof(uint));		

		for(int i = 0; i < indexbits*numways; i++) {
			btb_tags[i] = 0xffffffff;
		}
		for(uint i = 0; i < btbsize; i++) {
			btb_lru[i] = 0;
		}
	}
	virtual ~BTB_CACHE() {
		free(btb_buffer);
		free(btb_tags);
		free(btb_lru);
	}
	virtual bool predict(uint instr, uint &target) {
		uint index = instr & ((1 << indexbits) - 1);
		uint shiftedindex = index * numways;
		uint tag = instr >> indexbits;
		
		for(uint i = 0; i < numways; i++) {
			if(btb_tags[shiftedindex+i] == tag) {
				target =  btb_buffer[shiftedindex+i];
				return true;
			}
		}
		return false;
	}
	virtual void update(uint addr, uint target) {
		uint index = addr & ((1 << indexbits) - 1);
		uint shiftedindex = index * numways;
		uint tag = addr >> indexbits;
		
		for (uint i = 0; i < numways; i++) {
			if (btb_tags[shiftedindex+i] == tag){
				return;
			}
		}	

		// insert into table
		btb_tags[shiftedindex + btb_lru[index]] = tag;
		btb_buffer[shiftedindex+ btb_lru[index]] = target;
		btb_lru[index]++;
		btb_lru[index] = btb_lru[index] % numways;
	}

	virtual uint size(void) {
		uint size = 0;
		size += btbsize * numways * tagsize;
		size += btbsize * numways* 32;
		if(numways > 1)
			size += btbsize*log2(numways);
		return size;
	}
};


BTB_CACHE *maincache;
uint btb_predict(const branch_record_c *br)
{
	if(br->is_return) {
		return lastcall;
	}

	uint target;
	if(!maincache->predict(br->instruction_addr, target))
		target = br->instruction_next_addr;

	return target;
}

void btb_update(const branch_record_c *br, uint actual_addr)
{

	if(br->is_call) {
		lastcall = br->instruction_next_addr;
	}

	maincache->update(br->instruction_addr, actual_addr);
}


// setup and destroy functions for the btb predictor
void btb_setup(void)
{
	char* str_bitsize = NULL;
	char* str_numways = NULL;
	str_bitsize = getenv("BTB_BITSIZE");
	str_numways = getenv("BTB_NUM_WAYS");
	int indexbits = 4;
	int numways = 1;
	if(str_bitsize)
		indexbits = atoi(str_bitsize);
	if(str_numways)
		numways = atoi(str_numways);

	fprintf(stderr, "%d entries by %d ways\n", 1 << indexbits, numways);

	maincache = new BTB_CACHE(indexbits, numways);

	fprintf(stderr, "BTB size: %d\n",maincache->size());

}

void btb_destroy(void)
{
	delete maincache;
}
#endif

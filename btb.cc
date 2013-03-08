#ifndef __BTB_CC__
#define __BTB_CC__

#include <stdint.h>
#include <stdlib.h>

#define log_message(...) do {if(debugmode)fprintf(stderr, __VA_ARGS__);} while(0)
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
	int m_displacementbits;
	uint* btb_buffer;
	uint* btb_tags;
	uint* btb_lru;

public:
	int displacementbits() {
		if (m_displacementbits < 0 || m_displacementbits >= 32)
			return 32;
		else
			return m_displacementbits;
	}
	BTB_CACHE(int indexbits = 4, int numways = 1, int displacementbits = -1) : indexbits(indexbits), numways(numways), btbsize(1 << indexbits), tagsize(32-indexbits), m_displacementbits(displacementbits) {
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
	~BTB_CACHE() {
		free(btb_buffer);
		free(btb_tags);
		free(btb_lru);
	}
	bool predict(uint instr, uint &target) {
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
	bool update(uint addr, uint target) {
		uint index = addr & ((1 << indexbits) - 1);
		uint shiftedindex = index * numways;
		uint tag = addr >> indexbits;
		
		if (displacementbits() != 32) {
			uint delta = target - addr;
			int64_t maxdisp = ((int64_t)1 << (m_displacementbits - 1)) - 1;
			int64_t mindisp = -maxdisp - 1;
			if (delta > maxdisp || delta < mindisp)
				return false;
		}
		
		for (uint i = 0; i < numways; i++) {
			if (btb_tags[shiftedindex+i] == tag){
				return true;
			}
		}	

		// insert into table
		btb_tags[shiftedindex + btb_lru[index]] = tag;
		btb_buffer[shiftedindex+ btb_lru[index]] = target;
		btb_lru[index]++;
		btb_lru[index] = btb_lru[index] % numways;
		return true;
	}

	uint size(void) {
		uint size = 0;
		size += btbsize * numways * tagsize;
		size += btbsize * numways* displacementbits();
		if(numways > 1)
			size += btbsize*log2(numways);
		return size;
	}
};


BTB_CACHE *maincache;
static uint lastcall;
static uint nummissed = 0;
static int debugmode = 0;
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

	if(!maincache->update(br->instruction_addr, actual_addr))
		nummissed++;
}

void getparam(const char* name, int *value) {
	char* tmpval = NULL;
	tmpval = getenv(name);
	if (tmpval)
		*value = atoi(tmpval);
}
// setup and destroy functions for the btb predictor
void btb_setup(void)
{
	int indexbits = 4;
	int numways = 1;
	int dispsize = -1;
	getparam("BTB_BITSIZE", &indexbits);
	getparam("BTB_NUM_WAYS", &numways);
	getparam("BTB_DISP_SIZE", &dispsize);
	getparam("BTB_DEBUG", &debugmode);

	maincache = new BTB_CACHE(indexbits, numways, dispsize);
	log_message("%d entries by %d ways, %d bit displacements\n", 
		1 << indexbits, numways, maincache->displacementbits());

	log_message("BTB size: %d\n",maincache->size());

}

void btb_destroy(void)
{
	delete maincache;
	log_message("Unable to cache %d branch targets.\n", nummissed);
}
#endif

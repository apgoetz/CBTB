#ifndef __BTB_CC__
#define __BTB_CC__

#include <stdint.h>
#include <stdlib.h>
#include "util.cc"
#include <vector>
#include <deque>
enum WayAlg_t {WAY_RROBIN = 0, WAY_LRU = 1};


// picks which way to replace, based on either lru or round robin. The
// LRU implementation uses counters, but sizes itself with the FSM
// implementation in order to reduce code size
class WayPicker {
private:
	int numways;	
	WayAlg_t algorithm;
	int *counters;
public:
	WayPicker(int numways = 1) 
		: numways(numways), algorithm(WAY_RROBIN), 
		  counters(new int[numways]) {
		getparam("BTB_WAY_ALGO", (int*)&algorithm);
		assert(numways > 0);
		if (algorithm > WAY_LRU)
			algorithm = WAY_LRU;
		else if (algorithm < WAY_RROBIN)
			algorithm = WAY_RROBIN;
		for(int i = 0; i < numways; i++)
			counters[i] = i;
	}
	// copy constructor
	WayPicker(const WayPicker& other) 
		: numways(other.numways), algorithm(other.algorithm), 
		  counters(new int[other.numways]){
		assert(numways > 0);
		for(int i = 0; i < numways; i++ )
			counters[i] = other.counters[i];
		
	}

	// copy assignment
	WayPicker & operator= (const WayPicker & other) {
		if(this != &other)
		{
			numways = other.numways;
			assert(numways > 0);
			algorithm = other.algorithm;
			delete[] counters;
			counters = new int[numways];
			for(int i = 0; i < numways; i++)
				counters[i] = other.counters[i];
		}
		return *this;
	}
	
	~WayPicker() {
		delete [] counters;
	}

	int replace() {
		assert(numways > 0);
		if(algorithm == WAY_RROBIN) {
			int choice = counters[0];			
			counters[0]++;
			counters[0] %= numways;
			return choice;
		} else {
			for(int i = 0; i < numways; i++) {
				if(counters[i] == numways-1) {
					update(i);
					return i;
				}	
			}
			assert(0);
		}
	}

	void update(int way_used) {
		assert(numways > 0);
		if(algorithm == WAY_LRU) {
			int oldval = counters[way_used];
			for(int i = 0; i < numways; i++)
				if (counters[i] < oldval)
					counters[i]++;
			counters[way_used] = 0;
		}
	}
	
	int size() {
		if (algorithm == WAY_RROBIN)
			return log2(numways);
		else
			return log2(factorial(numways));
	}
};

class CallHistoryQueue {
private:
	std::deque<uint> callqueue;
	int capacity;
	
public:
	uint maxsize;
	uint call_overflow;
	uint ret_underflow;
	CallHistoryQueue()
		: capacity(0), maxsize(0), call_overflow(0), ret_underflow(0) {
		getparam("BTB_FUNC_CAP", (int*)&capacity);
	}
	
	void call(uint addr) {
		if (capacity == 0) {
			call_overflow++;
			return;
		}
		callqueue.push_front(addr);
		uint size = callqueue.size();
		if(capacity > 0 && (int)size > capacity) {
			callqueue.pop_back();
			call_overflow++;
		}
		if(size > maxsize)
			maxsize = size;
	}
	
	bool ret(uint * addr) {
		if(callqueue.size() > 0) {
			*addr = callqueue.front();
			callqueue.pop_front();
			return true;
		} 
		ret_underflow++;
		return false;

	}
	
	int size() {
		if (capacity > 0)
			return 32*capacity + 2*log2(capacity);
		else if (capacity < 0)
			return -1;
		else
			return 0;
	}
};

// This class implements a branch target buffer that can store
// elements in an arbitrary number of ways, 
class BTB_CACHE {
private:
	int indexbits;
	size_t numways;
	size_t btbsize;
	size_t tagsize;

	int m_displacementbits;
	uint* btb_buffer;
	uint* btb_tags;
	std::vector<WayPicker> btb_lru;


public:
	uint nummissed;
	int displacementbits() {
		if (m_displacementbits < 0 || m_displacementbits >= 32)
			return 32;
		else
			return m_displacementbits;
	}

	BTB_CACHE() 
		: indexbits(-1) {
	}
	BTB_CACHE(int indexbits, int numways = 1, int displacementbits = -1) 
		: indexbits(indexbits), numways(numways), 
		  btbsize(1 << indexbits), 
		  tagsize(32-indexbits),
		  m_displacementbits(displacementbits), 
		  btb_lru(btbsize, WayPicker(numways)),
		  nummissed(0){		
		btb_buffer = (uint*)malloc(btbsize*numways*sizeof(uint));
		btb_tags = (uint*)malloc(btbsize*numways*sizeof(uint));

		for(uint i = 0; i < btbsize*numways; i++) {
			btb_tags[i] = 0xffffffff;
		}
	}
	~BTB_CACHE() {
		if (indexbits < 0)
			return;
		free(btb_buffer);
		free(btb_tags);
	}
	bool predict(uint instr, uint &target) {

		if(indexbits < 0)
			return false;

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

		if(indexbits < 0)
			return false;

		uint index = addr & ((1 << indexbits) - 1);
		uint shiftedindex = index * numways;
		uint tag = addr >> indexbits;
		
		if (displacementbits() != 32) {
			uint delta = target - addr;
			int64_t maxdisp = ((int64_t)1 << (m_displacementbits - 1)) - 1;
			int64_t mindisp = -maxdisp - 1;
			if (delta > maxdisp || delta < mindisp) {
				nummissed++;
				return false;
			}
		}
		
		for (uint i = 0; i < numways; i++) {
			if (btb_tags[shiftedindex+i] == tag){
				btb_lru[index].update(i);
				return true;
			}
		}	

		// insert into table
		uint way = btb_lru[index].replace();
		btb_tags[shiftedindex + way] = tag;
		btb_buffer[shiftedindex+ way] = target;

		return true;
	}

	uint size(void) {

		if(indexbits < 0)
			return 0;
		uint size = 0;
		size += btbsize * numways * tagsize;
		size += btbsize * numways* displacementbits();
		if(numways > 1) {
			WayPicker tmppicker(numways);
			size += btbsize*tmppicker.size();
		}
		if(numways > 8)
			size *=2;
		return size;
	}
};


BTB_CACHE *maincache;
BTB_CACHE *dispcache;
static CallHistoryQueue callqueue;
uint btb_predict(const branch_record_c *br)
{
	uint target;
	// check the displacement cache first
	if(!dispcache->predict(br->instruction_addr, target))
		// If it wasn't in the displacement cache, check the main cache
		if(!maincache->predict(br->instruction_addr, target))
			// if we still failed, we do not have it in
			// the cache. Return the next address instead. 
			target = br->instruction_next_addr;

	if(br->is_return) {
		if(callqueue.ret(&target))
			return target;
	}


	return target;
}

void btb_update(const branch_record_c *br, uint actual_addr)
{

	if(br->is_call)
		callqueue.call(br->instruction_next_addr);

	// if we cannot place the address in the displacement cache,
	// place it in the main cache (hierarchical caches!)
	if(!dispcache->update(br->instruction_addr, actual_addr)) {
		maincache->update(br->instruction_addr, actual_addr);
	}
}

// setup and destroy functions for the btb predictor
void btb_setup(void)
{
	int main_size = 0;
	int main_ways = 1;
	int disp_entries = 16;
	int disp_size = 0;
	int disp_ways = 1;
	WayAlg_t way_algo = WAY_RROBIN;
	getparam("BTB_MAIN_SIZE", &main_size);
	getparam("BTB_MAIN_WAYS", &main_ways);
	getparam("BTB_DISP_ENTRIES", &disp_entries);
	getparam("BTB_DISP_SIZE", &disp_size);
	getparam("BTB_DISP_WAYS", &disp_ways);
	getparam("BTB_WAY_ALGO", (int*)&way_algo);
	maincache = new BTB_CACHE(main_size, main_ways);
	if (disp_size >= 0)
		dispcache = new BTB_CACHE(disp_size, disp_ways, disp_entries);
	else 
		dispcache = new BTB_CACHE();
	debug("Way Algo: ");
	if(way_algo == WAY_RROBIN)
		debug("Round Robin\n");
	else
		debug("LRU\n");
	
	debug("Main: %d entries by %d ways, %d bit displacements\n", 
		1 << main_size, main_ways, maincache->displacementbits());
	if(disp_size >= 0)
	debug("Displacement: %d entries by %d ways, %d bit displacements\n", 
		1 << disp_size, disp_ways, dispcache->displacementbits());
	else
		debug("No displacement cache.\n");
	debug("BTB size: %d\n",dispcache->size() + maincache->size() + callqueue.size());


}

void btb_destroy(void)
{
	delete maincache;
	delete dispcache;
	debug("Unable to cache %d branch targets.\n", dispcache->nummissed);
	debug("max func stack size: %d\n", callqueue.maxsize);
	debug("call overflow: %d\n", callqueue.call_overflow);
	debug("ret underflow: %d\n", callqueue.ret_underflow);
}
#endif

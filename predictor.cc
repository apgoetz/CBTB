#include <stdint.h>
#include "predictor.h"
#include <cassert>
#include <stdlib.h>
#include <set>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include <deque>


static int util_debugmode = 0;
#define debug(...) do {if(util_debugmode)fprintf(stderr, __VA_ARGS__);} while(0)

// implements ceil(log2(x))
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

int factorial(int n){
	if(n <= 1)
		return 1;
	else 
		return n*factorial(n-1);
}

void getparam(const char* name, int *value)
{
	char* tmpval = NULL;
	tmpval = getenv(name);
	if (tmpval)
		*value = atoi(tmpval);
}

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

// These functions are called once at the begining, and once at the
// end of the trace
static void on_exit(void);
static void init(void);
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

// helper function to increment a saturated counter
void inc_cnt(unsigned char &counter, uint size) {
	if (counter < (unsigned char)((1 << size) - 1))
		counter++;
}



// helper function to decrement a saturated counter
void dec_cnt(unsigned char &counter, uint size) {
	if (counter > 0)
		counter--;
}
//predicts taken/not taken branch based on local history
bool alpha_local_predict(const branch_record_c *br)
{
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
	}
	else
	{
		taken = alpha_local_predict(br);
	}

	return taken;
}

//updates the path history to reflect whether the last branch
//was actually taken
void alpha_update(const branch_record_c *br, bool taken)
{

	unsigned int PC = (br->instruction_addr) & 0x3FF;
	bool g_correct = (alpha_global_predict(br) == taken);
	bool l_correct = (alpha_local_predict(br) == taken);

	// update choice predictor
	if (!g_correct && !l_correct) {
		// do nothing
	} else if (!g_correct && l_correct) {
		dec_cnt(choice_predict[path_history], 2);
	} else if (g_correct && !l_correct) {
		inc_cnt(choice_predict[path_history], 2);
	} else if (g_correct && l_correct) {
		// do nothing
	}
		

	//Global Predictor:
	//updates the global predictor saturated counter
	if(taken)
		inc_cnt(global_predict[path_history],2);

	else
		dec_cnt(global_predict[path_history], 2);

	//Local Predictor:
	//updates the local predictor saturated counter
	if(taken)
		inc_cnt(local_predict[local_hist_table[PC]], 3);

	else if(!taken)
		dec_cnt(local_predict[local_hist_table[PC]], 3);

	//Path History:
	//shift left by one and mask off the last 12 bits
	//so that any bits above the 12th will be zero
	path_history = (path_history << 1) & 0xFFF;
	
	if(taken)
		path_history++;


	//update the local history table with the newest history
	unsigned short &local_hist = local_hist_table[PC];
	local_hist = local_hist << 1;
	local_hist += taken ? 1: 0;
	local_hist &= 0x3ff;
}



void alpha_setup(void) 
{
	int i = 0;

	//initialize the tables for the local predictor
	for(i = 0; i < 1024; i++)
	{
		local_hist_table[i] = 0;
		local_predict[i] = 0b011;
	}

	//initialize the global prediction table
	//and the choice prediction table
	for(i = 0; i < 4096; i++)
	{
		global_predict[i] = 0b01;
		//sets the default choice prediction to weakly not taken
		choice_predict[i] = 0b10;
	}

	//initializes the path history to all not taken by default
	path_history = 0;

	
}

void alpha_destroy(void)
{
}

static FILE *oraclefd = NULL;
static std::set<uint> addr_hist;
bool PREDICTOR::get_prediction(
	const branch_record_c* br, 
	const op_state_c* os, 
	uint *predicted_target_address)
{
	// need to only run this code once.
	static int initial_run = 1;
	if(initial_run){
		initial_run = 0;
		init();
	}
	bool taken = true;

	if(oraclefd == NULL) {
		if (br->is_conditional)
			taken = alpha_predict(br);
	} else {
		uint instr_addr = 0xdeadbeef;
		uint next_addr = 0xdeadbeef;
		uint actual_addr = 0xdeadbeef;
		uint status = 0x3f;
		fscanf(oraclefd, "%08x%08x%08x%02x\n", 
			&instr_addr,
			&next_addr,
			&actual_addr,
			&status);
		assert(instr_addr == br->instruction_addr);
		taken = status & 1;
		if(addr_hist.count(br->instruction_addr)){
			*predicted_target_address = actual_addr;
			//			return taken;
		} else {
			*predicted_target_address = br->instruction_next_addr;
			addr_hist.insert(br->instruction_addr);
			//			return false;
		}
	}

	// the predictor is only checked if the branch was taken, or
	// it was unconditional. Therefore, we only need to call the
	// target predictor if we think this was a taken branch, or if
	// the branch is unconditional
	*predicted_target_address = btb_predict(br);

	return taken;
}

// Update the predictor after a prediction has been made.  This should accept
// the branch record (br) and architectural state (os), as well as a third
// argument (taken) indicating whether or not the branch was taken.
void PREDICTOR::update_predictor(
	const branch_record_c* br, 
	const op_state_c* os, bool taken, uint actual_target_address)
{

	btb_update(br, actual_target_address);

	if (br->is_conditional)
		alpha_update(br, taken);
}
static void on_exit(void)
{
	// if we are using an oracle, close the pipe.
	if (oraclefd)
		pclose(oraclefd);

	// run the  destroy functions:
	btb_destroy();
	alpha_destroy();
}

static void init(void)
{
	// determine if we should print debug messages...
	getparam("BTB_DEBUG", &util_debugmode);

	// If we decided to use an ORACLE, hook the oracle now.
	char* oraclefile = getenv("ORACLE");
	char  oraclecmd[1024];
	sprintf(oraclecmd, "xzcat %s",oraclefile);
	if (oraclefile){
		oraclefd = popen(oraclecmd, "r");
		debug("Hooking ORACLE... %s\n", oraclefile);
	}

	// hook the exit function, so that data structures can be
	// cleaned up. 
	atexit(on_exit);

	// call the setup functions:
	btb_setup();
	alpha_setup();
}

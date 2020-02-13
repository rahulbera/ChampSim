#ifndef SCOOBY_HELPER_H
#define SCOOBY_HELPER_H

#include <string>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "bitmap.h"

using namespace std;

class State
{
public:
	uint64_t pc;
	uint64_t page;
	uint32_t offset;
	int32_t	 delta;
	uint32_t local_delta_sig;
	uint32_t local_delta_sig2;
	uint32_t local_pc_sig;
	
	/* 
	 * Add more states here
	 */

	void reset()
	{
		pc = 0xdeadbeef;
		page = 0xdeadbeef;
		offset = 0;
		delta = 0;
		local_delta_sig = 0;
		local_delta_sig2 = 0;
		local_pc_sig = 0;
	}
	State(){reset();}
	~State(){}
	uint32_t value(); /* apply as many state types as you want */
	uint32_t get_hash(uint64_t value); /* play wild with hashes */
	std::string to_string();
};

class Scooby_STEntry
{
public:
	uint64_t page;
	deque<uint64_t> pcs;
	deque<uint32_t> offsets;
	deque<int32_t> deltas;
	Bitmap bmp_real;
	Bitmap bmp_pred;
	unordered_set <uint64_t> unique_pcs;
	unordered_set <int32_t> unique_deltas;
	uint64_t trigger_pc;
	uint32_t trigger_offset;
	bool streaming;

	uint32_t total_prefetches;

public:
	Scooby_STEntry(uint64_t p, uint64_t pc, uint32_t offset) : page(p)
	{
		pcs.clear();
		offsets.clear();
		deltas.clear();
		bmp_real.reset();
		unique_pcs.clear();
		unique_deltas.clear();
		trigger_pc = pc;
		trigger_offset = offset;
		streaming = false;

		pcs.push_back(pc);
		offsets.push_back(offset);
		unique_pcs.insert(pc);
		bmp_real[offset] = 1;
	}
	~Scooby_STEntry(){}
	uint32_t get_delta_sig();
	uint32_t get_delta_sig2();
	uint32_t get_pc_sig();
	void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
	void track_prefetch(uint32_t offset);
};

/* some data structures to mine information from workloads */
class ScoobyRecorder
{
public:
	unordered_set<uint64_t> unique_pcs;
	unordered_set<uint64_t> unique_trigger_pcs;
	unordered_set<uint64_t> unique_pages;
	unordered_map<uint64_t, uint64_t> access_bitmap_dist;
	uint64_t total_bitmaps_seen;
	uint64_t unique_bitmaps_seen;
	ScoobyRecorder(){}
	~ScoobyRecorder(){}	
	void record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset);
	void record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset);
	void record_access_knowledge(Scooby_STEntry *stentry);
	void dump_stats();
};

/* auxiliary functions to get insights from workloads */
void print_access_debug(Scooby_STEntry *stentry);

#endif /* SCOOBY_HELPER_H */


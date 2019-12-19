#ifndef SCOOBY_HELPER_H
#define SCOOBY_HELPER_H

#include <deque>
#include <unordered_set>
#include "bitmap.h"

using namespace std;

class Scooby_STEntry
{
public:
	uint64_t page;
	deque<uint64_t> pcs;
	deque<uint32_t> offsets;
	deque<int32_t> deltas;
	Bitmap pattern;
	unordered_set <uint64_t> unique_pcs;
	unordered_set <int32_t> unique_deltas;

public:
	Scooby_STEntry(uint64_t p, uint64_t pc, uint32_t offset) : page(p)
	{
		pcs.clear();
		offsets.clear();
		deltas.clear();
		pattern.reset();
		unique_pcs.clear();
		unique_deltas.clear();

		pcs.push_back(pc);
		offsets.push_back(offset);
		unique_pcs.insert(pc);
		pattern[offset] = 1;
	}
	~Scooby_STEntry(){}
	uint32_t get_delta_sig();
	uint32_t get_pc_sig();
	void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
};

/* some data structures to mine information from workloads */
class ScoobyRecorder
{
public:
	unordered_set<uint64_t> unique_pcs;
	unordered_set<uint64_t> unique_trigger_pcs;
	unordered_set<uint64_t> unique_pages;
	ScoobyRecorder(){}
	~ScoobyRecorder(){}	
	void record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset);
	void record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset);
	void dump_stats();
};

/* auxiliary functions to get insights from workloads */
void print_access_debug(Scooby_STEntry *stentry);

#endif /* SCOOBY_HELPER_H */


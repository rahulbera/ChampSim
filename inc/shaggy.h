#ifndef SHAGGY_H
#define SHAGGY_H

#include <vector>
#include <deque>
#include "scooby_helper.h"

using namespace std;

class Shaggy_PBEntry
{
public:
	uint64_t page;
	uint32_t last_prefetched_offset;
	Shaggy_PBEntry(uint64_t _page) : page(_page)
	{
		last_prefetched_offset = 0;
	}
	~Shaggy_PBEntry(){}
};

class Shaggy
{
private:
	deque<uint64_t> signature_table;
	deque<Shaggy_PBEntry*> prefetch_buffer;

	struct
	{
		struct
		{
			uint64_t called;
			uint64_t pb_evict;
			uint64_t pb_insert;
			uint64_t predicted;
		} predict;

		struct
		{
			uint64_t called;
			uint64_t cond_not_met;
			uint64_t st_evict;
			uint64_t st_insert;
		} record_signature;
		
	} stats;

private:
	void init_knobs();
	void init_stats();
	Shaggy_PBEntry* search_pb(uint64_t page);
	uint64_t create_signature(uint64_t pc, uint32_t offset);


public:
	Shaggy();
	~Shaggy();
	void predict(uint64_t page, uint32_t offset, vector<uint64_t> &pref_addr);
	void record_signature(Scooby_STEntry *stentry);
	bool lookup_signature(uint64_t pc, uint64_t page, uint32_t offset);
	void print_config();
	void dump_stats();
};

#endif /* SHAGGY_H */
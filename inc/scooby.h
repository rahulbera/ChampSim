#ifndef SCOOBY_H
#define SCOOBY_H

#include <vector>
#include "prefetcher.h"
#include "scooby_helper.h"
#include "learning_engine.h"

using namespace std;

#define MAX_REWARDS 16

class State
{
public:
	uint64_t pc;
	uint64_t page;
	uint32_t offset;
	int32_t	 delta;
	uint32_t local_delta_sig;
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
		local_pc_sig = 0;
	}
	State(){reset();}
	~State(){}
	uint32_t value(); /* apply as many hash as you want */
};

class Scooby_PTEntry
{
public:
	uint64_t address;
	State *state;
	uint32_t action_index;
	/* set when prefetched line is filled into cache 
	 * check during reward to measure timeliness */
	bool is_filled;
	uint32_t reward;
	bool has_reward;
	Scooby_PTEntry(uint64_t ad, State *st, uint32_t ac) : address(ad), state(st), action_index(ac)
	{
		is_filled = false;
		reward = 0;
		has_reward = false;
	}
	~Scooby_PTEntry(){}
};

class Scooby : public Prefetcher
{
private:
	deque<Scooby_STEntry*> signature_table;
	LearningEngine *brain;
	deque<Scooby_PTEntry*> prefetch_tracker;
	Scooby_PTEntry *last_evicted_tracker;
	
	struct
	{
		struct
		{
			uint64_t lookup;
			uint64_t hit;
			uint64_t evict;
			uint64_t insert;
		} st;

		struct
		{
			uint64_t called;
			vector<uint64_t> action_dist;
			uint64_t predicted;
		} predict;

		struct
		{
			uint64_t called;
			uint64_t evict;
		} track;

		struct
		{
			struct
			{
				uint64_t called;
				uint64_t pt_not_found;
				uint64_t has_reward;
			} demand;

			struct
			{
				uint64_t called;
			} train;
			
			uint64_t correct_timely;
			uint64_t correct_untimely;
			uint64_t no_pref;
			uint64_t incorrect;
			uint64_t dist[MAX_ACTIONS][MAX_REWARDS];
		} reward;

		struct
		{
			uint64_t called;
			uint64_t compute_reward;
		} train;

		struct
		{
			uint64_t called;
			uint64_t set;
		} register_fill;
	} stats;

private:
	void init_knobs();
	void init_stats();

	void update_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	Scooby_STEntry* update_st(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address);
	uint32_t predict(uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr);
	void track(uint64_t address, State *state, uint32_t action_index);
	void reward(uint64_t address);
	void reward(Scooby_PTEntry *ptentry);
	void train(Scooby_PTEntry *curr_evicted, Scooby_PTEntry *last_evicted);
	Scooby_PTEntry* search_pt(uint64_t address);

public:
	Scooby(string type);
	~Scooby();
	void invoke_prefetcher(uint64_t pc, uint64_t address, vector<uint64_t> &pref_addr);
	void register_fill(uint64_t address);
	void dump_stats();
	void print_config();
};

#endif /* SCOOBY_H */


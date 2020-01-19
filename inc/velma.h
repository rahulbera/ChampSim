#ifndef VELMA_H
#define VELMA_H

#include <vector>
#include <deque>
#include <unordered_set>
#include "prefetcher.h"
#include "learning_engine.h"
#include "fred.h"

using namespace std;

#define MAX_CANDIDATE_PREFETCHERS 8
#define VELMA_MAX_ACTIONS 8
#define VELMA_MAX_REWARDS 8

typedef enum 
{
	Accurate = 0,
	Inaccurate,

	NumVelmaRewardTypes
} VelmaRewardType;

const char* MapVelmaRewardTypeString(VelmaRewardType type);

class VelmaState
{
public:
	uint64_t pc;
	uint64_t page;
	uint32_t offset;
	VelmaState() : pc(0xdeadbeef), page(0xdeadbeef), offset(0) {}
	~VelmaState(){}
	uint32_t value();
};

class VelmaPTEntry
{
public:
	VelmaState *vstate;
	uint32_t action_index;
	uint64_t address;
	bool is_filled;
	bool has_reward;
	int32_t reward;
	VelmaPTEntry() : vstate(NULL), action_index(0), address(0xdeadbeef), is_filled(false), has_reward(false), reward(0){}
	~VelmaPTEntry(){}
};

class Velma : public Prefetcher
{
private:
	vector<Prefetcher*> m_prefetchers;
	vector<int32_t> m_actions;
	LearningEngine *m_brain;
	deque<VelmaPTEntry*> prefetch_tracker;
	VelmaPTEntry *last_evicted_tracker;

	Fred *m_fred;

	/* staistics */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t called_dist[MAX_CANDIDATE_PREFETCHERS];
			uint64_t pref_dist[MAX_CANDIDATE_PREFETCHERS];
		} invoke_prefetcher;

		struct
		{
			uint64_t total;
			uint64_t pref_dist[MAX_CANDIDATE_PREFETCHERS];
		} issue;

		struct
		{
			uint64_t called;
			uint64_t dist[MAX_CANDIDATE_PREFETCHERS];
		} selection;

		struct
		{
			uint64_t called;
			uint64_t evict;
			uint64_t insert;
			uint64_t tracker_hit;
		} track;

		struct
		{
			uint64_t called;
			uint64_t called_reward;
		} train;

		struct
		{
			struct
			{
				uint64_t called;
				uint64_t hit;
				uint64_t has_reward;
				uint64_t pt_miss;
			} demand;

			struct
			{
				uint64_t called;
			} pt_evict;

			uint64_t dist[NumVelmaRewardTypes];
			uint64_t action_reward_dist[VELMA_MAX_ACTIONS][VELMA_MAX_REWARDS];
		} reward;

	} stats;

private:
	void init_knobs();
	void init_stats();
	unordered_set<uint32_t> select_prefetchers(VelmaState *vstate, uint32_t &action_index);
	void populate_prefetch_list(VelmaState *vstate, uint32_t action_index, unordered_set<uint32_t> selected_pref_list, vector<vector<uint64_t> > pref_candidates, vector<uint64_t> &pref_addr);
	bool track_prefetch(VelmaState *vstate, uint32_t action_index, uint64_t pref_addr);
	void train(VelmaPTEntry *curr_evicted_tracker, VelmaPTEntry *last_evicted_tracker);
	void reward(uint64_t address);
	void reward(VelmaPTEntry *ptentry);
public:
	Velma(string type);
	~Velma();
	void invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr);
	void register_fill(uint64_t address);
	void register_prefetch_hit(uint64_t address);
	void dump_stats();
	void print_config();
	int32_t getAction(uint32_t action_index);
};


#endif /* VELMA_H */


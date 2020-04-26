#ifndef SCOOBY_HELPER_H
#define SCOOBY_HELPER_H

#include <string>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "bitmap.h"
#include "bf/all.hpp"

using namespace std;

#define MAX_HOP_COUNT 16

typedef enum
{
	PC = 0,
	Offset,
	Delta,
	PC_path,
	Offset_path,
	Delta_path,
	Address,
	Page,

	NumFeatures
} Feature;

const char* getFeatureString(Feature feature);

typedef enum
{
	none = 0,
	incorrect,
	correct_untimely,
	correct_timely,
	out_of_bounds,
	tracker_hit,

	num_rewards
} RewardType;

inline bool isRewardCorrect(RewardType type) { return (type == correct_timely || type == correct_untimely); }
inline bool isRewardIncorrect(RewardType type) { return type == incorrect; }

class State
{
public:
	uint64_t pc;
	uint64_t address;
	uint64_t page;
	uint32_t offset;
	int32_t	 delta;
	uint32_t local_delta_sig;
	uint32_t local_delta_sig2;
	uint32_t local_pc_sig;
	uint32_t local_offset_sig;
	
	/* 
	 * Add more states here
	 */

	void reset()
	{
		pc = 0xdeadbeef;
		address = 0xdeadbeef;
		page = 0xdeadbeef;
		offset = 0;
		delta = 0;
		local_delta_sig = 0;
		local_delta_sig2 = 0;
		local_pc_sig = 0;
		local_offset_sig = 0;
	}
	State(){reset();}
	~State(){}
	uint32_t value(); /* apply as many state types as you want */
	uint32_t get_hash(uint64_t value); /* play wild with hashes */
	std::string to_string();
};

class ActionTracker
{
public:
	int32_t action;
	uint32_t conf;
	ActionTracker(int32_t act, uint32_t c) : action(act), conf(c) {}
	~ActionTracker() {}
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
	// int32_t last_pref_offset;
	// uint32_t last_pref_offset_conf;

	/* tracks last n actions on a page to determine degree */
	deque<ActionTracker*> action_tracker;
	unordered_set<int32_t> action_with_max_degree;
	unordered_set<int32_t> afterburning_actions;

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
		// last_pref_offset = 0;
		// last_pref_offset_conf = 0;

		pcs.push_back(pc);
		offsets.push_back(offset);
		unique_pcs.insert(pc);
		bmp_real[offset] = 1;
	}
	~Scooby_STEntry(){}
	uint32_t get_delta_sig();
	uint32_t get_delta_sig2();
	uint32_t get_pc_sig();
	uint32_t get_offset_sig();
	void update(uint64_t page, uint64_t pc, uint32_t offset, uint64_t address);
	void track_prefetch(uint32_t offset, int32_t pref_offset);
	void insert_action_tracker(int32_t pref_offset);
	bool search_action_tracker(int32_t action, uint32_t &conf);
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
	/* set when prefetched line is alredy found in cache
	 * donotes extreme untimely prefetch */
	bool pf_cache_hit;
	int32_t reward;
	RewardType reward_type;
	bool has_reward;
	vector<bool> consensus_vec; // only used in featurewise engine
	
	Scooby_PTEntry(uint64_t ad, State *st, uint32_t ac) : address(ad), state(st), action_index(ac)
	{
		is_filled = false;
		pf_cache_hit = false;
		reward = 0;
		reward_type = RewardType::none;
		has_reward = false;
	}
	~Scooby_PTEntry(){}
};

/* some data structures to mine information from workloads */
class ScoobyRecorder
{
public:
	unordered_set<uint64_t> unique_pcs;
	unordered_set<uint64_t> unique_trigger_pcs;
	unordered_set<uint64_t> unique_pages;
	unordered_map<uint64_t, uint64_t> access_bitmap_dist;
	// vector<unordered_map<int32_t, uint64_t>> hop_delta_dist;
	uint64_t hop_delta_dist[MAX_HOP_COUNT+1][127];
	uint64_t total_bitmaps_seen;
	uint64_t unique_bitmaps_seen;
	ScoobyRecorder(){}
	~ScoobyRecorder(){}	
	void record_access(uint64_t pc, uint64_t address, uint64_t page, uint32_t offset);
	void record_trigger_access(uint64_t page, uint64_t pc, uint32_t offset);
	void record_access_knowledge(Scooby_STEntry *stentry);
	void dump_stats();
};

class DegreeDetector
{
public:
	uint32_t opt_hash_functions;
	bf::basic_bloom_filter *bf;
	uint32_t num_insert, num_hit, num_access;
	float ratio;

	/* stats */
	struct
	{
		uint64_t epochs;
		uint64_t histogram[10];
	} stats;

	DegreeDetector();
	~DegreeDetector(){}
	void add_pf(uint64_t addr);
	void add_dm(uint64_t addr);
	inline float get_ratio() {return ratio;}
	void dump_stats();
};

/* auxiliary functions */
void print_access_debug(Scooby_STEntry *stentry);
string print_active_features(vector<int32_t> active_features);
string print_active_features2(vector<int32_t> active_features);
uint64_t compress_address(uint64_t address);

#endif /* SCOOBY_HELPER_H */


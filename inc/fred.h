#ifndef FRED_H
#define FRED_H

#include <vector>
#include "bf/all.hpp"
using namespace std;

#define MAX_CANDIDATES 8

typedef enum
{
	Evaluate = 0,
	Apply,

	NumFredPhases
} FredPhase;

class Fred
{
private:
	uint32_t num_candidate_prefetchers;
	FredPhase phase;
	vector<bool> active_prefetchers;
	vector<uint32_t> scores;
	uint32_t access_counter;

	/* bloom filter */
	uint32_t opt_hash_functions;
	bf::basic_bloom_filter *bf;

	struct
	{
		int32_t curr_ptr;
	} eval;

	/* Statistics */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t apply_phase;
			uint64_t end_of_apply;
			uint64_t evaluation_phase;
			uint64_t filter_lookup;
			uint64_t filter_lookup_dist[MAX_CANDIDATES];
			uint64_t filter_hit;
			uint64_t filter_hit_dist[MAX_CANDIDATES];
			uint64_t filter_add;
			uint64_t filter_add_dist[MAX_CANDIDATES];
			uint64_t end_of_round;
			uint64_t end_of_evaluation;
		} invoke;

		struct
		{
			uint64_t called;
			uint64_t called_dist[MAX_CANDIDATES];
			uint64_t active;
			uint64_t active_dist[MAX_CANDIDATES];
			uint64_t inactive;
			uint64_t inactive_dist[MAX_CANDIDATES];
		} evaluate_score;
	} stats;

private:
	void init_knobs();
	void init_stats();
	void init_active_prefetchers();
	void reset_eval();
	void end_of_apply();
	void end_of_round(int32_t pref_index);
	void end_of_evaluation();
	void evaluate_prefetcher_score(int32_t pref_index);
	void filter_add(uint64_t address);
	bool filter_lookup(uint64_t address);

public:
	Fred();
	~Fred();
	void print_config();
	void invoke(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<vector<uint64_t> > pref_candidates);
	vector<bool> get_active_prefetchers();
	void dump_stats();
};

#endif /* FRED_H */


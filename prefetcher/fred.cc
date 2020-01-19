#include <stdlib.h>
#include <cmath>
#include "fred.h"
#include "champsim.h"

#if 0
#	define LOCKED(...) {fflush(stdout); __VA_ARGS__; fflush(stdout);}
#	define LOGID() fprintf(stdout, "[%25s@%3u] ", \
							__FUNCTION__, __LINE__ \
							);
#	define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#	define MYLOG(...) {}
#endif

namespace knob
{
	/* Fred knobs */
	extern uint32_t fred_bloom_filter_size;
	extern uint32_t fred_num_access_in_round;
	extern uint32_t fred_seed;
	extern uint32_t fred_init_pref_selection_type;
	extern uint32_t fred_num_access_in_apply_phase;
	extern float    fred_score_cutoff;

	/* Related Velma knobs */
	extern vector<string> velma_candidate_prefetchers;
}

void Fred::init_knobs()
{
	num_candidate_prefetchers = knob::velma_candidate_prefetchers.size();
}

void Fred::init_stats()
{
	bzero(&stats, sizeof(stats));
}

Fred::Fred()
{
	init_knobs();
	init_stats();
	init_active_prefetchers();
	
	phase = FredPhase::Evaluate;
	scores.resize(num_candidate_prefetchers, 0);
	access_counter = 0;

	/* init bloom filter */
	/* for a given m (bloom filter bit array length) and n (number of elements to be inserted)
	 * the k (number of hash functions) to produce minimal false +ve rate would be m/n*ln(2)
	 * https://en.wikipedia.org/wiki/Bloom_filter#Probability_of_false_positives */
	opt_hash_functions = (uint32_t)ceil(knob::fred_bloom_filter_size * log(2) / (knob::fred_num_access_in_round * 4)); /* 4 is for the pref degree of candidate prefetcher */
	bf = new bf::basic_bloom_filter(bf::make_hasher(opt_hash_functions, knob::fred_seed, true), knob::fred_bloom_filter_size);
	assert(bf);

	reset_eval();
}

Fred::~Fred()
{

}

void Fred::print_config()
{
	cout << "fred_bloom_filter_size " << knob::fred_bloom_filter_size << endl
		<< "fred_num_access_in_round " << knob::fred_num_access_in_round << endl
		<< "fred_opt_hash_functions " << opt_hash_functions << endl
		<< "fred_seed " << knob::fred_seed << endl
		<< "fred_init_pref_selection_type " << knob::fred_init_pref_selection_type << endl
		<< "fred_num_access_in_apply_phase " << knob::fred_num_access_in_apply_phase << endl
		<< "fred_score_cutoff " << knob::fred_score_cutoff << endl
		;
}

void Fred::init_active_prefetchers()
{
	switch(knob::fred_init_pref_selection_type)
	{
		case 1: /* select all */
			active_prefetchers.resize(num_candidate_prefetchers, true);
			break;

		case 2: /* select none */
			active_prefetchers.resize(num_candidate_prefetchers, false);
			break;

		case 3: /* select the first one */
			active_prefetchers.resize(num_candidate_prefetchers, false);
			active_prefetchers[0] = true;
			break;

		default:
			cout << "Unsupported fred_init_pref_selection_type " << knob::fred_init_pref_selection_type << endl;
			assert(false);
	}
}

void Fred::reset_eval()
{
	eval.curr_ptr = 0;
}

void Fred::invoke(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<vector<uint64_t> > pref_candidates)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);
	MYLOG("---------------------------------------------------------------------");
	MYLOG("%s %lx pc %lx page %lx off %u", GetAccessType(type), address, pc, page, offset);

	stats.invoke.called++;
	access_counter++;

	if(phase == FredPhase::Apply)
	{
		MYLOG("Phase: Apply");
		stats.invoke.apply_phase++;
		if(access_counter >= knob::fred_num_access_in_apply_phase)
		{
			end_of_apply();
			stats.invoke.end_of_apply++;
		}
	}
	else /* in evaluation phase */
	{
		MYLOG("Phase: Evaluation");
		stats.invoke.evaluation_phase++;
		assert(phase == FredPhase::Evaluate);
		assert(eval.curr_ptr != -1);

		/* =============================================
		 * Step 1: Probe bloom filter and manipulate prefetcher score
		 * Step 2: Insert generated prefetch address into bloom filter 
		 * Step 3: Check end of round and end of evaluation */

		/* Step 1: Probe bloom filter and manipulate prefetcher score */
		stats.invoke.filter_lookup++;
		stats.invoke.filter_lookup_dist[eval.curr_ptr]++;
		bool filter_hit = filter_lookup(address);
		if(filter_hit)
		{
			MYLOG("Addr %lx lookup HIT", address);
			scores[eval.curr_ptr]++;
			stats.invoke.filter_hit++;
			stats.invoke.filter_hit_dist[eval.curr_ptr]++;
		}

		/* Step 2: Insert generated prefetch address into bloom filter */
		vector<uint64_t> pref_addr = pref_candidates[eval.curr_ptr];
		MYLOG("pref_index %d pref_count %u", eval.curr_ptr, pref_addr.size());
		// cout << "pc " << setw(10) << hex << pc << dec 
		// 	<< " page " << setw(10) << hex << page << dec
		// 	<< " offset " << setw(2) << offset 
		// 	<< " pref " << eval.curr_ptr << " num_pref " << pref_addr.size() << endl;
		for(uint32_t index = 0; index < pref_addr.size(); ++index)
		{
			filter_add(pref_addr[index]);
			stats.invoke.filter_add++;
			stats.invoke.filter_add_dist[eval.curr_ptr]++;
		}

		/* Step 3: Check end of round and end of evaluation */
		/* End of round := one prefetcher has been evaluated. Turn to evaluate next prefetcher
		 * End of evaluation := all the prefetchers has been evaluated. Switch to apply phase */
		if(access_counter >= knob::fred_num_access_in_round)
		{
			end_of_round(eval.curr_ptr);
			int32_t next_ptr = eval.curr_ptr + 1;
			if(next_ptr < (int32_t)num_candidate_prefetchers)
			{
				eval.curr_ptr = next_ptr;
				stats.invoke.end_of_round++;
			}
			else
			{
				/* end of evaluation */
				end_of_evaluation();
				reset_eval();
				stats.invoke.end_of_evaluation++;
			}
		}
	}
}

void Fred::end_of_apply()
{
	/* clearup scores */
	scores.clear();
	scores.resize(num_candidate_prefetchers, 0);

	/* clearup bloom filter */
	bf->clear();

	/* reset access counter */
	access_counter = 0;

	/* switch phase */
	phase = FredPhase::Evaluate;

	MYLOG("Transitioning to Evaluation phase");
}

void Fred::end_of_round(int32_t pref_index)
{
	evaluate_prefetcher_score(pref_index);

	/* clearup bloom filter */
	bf->clear();

	/* reset access counter */
	access_counter = 0;

	MYLOG("End of round for prefetcher %d", pref_index);
}

void Fred::end_of_evaluation()
{
	/* swtich phase */
	phase = FredPhase::Apply;
	MYLOG("Transitioning to Apply phase");
}

void Fred::evaluate_prefetcher_score(int32_t pref_index)
{
	MYLOG("Pref %d score %u", pref_index, scores[pref_index]);

	stats.evaluate_score.called++;
	stats.evaluate_score.called_dist[pref_index]++;
	assert(phase == FredPhase::Evaluate);
	float cutoff_score = (float)knob::fred_num_access_in_round * knob::fred_score_cutoff;
	if(scores[pref_index] >= cutoff_score)
	{
		active_prefetchers[pref_index] = true;
		stats.evaluate_score.active++;
		stats.evaluate_score.active_dist[pref_index]++;
		MYLOG("Activate Pref %d", pref_index);
	}
	else
	{
		active_prefetchers[pref_index] = false;
		stats.evaluate_score.inactive++;
		stats.evaluate_score.inactive_dist[pref_index]++;
		MYLOG("Inactivate Pref %d", pref_index);
	}
}

void Fred::filter_add(uint64_t address)
{
	bf->add(address);
}

bool Fred::filter_lookup(uint64_t address)
{
	return bf->lookup(address);
}

vector<bool> Fred::get_active_prefetchers()
{
	return active_prefetchers;
}

void Fred::dump_stats()
{
	cout << "fred_invoke_called " << stats.invoke.called << endl
		<< "fred_invoke_apply_phase " << stats.invoke.apply_phase << endl
		<< "fred_invoke_end_of_apply " << stats.invoke.end_of_apply << endl
		<< "fred_invoke_evaluation_phase " << stats.invoke.evaluation_phase << endl
		<< "fred_invoke_end_of_round " << stats.invoke.end_of_round << endl
		<< "fred_invoke_end_of_evaluation " << stats.invoke.end_of_evaluation << endl
		<< "fred_invoke_filter_lookup " << stats.invoke.filter_lookup << endl
		<< "fred_invoke_filter_hit " << stats.invoke.filter_hit << endl
		<< "fred_invoke_filter_add " << stats.invoke.filter_add << endl
		<< endl;

	for(uint32_t index = 0; index < num_candidate_prefetchers; ++index)
	{
		cout << "fred_invoke_filter_lookup_" << index << " " << stats.invoke.filter_lookup_dist[index] << endl
			<< "fred_invoke_filter_hit_" << index << " " << stats.invoke.filter_hit_dist[index] << endl
			<< "fred_invoke_filter_add_" << index << " " << stats.invoke.filter_add_dist[index] << endl;
	}
	cout << endl;
		
	cout << "fred_evaluate_score_called " << stats.evaluate_score.called << endl
		<< "fred_evaluate_score_active " << stats.evaluate_score.active << endl
		<< "fred_evaluate_score_inactive " << stats.evaluate_score.inactive << endl
		<< endl;
		
	for(uint32_t index = 0; index < num_candidate_prefetchers; ++index)
	{	
		cout << "fred_evaluate_score_called_" << index << " " << stats.evaluate_score.called_dist[index] << endl
			<< "fred_evaluate_score_active_" << index << " " << stats.evaluate_score.active_dist[index] << endl
			<< "fred_evaluate_score_inactive_" << index << " " << stats.evaluate_score.inactive_dist[index] << endl;
	}
	cout << endl;
}
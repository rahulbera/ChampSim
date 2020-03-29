#include <assert.h>
#include <algorithm>
#include <iomanip>
#include "champsim.h"
#include "memory_class.h"
#include "scooby.h"
#include "util.h"

#if 0
#	define LOCKED(...) {fflush(stdout); __VA_ARGS__; fflush(stdout);}
#	define LOGID() fprintf(stdout, "[%25s@%3u] ", \
							__FUNCTION__, __LINE__ \
							);
#	define MYLOG(...) LOCKED(LOGID(); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n");)
#else
#	define MYLOG(...) {}
#endif


/* Action array
 * Basically a set of deltas to evaluate
 * Similar to the concept of BOP */
std::vector<int32_t> Actions;

namespace knob
{
	extern float    scooby_alpha;
	extern float    scooby_gamma;
	extern float    scooby_epsilon;
	extern uint32_t scooby_state_num_bits;
	extern uint32_t scooby_max_states;
	extern uint32_t scooby_seed;
	extern string   scooby_policy;
	extern string   scooby_learning_type;
	extern vector<int32_t> scooby_actions;
	extern uint32_t scooby_max_actions;
	extern uint32_t scooby_pt_size;
	extern uint32_t scooby_st_size;
	extern uint32_t scooby_max_pcs;
	extern uint32_t scooby_max_offsets;
	extern uint32_t scooby_max_deltas;
	extern int32_t  scooby_reward_none;
	extern int32_t  scooby_reward_incorrect;
	extern int32_t  scooby_reward_correct_untimely;
	extern int32_t  scooby_reward_correct_timely;
	extern bool		scooby_brain_zero_init;
	extern bool     scooby_enable_reward_all;
	extern bool     scooby_enable_track_multiple;
	extern bool     scooby_enable_reward_out_of_bounds;
	extern int32_t  scooby_reward_out_of_bounds;
	extern uint32_t scooby_state_type;
	extern bool     scooby_access_debug;
	extern bool     scooby_print_access_debug;
	extern bool     scooby_enable_state_action_stats;
	extern bool     scooby_enable_reward_tracker_hit;
	extern int32_t  scooby_reward_tracker_hit;
	extern bool     scooby_enable_shaggy;
	extern uint32_t scooby_state_hash_type;
	extern bool     scooby_prefetch_with_shaggy;
	extern bool     scooby_enable_cmac_engine;
	extern bool     scooby_enable_cmac2_engine;
	extern uint32_t scooby_pref_degree;
	extern bool     scooby_enable_dyn_degree;
	extern vector<float> scooby_max_to_avg_q_thresholds;
	extern vector<int32_t> scooby_dyn_degrees;
	extern uint64_t scooby_early_exploration_window;
	extern bool     scooby_enable_het_reward;
	extern int32_t  scooby_reward_fa_correct_timely;
	extern int32_t  scooby_reward_fa_correct_untimely;
	extern int32_t  scooby_reward_fa_incorrect;
	extern int32_t  scooby_reward_fa_none;
	extern int32_t  scooby_reward_fa_out_of_bounds;
	extern int32_t  scooby_reward_fa_tracker_hit;
	extern bool 	scooby_enable_pt_address_compression;
	extern uint32_t scooby_pt_address_hash_type;
	extern uint32_t scooby_pt_address_hashed_bits;

	/* Learning Engine knobs */
	extern bool     le_enable_trace;
	extern uint32_t le_trace_interval;
	extern std::string   le_trace_file_name;
	extern uint32_t le_trace_state;
	extern bool     le_enable_score_plot;
	extern std::vector<int32_t> le_plot_actions;
	extern std::string   le_plot_file_name;
	extern bool     le_enable_action_trace;
	extern uint32_t le_action_trace_interval;
	extern std::string le_action_trace_name;
	extern bool     le_enable_action_plot;

	/* CMAC engine knobs */
	extern uint32_t scooby_cmac_num_planes;
	extern uint32_t scooby_cmac_num_entries_per_plane;
	extern vector<int32_t> scooby_cmac_plane_offsets;
	extern vector<int32_t> scooby_cmac_feature_granularities;
	extern vector<int32_t> scooby_cmac_action_factors;
	extern uint32_t scooby_cmac_hash_type;

	/* CMAC engine 2.0 knobs */
	extern uint32_t scooby_cmac2_num_planes;
	extern uint32_t scooby_cmac2_num_entries_per_plane;
	extern vector<int32_t> scooby_cmac2_plane_offsets;
	extern vector<int32_t> scooby_cmac2_feature_granularities;
	extern uint32_t scooby_cmac2_hash_type;
	extern bool 		le_cmac2_enable_trace;
	extern string 		le_cmac2_trace_state;
	extern uint32_t 	le_cmac2_trace_interval;
	extern bool 		le_cmac2_enable_score_plot;
	extern vector<int32_t> le_cmac2_plot_actions;
	extern std::string 	le_cmac2_trace_file_name;
	extern std::string 	le_cmac2_plot_file_name;
	extern bool 		le_cmac2_state_action_debug;
	extern vector<float> le_cmac2_qvalue_threshold_levels;
	extern uint32_t 	le_cmac2_max_to_avg_q_ratio_type;
	extern float 	 	le_cmac2_max_q_thresh;
	extern uint32_t 	le_cmac2_state_type;
	extern vector<int32_t> le_cmac2_active_features;
	extern uint32_t 	le_cmac2_feature_shift_amount;
	extern bool         le_cmac2_enable_action_fallback;
}

void Scooby::init_knobs()
{
	Actions.resize(knob::scooby_max_actions, 0);
	std::copy(knob::scooby_actions.begin(), knob::scooby_actions.end(), Actions.begin());
	assert(Actions.size() == knob::scooby_max_actions);
	assert(Actions.size() <= MAX_ACTIONS);
	if(knob::scooby_access_debug)
	{
		cout << "***WARNING*** setting knob::scooby_max_pcs, knob::scooby_max_offsets, and knob::scooby_max_deltas to large value as knob::scooby_access_debug is true" << endl;
		knob::scooby_max_pcs = 1024;
		knob::scooby_max_offsets = 1024;
		knob::scooby_max_deltas = 1024;
	}
	assert(knob::scooby_pref_degree >= 1 && (knob::scooby_pref_degree == 1 || !knob::scooby_enable_dyn_degree));
	assert(knob::scooby_max_to_avg_q_thresholds.size() == knob::scooby_dyn_degrees.size()-1);
}

void Scooby::init_stats()
{
	bzero(&stats, sizeof(stats));
	stats.predict.action_dist.resize(knob::scooby_max_actions, 0);
	stats.predict.issue_dist.resize(knob::scooby_max_actions, 0);
	stats.predict.pred_hit.resize(knob::scooby_max_actions, 0);
	stats.predict.out_of_bounds_dist.resize(knob::scooby_max_actions, 0);
	state_action_dist.clear();
}

Scooby::Scooby(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	recorder = new ScoobyRecorder();

	last_evicted_tracker = NULL;

	/* init learning engine */
	brain_cmac = NULL;
	brain_cmac2 = NULL;
	brain = NULL;

	if(knob::scooby_enable_cmac_engine)
	{
		CMACConfig config;
		config.num_planes = knob::scooby_cmac_num_planes;
		config.num_entries_per_plane = knob::scooby_cmac_num_entries_per_plane;
		config.plane_offsets = knob::scooby_cmac_plane_offsets;
		config.feature_granularities = knob::scooby_cmac_feature_granularities;
		config.action_factors = knob::scooby_cmac_action_factors;
		config.hash_type = knob::scooby_cmac_hash_type;

		brain_cmac = new LearningEngineCMAC(config,
											this,
											knob::scooby_alpha, knob::scooby_gamma, knob::scooby_epsilon, 
											knob::scooby_max_actions, 
											knob::scooby_max_states,
											knob::scooby_seed,
											knob::scooby_policy,
											knob::scooby_learning_type,
											knob::scooby_brain_zero_init,
											knob::scooby_early_exploration_window);
	}
	else if(knob::scooby_enable_cmac2_engine)
	{
		CMACConfig config;
		config.num_planes = knob::scooby_cmac2_num_planes;
		config.num_entries_per_plane = knob::scooby_cmac2_num_entries_per_plane;
		config.plane_offsets = knob::scooby_cmac2_plane_offsets;
		config.feature_granularities = knob::scooby_cmac2_feature_granularities;
		config.hash_type = knob::scooby_cmac2_hash_type;

		brain_cmac2 = new LearningEngineCMAC2(config,
											this,
											knob::scooby_alpha, knob::scooby_gamma, knob::scooby_epsilon, 
											knob::scooby_max_actions, 
											knob::scooby_max_states,
											knob::scooby_seed,
											knob::scooby_policy,
											knob::scooby_learning_type,
											knob::scooby_brain_zero_init,
											knob::scooby_early_exploration_window);
	}
	else
	{
		brain = new LearningEngineBasic( this,
									knob::scooby_alpha, knob::scooby_gamma, knob::scooby_epsilon, 
									knob::scooby_max_actions, 
									knob::scooby_max_states,
									knob::scooby_seed,
									knob::scooby_policy,
									knob::scooby_learning_type,
									knob::scooby_brain_zero_init,
									knob::scooby_early_exploration_window);
	}

	/* init Shaggy */
	if(knob::scooby_enable_shaggy)
	{
		shaggy = new Shaggy();	
	}

	bw_level = 0;
	core_ipc = 0;
}

Scooby::~Scooby()
{
	if(brain_cmac) 	delete brain_cmac;
	if(brain_cmac2) delete brain_cmac2;
	if(brain) 		delete brain;
}

void Scooby::print_config()
{
	cout << "scooby_alpha " << knob::scooby_alpha << endl
		<< "scooby_gamma " << knob::scooby_gamma << endl
		<< "scooby_epsilon " << knob::scooby_epsilon << endl
		<< "scooby_state_num_bits " << knob::scooby_state_num_bits << endl
		<< "scooby_max_states " << knob::scooby_max_states << endl
		<< "scooby_seed " << knob::scooby_seed << endl
		<< "scooby_policy " << knob::scooby_policy << endl
		<< "scooby_learning_type " << knob::scooby_learning_type << endl
		<< "scooby_actions " << array_to_string(Actions) << endl
		<< "scooby_max_actions " << knob::scooby_max_actions << endl
		<< "scooby_pt_size " << knob::scooby_pt_size << endl
		<< "scooby_st_size " << knob::scooby_st_size << endl
		<< "scooby_max_pcs " << knob::scooby_max_pcs << endl
		<< "scooby_max_offsets " << knob::scooby_max_offsets << endl
		<< "scooby_max_deltas " << knob::scooby_max_deltas << endl
		<< "scooby_reward_none " << knob::scooby_reward_none << endl
		<< "scooby_reward_incorrect " << knob::scooby_reward_incorrect << endl
		<< "scooby_reward_correct_untimely " << knob::scooby_reward_correct_untimely << endl
		<< "scooby_reward_correct_timely " << knob::scooby_reward_correct_timely << endl
		<< "scooby_brain_zero_init " << knob::scooby_brain_zero_init << endl
		<< "scooby_enable_reward_all " << knob::scooby_enable_reward_all << endl
		<< "scooby_enable_track_multiple " << knob::scooby_enable_track_multiple << endl
		<< "scooby_enable_reward_out_of_bounds " << knob::scooby_enable_reward_out_of_bounds << endl
		<< "scooby_reward_out_of_bounds " << knob::scooby_reward_out_of_bounds << endl
		<< "scooby_state_type " << knob::scooby_state_type << endl
		<< "scooby_state_hash_type " << knob::scooby_state_hash_type << endl
		<< "scooby_access_debug " << knob::scooby_access_debug << endl
		<< "scooby_print_access_debug " << knob::scooby_print_access_debug << endl
		<< "scooby_enable_state_action_stats " << knob::scooby_enable_state_action_stats << endl
		<< "scooby_enable_reward_tracker_hit " << knob::scooby_enable_reward_tracker_hit << endl
		<< "scooby_reward_tracker_hit " << knob::scooby_reward_tracker_hit << endl
		<< "scooby_enable_shaggy " << knob::scooby_enable_shaggy << endl
		<< "scooby_prefetch_with_shaggy " << knob::scooby_prefetch_with_shaggy << endl
		<< "scooby_enable_cmac_engine " << knob::scooby_enable_cmac_engine << endl
		<< "scooby_enable_cmac2_engine " << knob::scooby_enable_cmac2_engine << endl
		<< "scooby_pref_degree " << knob::scooby_pref_degree << endl
		<< "scooby_enable_dyn_degree " << knob::scooby_enable_dyn_degree << endl
		<< "scooby_max_to_avg_q_thresholds " << array_to_string(knob::scooby_max_to_avg_q_thresholds) << endl
		<< "scooby_dyn_degrees " << array_to_string(knob::scooby_dyn_degrees) << endl
		<< "scooby_early_exploration_window " << knob::scooby_early_exploration_window << endl
		<< "scooby_enable_het_reward " << knob::scooby_enable_het_reward << endl
		<< "scooby_reward_fa_correct_timely " << knob::scooby_reward_fa_correct_timely << endl
		<< "scooby_reward_fa_correct_untimely " << knob::scooby_reward_fa_correct_untimely << endl
		<< "scooby_reward_fa_incorrect " << knob::scooby_reward_fa_incorrect << endl
		<< "scooby_reward_fa_none " << knob::scooby_reward_fa_none << endl
		<< "scooby_reward_fa_out_of_bounds " << knob::scooby_reward_fa_out_of_bounds << endl
		<< "scooby_reward_fa_tracker_hit " << knob::scooby_reward_fa_tracker_hit << endl
		<< "scooby_enable_pt_address_compression " << knob::scooby_enable_pt_address_compression << endl
		<< "scooby_pt_address_hash_type " << knob::scooby_pt_address_hash_type << endl
		<< "scooby_pt_address_hashed_bits " << knob::scooby_pt_address_hashed_bits << endl
		<< endl
		<< "le_enable_trace " << knob::le_enable_trace << endl
		<< "le_trace_interval " << knob::le_trace_interval << endl
		<< "le_trace_file_name " << knob::le_trace_file_name << endl
		<< "le_trace_state " << hex << knob::le_trace_state << dec << endl
		<< "le_enable_score_plot " << knob::le_enable_score_plot << endl
		<< "le_plot_file_name " << knob::le_plot_file_name << endl
		<< "le_plot_actions " << array_to_string(knob::le_plot_actions) << endl
		<< "le_enable_action_trace " << knob::le_enable_action_trace << endl
		<< "le_action_trace_interval " << knob::le_action_trace_interval << endl
		<< "le_action_trace_name " << knob::le_action_trace_name << endl
		<< "le_enable_action_plot " << knob::le_enable_action_plot << endl
		<< endl
		<< "scooby_cmac_num_planes " << knob::scooby_cmac_num_planes << endl
		<< "scooby_cmac_num_entries_per_plane " << knob::scooby_cmac_num_entries_per_plane << endl
		<< "scooby_cmac_plane_offsets " << array_to_string(knob::scooby_cmac_plane_offsets, true) << endl
		<< "scooby_cmac_feature_granularities " << array_to_string(knob::scooby_cmac_feature_granularities) << endl
		<< "scooby_cmac_action_factors " << array_to_string(knob::scooby_cmac_action_factors, true) << endl
		<< "scooby_cmac_hash_type " << knob::scooby_cmac_hash_type << endl
		<< endl
		<< "scooby_cmac2_num_planes " << knob::scooby_cmac2_num_planes << endl
		<< "scooby_cmac2_num_entries_per_plane " << knob::scooby_cmac2_num_entries_per_plane << endl
		<< "scooby_cmac2_plane_offsets " << array_to_string(knob::scooby_cmac2_plane_offsets, true) << endl
		<< "scooby_cmac2_feature_granularities " << array_to_string(knob::scooby_cmac2_feature_granularities) << endl
		<< "scooby_cmac2_hash_type " << knob::scooby_cmac2_hash_type << endl
		<< endl
		<< "le_cmac2_enable_trace " << knob::le_cmac2_enable_trace << endl
		<< "le_cmac2_trace_interval " << knob::le_cmac2_trace_interval << endl
		<< "le_cmac2_trace_file_name " << knob::le_cmac2_trace_file_name << endl
		<< "le_cmac2_trace_state " << hex << knob::le_cmac2_trace_state << dec << endl
		<< "le_cmac2_enable_score_plot " << knob::le_cmac2_enable_score_plot << endl
		<< "le_cmac2_plot_file_name " << knob::le_cmac2_plot_file_name << endl
		<< "le_cmac2_plot_actions " << array_to_string(knob::le_cmac2_plot_actions) << endl
		<< "le_cmac2_state_action_debug " << knob::le_cmac2_state_action_debug << endl
		<< "le_cmac2_qvalue_threshold_levels " << array_to_string(knob::le_cmac2_qvalue_threshold_levels) << endl
		<< "le_cmac2_max_to_avg_q_ratio_type " << knob::le_cmac2_max_to_avg_q_ratio_type << endl
		<< "le_cmac2_max_q_thresh " << knob::le_cmac2_max_q_thresh << endl
		<< "le_cmac2_state_type " << knob::le_cmac2_state_type << endl
		<< "le_cmac2_active_features " << print_active_features(knob::le_cmac2_active_features) << endl
		<< "le_cmac2_feature_shift_amount " << knob::le_cmac2_feature_shift_amount << endl
		<< "le_cmac2_enable_action_fallback " << knob::le_cmac2_enable_action_fallback << endl
		<< endl;
		
	if(knob::scooby_enable_shaggy)
	{
		shaggy->print_config();
	}
}

void Scooby::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	MYLOG("---------------------------------------------------------------------");
	MYLOG("%s %lx pc %lx page %lx off %u", GetAccessType(type), address, pc, page, offset);

	/* compute reward on demand */
	reward(address);

	/* record the access: just to gain some insights from the workload
	 * defined in scooby_helper.h(cc) */
	recorder->record_access(pc, address, page, offset);

	/* global state tracking */
	update_global_state(pc, page, offset, address);
	/* per page state tracking */
	Scooby_STEntry *stentry = update_local_state(pc, page, offset, address);

	/* Measure state.
	 * state can contain per page local information like delta signature, pc signature etc.
	 * it can also contain global signatures like last three branch PCs etc.
	 */
	State *state = new State();
	state->pc = pc;
	state->address = address;
	state->page = page;
	state->offset = offset;
	state->delta = !stentry->deltas.empty() ? stentry->deltas.back() : 0;
	state->local_delta_sig = stentry->get_delta_sig();
	state->local_delta_sig2 = stentry->get_delta_sig2();
	state->local_pc_sig = stentry->get_pc_sig();
	state->local_offset_sig = stentry->get_offset_sig();

	/* Shaggy only predicts for streaming accesses */
	bool cond_streaming = (knob::scooby_enable_shaggy && stentry->streaming);
	if(cond_streaming)
	{
		shaggy->predict(page, offset, pref_addr);
		stats.predict.shaggy_called++;
		stats.pref_issue.shaggy += pref_addr.size();
	}

	/* call Scooby's prediction in two cases:
	 * 1. The page is not streaming
	 * 2. The page is streaming, but still want to invoke scooby so that Scooby's learning doesn't get hampered */
	if(!cond_streaming || knob::scooby_prefetch_with_shaggy)
	{
		uint32_t count = pref_addr.size();
		predict(address, page, offset, state, pref_addr);
		stats.pref_issue.scooby += (pref_addr.size() - count);
	}
}

void Scooby::update_global_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	/* @rbera TODO: implement */
}

Scooby_STEntry* Scooby::update_local_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	stats.st.lookup++;
	Scooby_STEntry *stentry = NULL;
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](Scooby_STEntry *stentry){return stentry->page == page;});
	if(st_index != signature_table.end())
	{
		stats.st.hit++;
		stentry = (*st_index);
		stentry->update(page, pc, offset, address);
		signature_table.erase(st_index);
		signature_table.push_back(stentry);
		return stentry;
	}
	else
	{
		if(signature_table.size() >= knob::scooby_st_size)
		{
			stats.st.evict++;
			stentry = signature_table.front();
			signature_table.pop_front();
			if(knob::scooby_access_debug)
			{
				recorder->record_access_knowledge(stentry);
				if(knob::scooby_print_access_debug)
				{
					print_access_debug(stentry);
				}
			}
			if(knob::scooby_enable_shaggy)
			{
				shaggy->record_signature(stentry);
			}
			delete stentry;
		}

		stats.st.insert++;
		stentry = new Scooby_STEntry(page, pc, offset);
		recorder->record_trigger_access(page, pc, offset);
		if(knob::scooby_enable_shaggy)
		{
			stentry->streaming = shaggy->lookup_signature(pc, page, offset);
			stats.st.streaming++;
		}
		signature_table.push_back(stentry);
		return stentry;
	}
}

uint32_t Scooby::predict(uint64_t base_address, uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr)
{
	MYLOG("addr@%lx page %lx off %u state %x", base_address, page, offset, state->value());

	stats.predict.called++;
	uint32_t state_index = state->value();
	assert(state_index < knob::scooby_max_states);

	/* query learning engine to get the next prediction */
	uint32_t action_index = 0;
	uint32_t pref_degree = knob::scooby_pref_degree;

	if(knob::scooby_enable_cmac_engine)
	{
		action_index = brain_cmac->chooseAction(state);
		if(knob::scooby_enable_state_action_stats)
		{
			update_stats(state, action_index, pref_degree);
		}
	}
	else if(knob::scooby_enable_cmac2_engine)
	{
		float max_to_avg_q_ratio = 1.0;
		action_index = brain_cmac2->chooseAction(state, max_to_avg_q_ratio);
		if(knob::scooby_enable_dyn_degree)
		{
			pref_degree = get_dyn_pref_degree(max_to_avg_q_ratio);
		}
		if(knob::scooby_enable_state_action_stats)
		{
			update_stats(state, action_index, pref_degree);
		}
	}
	else
	{
		action_index = brain->chooseAction(state_index);
		if(knob::scooby_enable_state_action_stats)
		{
			update_stats(state_index, action_index, pref_degree);
		}
	}
	assert(action_index < knob::scooby_max_actions);

	MYLOG("act_idx %u act %d", action_index, Actions[action_index]);

	uint64_t addr = 0xdeadbeef;
	Scooby_PTEntry *ptentry = NULL;
	int32_t predicted_offset = 0;
	if(Actions[action_index] != 0)
	{
		predicted_offset = (int32_t)offset + Actions[action_index];
		if(predicted_offset >=0 && predicted_offset < 64)
		{
			addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
			MYLOG("pred_off %d pred_addr %lx", predicted_offset, addr);
			/* track prefetch */
			bool new_addr = track(addr, state, action_index, &ptentry);
			if(new_addr)
			{
				pref_addr.push_back(addr);
				track_in_st(page, predicted_offset); /* will be used for debugging */
				stats.predict.issue_dist[action_index]++;
				if(pref_degree > 1)
				{
					gen_multi_degree_pref(page, offset, Actions[action_index], pref_degree, pref_addr);
				}
			}
			else
			{
				MYLOG("pred_off %d tracker_hit", predicted_offset);
				stats.predict.pred_hit[action_index]++;
				if(knob::scooby_enable_reward_tracker_hit)
				{
					addr = 0xdeadbeef;
					track(addr, state, action_index, &ptentry);
					assert(ptentry);
					// assign_reward(ptentry, knob::scooby_reward_tracker_hit);
					assign_reward(ptentry, RewardType::tracker_hit);
				}
			}
			stats.predict.action_dist[action_index]++;
		}
		else
		{
			MYLOG("pred_off %d out_of_bounds", predicted_offset);
			stats.predict.out_of_bounds++;
			stats.predict.out_of_bounds_dist[action_index]++;
			if(knob::scooby_enable_reward_out_of_bounds)
			{
				addr = 0xdeadbeef;
				track(addr, state, action_index, &ptentry);
				assert(ptentry);
				// assign_reward(ptentry, knob::scooby_reward_out_of_bounds);
				assign_reward(ptentry, RewardType::out_of_bounds);
			}
		}
	}
	else
	{
		MYLOG("no prefecth");
		/* agent decided not to prefetch */
		addr = 0xdeadbeef;
		/* track no prefetch */
		track(addr, state, action_index, &ptentry);
		stats.predict.action_dist[action_index]++;
	}

	stats.predict.predicted += pref_addr.size();
	MYLOG("end@%lx", base_address);

	return pref_addr.size();
}

/* Returns true if the address is not already present in prefetch_tracker
 * false otherwise */
bool Scooby::track(uint64_t address, State *state, uint32_t action_index, Scooby_PTEntry **tracker)
{
	MYLOG("addr@%lx state %x act_idx %u act %d", address, state->value(), action_index, Actions[action_index]);
	stats.track.called++;

	bool new_addr = true;
	vector<Scooby_PTEntry*> ptentries = search_pt(address, false);
	if(ptentries.empty())
	{
		new_addr = true;
	}
	else
	{
		new_addr = false;
	}

	if(!new_addr && address != 0xdeadbeef && !knob::scooby_enable_track_multiple)
	{
		stats.track.same_address++;
		tracker = NULL;
		return new_addr;
	}

	Scooby_PTEntry *ptentry = NULL;

	if(prefetch_tracker.size() >= knob::scooby_pt_size)
	{
		stats.track.evict++;
		ptentry = prefetch_tracker.front();
		prefetch_tracker.pop_front();
		MYLOG("victim_state %x victim_act_idx %u victim_act %d", ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index]);
		if(last_evicted_tracker)
		{
			MYLOG("last_victim_state %x last_victim_act_idx %u last_victim_act %d", last_evicted_tracker->state->value(), last_evicted_tracker->action_index, Actions[last_evicted_tracker->action_index]);
			train(ptentry, last_evicted_tracker);
			delete last_evicted_tracker->state;
			delete last_evicted_tracker;
		}
		last_evicted_tracker = ptentry;
	}

	ptentry = new Scooby_PTEntry(address, state, action_index);
	if(knob::scooby_enable_pt_address_compression && ptentry->address != 0xdeadbeef)
	{
		ptentry->address = compress_address(ptentry->address);
	}
	prefetch_tracker.push_back(ptentry);
	assert(prefetch_tracker.size() <= knob::scooby_pt_size);

	(*tracker) = ptentry;
	MYLOG("end@%lx", address);

	return new_addr;
}

void Scooby::gen_multi_degree_pref(uint64_t page, uint32_t offset, int32_t action, uint32_t pref_degree, vector<uint64_t> &pref_addr)
{
	stats.predict.multi_deg_called++;
	uint64_t addr = 0xdeadbeef;
	int32_t predicted_offset = 0;
	if(action != 0)
	{
		for(uint32_t degree = 2; degree < pref_degree; ++degree)
		{
			predicted_offset = (int32_t)offset + degree * action;
			if(predicted_offset >=0 && predicted_offset < 64)
			{
				addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
				pref_addr.push_back(addr);
				MYLOG("degree %u pred_off %d pred_addr %lx", degree, predicted_offset, addr);
				stats.predict.multi_deg++;
				stats.predict.multi_deg_histogram[degree]++;
			}
		}
	}
}

uint32_t Scooby::get_dyn_pref_degree(float max_to_avg_q_ratio)
{
	uint32_t counted = false;
	uint32_t degree = 1;
	for(uint32_t index = 0; index < knob::scooby_max_to_avg_q_thresholds.size(); ++index)
	{
		if(max_to_avg_q_ratio < knob::scooby_max_to_avg_q_thresholds[index])
		{
			degree = knob::scooby_dyn_degrees[index];
			counted = true;
			break;
		}
	}
	if(!counted)
	{
		degree = knob::scooby_dyn_degrees.back();
	}
	return degree;
}

/* This reward fucntion is called after seeing a demand access to the address */
/* TODO: what if multiple prefetch request generated the same address?
 * Currently, it just rewards the oldest prefetch request to the address
 * Should we reward all? */
void Scooby::reward(uint64_t address)
{
	MYLOG("addr @ %lx", address);

	stats.reward.demand.called++;
	vector<Scooby_PTEntry*> ptentries = search_pt(address, knob::scooby_enable_reward_all);
	
	if(ptentries.empty())
	{
		MYLOG("PT miss");
		stats.reward.demand.pt_not_found++;
		return;
	}
	else
	{
		stats.reward.demand.pt_found++;
	}

	for(uint32_t index = 0; index < ptentries.size(); ++index)
	{
		Scooby_PTEntry *ptentry = ptentries[index];
		stats.reward.demand.pt_found_total++;

		MYLOG("PT hit. state %x act_idx %u act %d", ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index]);
		/* Do not compute reward if already has a reward.
		 * This can happen when a prefetch access sees multiple demand reuse */
		if(ptentry->has_reward)
		{
			MYLOG("entry already has reward: %d", ptentry->reward);
			stats.reward.demand.has_reward++;
			return;
		}

		if(ptentry->is_filled) /* timely */
		{
			stats.reward.correct_timely++;
			stats.reward.dist[ptentry->action_index][RewardType::correct_timely]++;
			ptentry->reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_correct_timely : knob::scooby_reward_correct_timely;
			MYLOG("assigned reward correct_timely(%d)", ptentry->reward);
		}
		else
		{
			stats.reward.correct_untimely++;
			stats.reward.dist[ptentry->action_index][RewardType::correct_untimely]++;
			ptentry->reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_correct_untimely :knob::scooby_reward_correct_untimely;
			MYLOG("assigned reward correct_untimely(%d)", ptentry->reward);
		}
		ptentry->has_reward = true;
	}
}

/* This reward function is called during eviction from prefetch_tracker */
void Scooby::reward(Scooby_PTEntry *ptentry)
{
	MYLOG("reward PT evict %lx state %x act_idx %u act %d", ptentry->address, ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index]);

	stats.reward.train.called++;
	assert(!ptentry->has_reward);
	/* this is called during eviction from prefetch tracker
	 * that means, this address doesn't see a demand reuse.
	 * hence it either can be incorrect, or no prefetch */
	if(ptentry->address == 0xdeadbeef) /* no prefetch */
	{
		stats.reward.no_pref++;
		stats.reward.dist[ptentry->action_index][RewardType::none]++;
		ptentry->reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_none : knob::scooby_reward_none;
		MYLOG("assigned reward no_pref(%d)", ptentry->reward);
	}
	else /* incorrect prefetch */
	{
		stats.reward.incorrect++;
		stats.reward.dist[ptentry->action_index][RewardType::incorrect]++;
		ptentry->reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_incorrect : knob::scooby_reward_incorrect;
		MYLOG("assigned reward incorrect(%d)", ptentry->reward);
	}
	ptentry->has_reward = true;
}

void Scooby::assign_reward(Scooby_PTEntry *ptentry, RewardType type)
{
	MYLOG("assign_reward PT evict %lx state %x act_idx %u act %d", ptentry->address, ptentry->state->value(), ptentry->action_index, Actions[ptentry->action_index]);
	stats.reward.assign_reward.called++;
	assert(!ptentry->has_reward);
	int32_t reward = 0;

	switch(type)
	{
		case RewardType::out_of_bounds:
			reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_out_of_bounds : knob::scooby_reward_out_of_bounds;
			stats.reward.out_of_bounds++;
			MYLOG("assigned reward out_of_bounds(%d)", reward);
			break;

		case RewardType::tracker_hit:
			reward = (knob::scooby_enable_het_reward && ptentry->action_index == 0) ? knob::scooby_reward_fa_tracker_hit : knob::scooby_reward_tracker_hit;
			stats.reward.tracker_hit++;
			MYLOG("assigned reward tracker_hit(%d)", reward);
			break;

		default:
			cout << "assign_reward called for invalid reward type: " << type << endl;
			assert(false);
	}

	stats.reward.dist[ptentry->action_index][type]++;
	ptentry->reward = reward;
	ptentry->has_reward = true;
}

void Scooby::train(Scooby_PTEntry *curr_evicted, Scooby_PTEntry *last_evicted)
{
	MYLOG("victim %s %u %d last_victim %s %u %d", curr_evicted->state->to_string().c_str(), curr_evicted->action_index, Actions[curr_evicted->action_index],
												last_evicted->state->to_string().c_str(), last_evicted->action_index, Actions[last_evicted->action_index]);

	stats.train.called++;
	if(!last_evicted->has_reward)
	{
		stats.train.compute_reward++;
		reward(last_evicted);
	}
	assert(last_evicted->has_reward);

	/* train */
	MYLOG("===SARSA=== S1: %s A1: %u R1: %d S2: %s A2: %u", last_evicted->state->to_string().c_str(), last_evicted->action_index, 
															last_evicted->reward, 
															curr_evicted->state->to_string().c_str(), curr_evicted->action_index);
	if(knob::scooby_enable_cmac_engine)
	{
		brain_cmac->learn(last_evicted->state, last_evicted->action_index, last_evicted->reward, curr_evicted->state, curr_evicted->action_index);
	}
	else if(knob::scooby_enable_cmac2_engine)
	{
		brain_cmac2->learn(last_evicted->state, last_evicted->action_index, last_evicted->reward, curr_evicted->state, curr_evicted->action_index);
	}
	else
	{
		brain->learn(last_evicted->state->value(), last_evicted->action_index, last_evicted->reward, curr_evicted->state->value(), curr_evicted->action_index);
	}
	MYLOG("train done");
}

/* TODO: what if multiple prefetch request generated the same address?
 * Currently it just sets the fill bit of the oldest prefetch request.
 * Do we need to set it for everyone? */
void Scooby::register_fill(uint64_t address)
{
	MYLOG("fill @ %lx", address);

	stats.register_fill.called++;
	vector<Scooby_PTEntry*> ptentries = search_pt(address, knob::scooby_enable_reward_all);
	if(!ptentries.empty())
	{
		stats.register_fill.set++;
		for(uint32_t index = 0; index < ptentries.size(); ++index)
		{
			stats.register_fill.set_total++;
			ptentries[index]->is_filled = true;
			MYLOG("fill PT hit. pref with act_idx %u act %d", ptentries[index]->action_index, Actions[ptentries[index]->action_index]);
		}
	}
}

void Scooby::register_prefetch_hit(uint64_t address)
{
	MYLOG("pref_hit @ %lx", address);

	stats.register_prefetch_hit.called++;
	vector<Scooby_PTEntry*> ptentries = search_pt(address, knob::scooby_enable_reward_all);
	if(!ptentries.empty())
	{
		stats.register_prefetch_hit.set++;
		for(uint32_t index = 0; index < ptentries.size(); ++index)
		{
			stats.register_prefetch_hit.set_total++;
			ptentries[index]->pf_cache_hit = true;
			MYLOG("pref_hit PT hit. pref with act_idx %u act %d", ptentries[index]->action_index, Actions[ptentries[index]->action_index]);
		}
	}
}

vector<Scooby_PTEntry*> Scooby::search_pt(uint64_t address, bool search_all)
{
	if(knob::scooby_enable_pt_address_compression && address != 0xdeadbeef)
	{
		uint32_t compressed_addr = compress_address(address);
		MYLOG("addr %lx compressed_addr %x", address, compressed_addr);
		cout << "addr " << hex << address << dec << " compressed_addr " << hex << compressed_addr << dec << endl;
		address = compressed_addr;
	}
	vector<Scooby_PTEntry*> entries;
	for(uint32_t index = 0; index < prefetch_tracker.size(); ++index)
	{
		if(prefetch_tracker[index]->address == address)
		{
			entries.push_back(prefetch_tracker[index]);
			if(!search_all) break;
		}
	}
	return entries;
}

void Scooby::update_stats(uint32_t state, uint32_t action_index, uint32_t pref_degree)
{
	auto it = state_action_dist.find(state);
	if(it != state_action_dist.end())
	{
		it->second[action_index]++;
	}
	else
	{
		vector<uint64_t> act_dist;
		act_dist.resize(knob::scooby_max_actions, 0);
		act_dist[action_index]++;
		state_action_dist.insert(std::pair<uint32_t, vector<uint64_t> >(state, act_dist));
	}
}

void Scooby::update_stats(State *state, uint32_t action_index, uint32_t degree)
{
	string state_str = state->to_string();
	auto it = state_action_dist2.find(state_str);
	if(it != state_action_dist2.end())
	{
		it->second[action_index]++;
		it->second[knob::scooby_max_actions]++; /* counts total occurences of this state */
	}
	else
	{
		vector<uint64_t> act_dist;
		act_dist.resize(knob::scooby_max_actions+1, 0);
		act_dist[action_index]++;
		act_dist[knob::scooby_max_actions]++; /* counts total occurences of this state */
		state_action_dist2.insert(std::pair<string, vector<uint64_t> >(state_str, act_dist));
	}

	auto it2 = action_deg_dist.find(getAction(action_index));
	if(it2 != action_deg_dist.end())
	{
		it2->second[degree]++;
	}
	else
	{
		vector<uint64_t> deg_dist;
		deg_dist.resize(MAX_SCOOBY_DEGREE, 0);
		deg_dist[degree]++;
		action_deg_dist.insert(std::pair<int32_t, vector<uint64_t> >(getAction(action_index), deg_dist));
	}
}

int32_t Scooby::getAction(uint32_t action_index)
{
	assert(action_index < Actions.size());
	return Actions[action_index];
}

void Scooby::track_in_st(uint64_t page, uint32_t pred_offset)
{
	auto st_index = find_if(signature_table.begin(), signature_table.end(), [page](Scooby_STEntry *stentry){return stentry->page == page;});
	if(st_index != signature_table.end())
	{
		(*st_index)->track_prefetch(pred_offset);
	}
}

void Scooby::update_bw(uint8_t bw)
{
	assert(bw < SCOOBY_MAX_BW_LEVEL);
	bw_level = bw;
	stats.bandwidth.epochs++;
	stats.bandwidth.histogram[bw_level]++;
}

void Scooby::update_ipc(uint8_t ipc)
{
	assert(ipc < SCOOBY_MAX_IPC_LEVEL);
	core_ipc = ipc;
	stats.ipc.epochs++;
	stats.ipc.histogram[ipc]++;
}

void Scooby::dump_stats()
{
	cout << "scooby_st_lookup " << stats.st.lookup << endl
		<< "scooby_st_hit " << stats.st.hit << endl
		<< "scooby_st_evict " << stats.st.evict << endl
		<< "scooby_st_insert " << stats.st.insert << endl
		<< "scooby_st_streaming " << stats.st.streaming << endl
		<< endl
		
		<< "scooby_predict_called " << stats.predict.called << endl
		<< "scooby_predict_shaggy_called " << stats.predict.shaggy_called << endl
		<< "scooby_predict_out_of_bounds " << stats.predict.out_of_bounds << endl;

	for(uint32_t index = 0; index < Actions.size(); ++index)
	{
		cout << "scooby_predict_action_" << Actions[index] << " " << stats.predict.action_dist[index] << endl;
		cout << "scooby_predict_issue_action_" << Actions[index] << " " << stats.predict.issue_dist[index] << endl;
		cout << "scooby_predict_hit_action_" << Actions[index] << " " << stats.predict.pred_hit[index] << endl;
		cout << "scooby_predict_out_of_bounds_action_" << Actions[index] << " " << stats.predict.out_of_bounds_dist[index] << endl;
	}

	cout << "scooby_predict_multi_deg_called " << stats.predict.multi_deg_called << endl
		<< "scooby_predict_predicted " << stats.predict.predicted << endl
		<< "scooby_predict_multi_deg " << stats.predict.multi_deg << endl;
	for(uint32_t index = 2; index < MAX_SCOOBY_DEGREE; ++index)
	{
		cout << "scooby_predict_multi_deg_" << index << " " << stats.predict.multi_deg_histogram[index] << endl;
	}
	cout << endl;

	if(knob::scooby_enable_state_action_stats)
	{
		if(knob::scooby_enable_cmac_engine || knob::scooby_enable_cmac2_engine)
		{
			std::vector<std::pair<string, vector<uint64_t> > > pairs;
			for (auto itr = state_action_dist2.begin(); itr != state_action_dist2.end(); ++itr)
			    pairs.push_back(*itr);
			sort(pairs.begin(), pairs.end(), [](std::pair<string, vector<uint64_t>>& a, std::pair<string, vector<uint64_t>>& b){return a.second[knob::scooby_max_actions] > b.second[knob::scooby_max_actions];});
			for(auto it = pairs.begin(); it != pairs.end(); ++it)
			{
				cout << "scooby_state_" << hex << it->first << dec << " ";
				for(uint32_t index = 0; index < it->second.size(); ++index)
				{
					cout << it->second[index] << ",";
				}
				cout << endl;
			}			
		}
		else
		{
			for(auto it = state_action_dist.begin(); it != state_action_dist.end(); ++it)
			{
				cout << "scooby_state_" << hex << it->first << dec << " ";
				for(uint32_t index = 0; index < it->second.size(); ++index)
				{
					cout << it->second[index] << ",";
				}
				cout << endl;
			}
		}
	}
	cout << endl;

	for(auto it = action_deg_dist.begin(); it != action_deg_dist.end(); ++it)
	{
		cout << "scooby_action_" << it->first << "_deg_dist ";
		for(uint32_t index = 0; index < MAX_SCOOBY_DEGREE; ++index)
		{
			cout << it->second[index] << ",";
		}
		cout << endl;
	}
	cout << endl;

	cout << "scooby_track_called " << stats.track.called << endl
		<< "scooby_track_same_address " << stats.track.same_address << endl
		<< "scooby_track_evict " << stats.track.evict << endl
		<< endl

		<< "scooby_reward_demand_called " << stats.reward.demand.called << endl
		<< "scooby_reward_demand_pt_not_found " << stats.reward.demand.pt_not_found << endl
		<< "scooby_reward_demand_pt_found " << stats.reward.demand.pt_found << endl
		<< "scooby_reward_demand_pt_found_total " << stats.reward.demand.pt_found_total << endl
		<< "scooby_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
		<< "scooby_reward_train_called " << stats.reward.train.called << endl
		<< "scooby_reward_assign_reward_called " << stats.reward.assign_reward.called << endl
		<< "scooby_reward_no_pref " << stats.reward.no_pref << endl
		<< "scooby_reward_incorrect " << stats.reward.incorrect << endl
		<< "scooby_reward_correct_untimely " << stats.reward.correct_untimely << endl
		<< "scooby_reward_correct_timely " << stats.reward.correct_timely << endl
		<< "scooby_reward_out_of_bounds " << stats.reward.out_of_bounds << endl
		<< "scooby_reward_tracker_hit " << stats.reward.tracker_hit << endl
		<< endl;
	for(uint32_t action = 0; action < Actions.size(); ++action)
	{
		cout << "scooby_reward_" << Actions[action] << " ";
		for(uint32_t reward = 0; reward < RewardType::num_rewards; ++reward)
		{
			cout << stats.reward.dist[action][reward] << ",";
		}
		cout << endl;
	}


	cout << endl 
		<< "scooby_train_called " << stats.train.called << endl
		<< "scooby_train_compute_reward " << stats.train.compute_reward << endl
		<< endl

		<< "scooby_register_fill_called " << stats.register_fill.called << endl
		<< "scooby_register_fill_set " << stats.register_fill.set << endl
		<< "scooby_register_fill_set_total " << stats.register_fill.set_total << endl
		<< endl

		<< "scooby_register_prefetch_hit_called " << stats.register_prefetch_hit.called << endl
		<< "scooby_register_prefetch_hit_set " << stats.register_prefetch_hit.set << endl
		<< "scooby_register_prefetch_hit_set_total " << stats.register_prefetch_hit.set_total << endl
		<< endl

		<< "scooby_pref_issue_scooby " << stats.pref_issue.scooby << endl
		<< "scooby_pref_issue_shaggy " << stats.pref_issue.shaggy << endl
		<< endl;

	std::vector<std::pair<string, uint64_t>> pairs;
	for (auto itr = target_action_state.begin(); itr != target_action_state.end(); ++itr)
	    pairs.push_back(*itr);
	sort(pairs.begin(), pairs.end(), [](std::pair<string, uint64_t>& a, std::pair<string, uint64_t>& b){return a.second > b.second;});
	for(auto it = pairs.begin(); it != pairs.end(); ++it)
	{
		cout << it->first << "," << it->second << endl;
	}

	if(brain_cmac)
	{
		brain_cmac->dump_stats();
	}
	if(brain_cmac2)
	{
		brain_cmac2->dump_stats();
	}
	if(brain)
	{
		brain->dump_stats();
	}
	recorder->dump_stats();

	if(knob::scooby_enable_shaggy)
	{
		shaggy->dump_stats();
	}

	cout << "scooby_bw_epochs " << stats.bandwidth.epochs << endl;
	for(uint32_t index = 0; index < SCOOBY_MAX_BW_LEVEL; ++index)
	{
		cout << "scooby_bw_level_" << index << " " << stats.bandwidth.histogram[index] << endl;
	}
	cout << endl;

	cout << "scooby_ipc_epochs " << stats.ipc.epochs << endl;
	for(uint32_t index = 0; index < SCOOBY_MAX_IPC_LEVEL; ++index)
	{
		cout << "scooby_ipc_level_" << index << " " << stats.ipc.histogram[index] << endl;
	}
	cout << endl;
}

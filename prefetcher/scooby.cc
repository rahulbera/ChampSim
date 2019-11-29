#include <assert.h>
#include <algorithm>
#include <iomanip>
#include "champsim.h"
#include "scooby.h"

/* Action array
 * Basically a set of deltas to evaluate
 * Similar to the concept of BOP */
std::vector<int32_t> Actions;

namespace knob
{
	extern float    scooby_alpha;
	extern float    scooby_gamma;
	extern float    scooby_epsilon;
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
}

uint32_t State::value()
{
	return (uint32_t)(pc % knob::scooby_max_states);
}

void Scooby::init_knobs()
{
	Actions.resize(knob::scooby_max_actions, 0);
	std::copy(knob::scooby_actions.begin(), knob::scooby_actions.end(), Actions.begin());
	assert(Actions.size() == knob::scooby_max_actions);
	assert(Actions.size() <= MAX_ACTIONS);
}

void Scooby::init_stats()
{
	bzero(&stats, sizeof(stats));
	stats.predict.action_dist.resize(knob::scooby_max_actions, 0);
}

Scooby::Scooby(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	last_evicted_tracker = NULL;

	/* init learning engine */
	brain = new LearningEngine(knob::scooby_alpha, knob::scooby_gamma, knob::scooby_epsilon, 
								knob::scooby_max_actions, 
								knob::scooby_max_states,
								knob::scooby_seed,
								knob::scooby_policy,
								knob::scooby_learning_type,
								knob::scooby_brain_zero_init);
	printf("*****WARNING: only modulo hash of PC is implemented*****\n");
}

Scooby::~Scooby()
{

}

void Scooby::print_config()
{
	cout << "scooby_alpha " << knob::scooby_alpha << endl
		<< "scooby_gamma " << knob::scooby_gamma << endl
		<< "scooby_epsilon " << knob::scooby_epsilon << endl
		<< "scooby_max_states " << knob::scooby_max_states << endl
		<< "scooby_seed " << knob::scooby_seed << endl
		<< "scooby_policy " << knob::scooby_policy << endl
		<< "scooby_learning_type " << knob::scooby_learning_type << endl
		<< "scooby_actions ";
	for(uint32_t index = 0; index < Actions.size(); ++index)
	{
		cout << Actions[index] << ",";
	}
	cout << endl
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
		<< endl;
}

void Scooby::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

//	cout << "[ACCESS]"
//		<< " pc " << hex << pc
//		<< " addr " << hex << address
//		<< " page " << hex << page
//		<< " offset " << dec << offset
//		<< endl;

	/* compute reward on demand */
	reward(address);

	/* global state tracking */
	update_state(pc, page, offset, address);
	/* per page state tracking */
	Scooby_STEntry *stentry = update_st(pc, page, offset, address);

	/* Measure state.
	 * state can contain per page local information like delta signature, pc signature etc.
	 * it can also contain global signatures like last three branch PCs etc.
	 */
	State *state = new State();
	state->pc = pc;
	state->page = page;
	state->offset = offset;
	state->delta = !stentry->deltas.empty() ? stentry->deltas.back() : 0;
	state->local_delta_sig = stentry->get_delta_sig();
	state->local_pc_sig = stentry->get_pc_sig();

	/* predict */
	predict(page, offset, state, pref_addr);
}

void Scooby::update_state(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
{
	/* @rbera TODO: implement */
}

Scooby_STEntry* Scooby::update_st(uint64_t pc, uint64_t page, uint32_t offset, uint64_t address)
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
			delete stentry;
		}

		stats.st.insert++;
		stentry = new Scooby_STEntry(page, pc, offset);
		signature_table.push_back(stentry);
		return stentry;
	}
}

uint32_t Scooby::predict(uint64_t page, uint32_t offset, State *state, vector<uint64_t> &pref_addr)
{
//	cout << "   [PREDICT]"
//		<< " page " << hex << page
//		<< " offset " << dec << offset
//		<< " state " << hex << state->value();

	stats.predict.called++;
	uint32_t state_index = state->value();
	assert(state_index < knob::scooby_max_states);

	/* query learning engine to get the next prediction */
	uint32_t action_index = brain->chooseAction(state_index);
	assert(action_index < knob::scooby_max_actions);

//	cout << " act_idx " << dec << action_index << " act " << dec << Actions[action_index];

	uint64_t addr = 0xdeadbeef;
	int32_t predicted_offset = 0;
	if(Actions[action_index] != 0)
	{
		predicted_offset = (int32_t)offset + Actions[action_index];
//		cout << " p_off " << predicted_offset;
		if(predicted_offset >=0 && predicted_offset < 64)
		{
			addr = (page << LOG2_PAGE_SIZE) + (predicted_offset << LOG2_BLOCK_SIZE);
			pref_addr.push_back(addr);
//			cout << " p_addr " << hex << addr << endl;
			/* track prefetch */
			track(addr, state, action_index);
			stats.predict.action_dist[action_index]++;
		}
		else
		{
//			cout << " out_of_bounds" << endl;
			stats.predict.out_of_bounds++;
		}
	}
	else
	{
//		cout << " no_pref" << endl;
		/* agent decided not to prefetch */
		addr = 0xdeadbeef;
		/* track no prefetch */
		track(addr, state, action_index);
		stats.predict.action_dist[action_index]++;
	}

	stats.predict.predicted += pref_addr.size();
	return pref_addr.size();
}

void Scooby::track(uint64_t address, State *state, uint32_t action_index)
{
//	cout << "      [TRACK]"
//		<< " addr " << hex << address
//		<< " state " << state->value()
//		<< " act_idx " << action_index
//		<< " act " << Actions[action_index];

	stats.track.called++;
	Scooby_PTEntry *ptentry = NULL;

	if(prefetch_tracker.size() >= knob::scooby_pt_size)
	{
		stats.track.evict++;
		ptentry = prefetch_tracker.front();
		prefetch_tracker.pop_front();
//		cout << " v_state " << hex << ptentry->state->value() << " v_act " << dec << Actions[ptentry->action_index];
		if(last_evicted_tracker)
		{
//			cout << " lv_state " << hex << last_evicted_tracker->state->value() << " lv_act " << dec << Actions[last_evicted_tracker->action_index] << endl;
			train(ptentry, last_evicted_tracker);
			delete last_evicted_tracker->state;
			delete last_evicted_tracker;
		}
		else
		{
//			cout << endl;
		}
		last_evicted_tracker = ptentry;
	}
	else
	{
//		cout << endl;
	}

	ptentry = new Scooby_PTEntry(address, state, action_index);
	prefetch_tracker.push_back(ptentry);
	assert(prefetch_tracker.size() <= knob::scooby_pt_size);
}

/* This reward fucntion is called after seeing a demand access to the address */
/* TODO: what if multiple prefetch request generated the same address?
 * Currently, it just rewards the oldest prefetch request to the address
 * Should we reward all? */
void Scooby::reward(uint64_t address)
{
//	cout << "   [REWARD_D]"
//		<< " addr " << hex << address;

	stats.reward.demand.called++;
	Scooby_PTEntry *ptentry = search_pt(address);
	if(!ptentry)
	{
//		cout << " miss" << endl;
		stats.reward.demand.pt_not_found++;
		return;
	}
//	cout << " found. state " << hex << ptentry->state->value() << " act " << dec << Actions[ptentry->action_index];
	/* Do not compute reward if already has a reward.
	 * This can happen when a prefetch access sees multiple demand reuse */
	if(ptentry->has_reward)
	{
//		cout << " already has reward" << endl;
		stats.reward.demand.has_reward++;
		return;
	}

	if(ptentry->is_filled) /* timely */
	{
//		cout << " correct_timely" << " reward " << knob::scooby_reward_correct_timely << endl;
		stats.reward.correct_timely++;
		stats.reward.dist[ptentry->action_index][RewardType::correct_timely]++;
		ptentry->reward = knob::scooby_reward_correct_timely;
	}
	else
	{
//		cout << " correct_untimely" << " reward " << knob::scooby_reward_correct_untimely << endl;
		stats.reward.correct_untimely++;
		stats.reward.dist[ptentry->action_index][RewardType::correct_untimely]++;
		ptentry->reward = knob::scooby_reward_correct_untimely;
	}
	ptentry->has_reward = true;
}

/* This reward function is called during eviction from prefetch_tracker */
void Scooby::reward(Scooby_PTEntry *ptentry)
{
//	cout << "            [REWARD_T]"
//		<< " addr " << hex << ptentry->address
//		<< " state " << hex << ptentry->state->value()
//		<< " act_idx " << dec << ptentry->action_index
//		<< " act " << dec << Actions[ptentry->action_index];

	stats.reward.train.called++;
	assert(!ptentry->has_reward);
	/* this is called during eviction from prefetch tracker
	 * that means, this address doesn't see a demand reuse.
	 * hence it either can be incorrect, or no prefetch */
	if(ptentry->address == 0xdeadbeef) /* no prefetch */
	{
//		cout << " no_pref " << " reward " << knob::scooby_reward_none << endl;
		stats.reward.no_pref++;
		stats.reward.dist[ptentry->action_index][RewardType::none]++;
		ptentry->reward = knob::scooby_reward_none;
	}
	else /* incorrect prefetch */
	{
//		cout << " incorrect " << " reward " << knob::scooby_reward_incorrect << endl;
		stats.reward.incorrect++;
		stats.reward.dist[ptentry->action_index][RewardType::incorrect]++;
		ptentry->reward = knob::scooby_reward_incorrect;
	}
	ptentry->has_reward = true;
}

void Scooby::train(Scooby_PTEntry *curr_evicted, Scooby_PTEntry *last_evicted)
{
//	cout << "         [TRAIN]"
//		<< " v_state " << hex << curr_evicted->state->value()
//		<< " v_act " << dec << Actions[curr_evicted->action_index]
//		<< " lv_state " << hex << last_evicted->state->value()
//		<< " lv_act " << dec << Actions[last_evicted->action_index];

	stats.train.called++;
	if(!last_evicted->has_reward)
	{
//		cout << " no_reward" << endl;
		stats.train.compute_reward++;
		reward(last_evicted);
	}
	else
	{
//		cout << " reward " << last_evicted->reward << endl;
	}
	assert(last_evicted->has_reward);

	/* train */
	brain->learn(last_evicted->state->value(), last_evicted->action_index, last_evicted->reward, curr_evicted->state->value(), curr_evicted->action_index);
}

/* TODO: what if multiple prefetch request generated the same address?
 * Currently it just sets the fill bit of the oldest prefetch request.
 * Do we need to set it for everyone? */
void Scooby::register_fill(uint64_t address)
{
//	cout << "[REG_FILL]"
//		<< " addr " << hex << address;

	stats.register_fill.called++;
	Scooby_PTEntry *ptentry = search_pt(address);
	if(ptentry)
	{
//		cout << " found in PT" << endl;
		stats.register_fill.set++;
		ptentry->is_filled = true;
	}
	else
	{
//		cout << " not found in PT" << endl;
	}
}

void Scooby::register_prefetch_hit(uint64_t address)
{
//	cout << "[REG_PF_HIT]"
//		<< " addr " << hex << address;

	stats.register_prefetch_hit.called++;
	Scooby_PTEntry *ptentry = search_pt(address);
	if(ptentry)
	{
//		cout << " found in PT" << endl;
		stats.register_prefetch_hit.set++;
		ptentry->pf_cache_hit = true;
	}
	else
	{
//		cout << " not found in PT" << endl;
	}
}

Scooby_PTEntry* Scooby::search_pt(uint64_t address)
{
	auto it = find_if(prefetch_tracker.begin(), prefetch_tracker.end(), [address](Scooby_PTEntry *ptentry){return ptentry->address == address;});
	return it != prefetch_tracker.end() ? (*it) : NULL;
}

void Scooby::dump_stats()
{
	cout << "scooby_st_lookup " << stats.st.lookup << endl
		<< "scooby_st_hit " << stats.st.hit << endl
		<< "scooby_st_evict " << stats.st.evict << endl
		<< "scooby_st_insert " << stats.st.insert << endl
		<< endl
		
		<< "scooby_stats_predict_called " << stats.predict.called << endl
		<< "scooby_stats_predict_out_of_bounds " << stats.predict.out_of_bounds << endl;

	for(uint32_t index = 0; index < Actions.size(); ++index)
	{
		cout << "scooby_predict_action_" << Actions[index] << " " << stats.predict.action_dist[index] << endl;
	}

	cout << "scooby_stats_predict_predicted " << stats.predict.predicted << endl
		<< endl 
		<< "scooby_track_called " << stats.track.called << endl
		<< "scooby_track_evict " << stats.track.evict << endl
		<< endl

		<< "scooby_reward_demand_called " << stats.reward.demand.called << endl
		<< "scooby_reward_demand_pt_not_found " << stats.reward.demand.pt_not_found << endl
		<< "scooby_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
		<< "scooby_reward_train_called " << stats.reward.train.called << endl
		<< "scooby_reward_no_pref " << stats.reward.no_pref << endl
		<< "scooby_reward_incorrect " << stats.reward.incorrect << endl
		<< "scooby_reward_correct_untimely " << stats.reward.correct_untimely << endl
		<< "scooby_reward_correct_timely " << stats.reward.correct_timely << endl
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
		<< endl

		<< "scooby_register_prefetch_hit_called " << stats.register_prefetch_hit.called << endl
		<< "scooby_register_prefetch_hit_set " << stats.register_prefetch_hit.set << endl
		<< endl;

	brain->dump_stats();
}

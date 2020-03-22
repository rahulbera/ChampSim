#include <iostream>
#include <assert.h>
#include <algorithm>
#include "champsim.h"
#include "memory_class.h"
#include "velma.h"
#include "sms.h"
#include "scooby.h"

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
	extern vector<string> 	velma_candidate_prefetchers;
	extern uint32_t velma_pref_selection_type;
	extern double	velma_alpha;
	extern double	velma_gamma;
	extern double	velma_epsilon;
	extern uint32_t velma_max_actions;
	extern uint32_t	velma_max_states;
	extern uint32_t	velma_seed;
	extern string	velma_policy;
	extern string	velma_learning_type;
	extern bool		velma_brain_zero_init;
	extern uint32_t velma_state_type;
	extern vector<int32_t> velma_actions;
	extern uint32_t velma_pt_size;
	extern int32_t	velma_reward_accurate;
	extern int32_t	velma_reward_inaccurate;
}

const char* VelmaRewardTypeString[] = {"Accurate", "Inaccurate"};
const char* MapVelmaRewardTypeString(VelmaRewardType type)
{
	assert((uint32_t)type < NumVelmaRewardTypes);
	return VelmaRewardTypeString[type];
} 

uint32_t VelmaState::value()
{
	uint32_t value = 0;
	switch(knob::velma_state_type)
	{
		case 1: /* no state information at all */
			value = 0;
			break;

		case 2:
			value = (uint32_t)pc % knob::velma_max_states;
			break;

		default:
			cout << "Unsupported velma_state_type " << knob::velma_state_type << endl;
			assert(false);
	}

	return value;
}

void Velma::init_knobs()
{
	assert(knob::velma_max_actions < MAX_CANDIDATE_PREFETCHERS);
	assert(knob::velma_max_actions <= knob::velma_candidate_prefetchers.size());
}

void Velma::init_stats()
{
	bzero(&stats, sizeof(stats));
}

Velma::Velma(string type) : Prefetcher(type)
{
	init_knobs();
	init_stats();

	for(uint32_t index = 0; index < knob::velma_candidate_prefetchers.size(); ++index)
	{
		if(!knob::velma_candidate_prefetchers[index].compare("sms"))
		{
			cout << "[Velma] Adding candidate: SMS" << endl;
			SMSPrefetcher *pref_sms = new SMSPrefetcher(knob::velma_candidate_prefetchers[index]);
			m_prefetchers.push_back(pref_sms);
		}
		else if(!knob::velma_candidate_prefetchers[index].compare("scooby"))
		{
			cout << "[Velma] Adding candidate: Scooby" << endl;
			Scooby *pref_scooby = new Scooby(knob::velma_candidate_prefetchers[index]);
			m_prefetchers.push_back(pref_scooby);
		}
		else if(!knob::velma_candidate_prefetchers[index].compare("none"))
		{
			cout << "[Velma] Cannot add prefetcher: none" << endl;
			exit(12);
		}
		else
		{
			cout << "[Velma] Unsupported prefetcher type: " << knob::velma_candidate_prefetchers[index] << endl;
			assert(false);
		}
	}
	assert(m_prefetchers.size() == knob::velma_candidate_prefetchers.size());

	/* init Velma's action list */
	for(uint32_t index = 0; index < knob::velma_actions.size(); ++index)
	{
		m_actions.push_back(knob::velma_actions[index]);
	}

	/* init Velma's brain */
	m_brain = NULL;
	if(knob::velma_pref_selection_type == 2)
	{
		m_brain = new LearningEngineBasic(this,
									knob::velma_alpha, knob::velma_gamma, knob::velma_epsilon, 
									knob::velma_max_actions, 
									knob::velma_max_states,
									knob::velma_seed,
									knob::velma_policy,
									knob::velma_learning_type,
									knob::velma_brain_zero_init,
									0 /* early exploration window */);
	}
	last_evicted_tracker = NULL;

	/* init Fred */
	m_fred = NULL;
	if(knob::velma_pref_selection_type == 3)
	{
		m_fred = new Fred();
	}
}

Velma::~Velma()
{
	if(m_brain) delete m_brain;
	if(m_fred)	delete m_fred;
}

void Velma::print_config()
{
	/* Call print_config of all candidate prefetchers */
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		m_prefetchers[index]->print_config();
	}

	/* Print own config parameters */
	cout << "velma_candidate_prefetchers ";
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << m_prefetchers[index]->get_type() << ",";
	}
	cout << endl;
	cout << "velma_pref_selection_type " << knob::velma_pref_selection_type << endl 
		<< "velma_alpha " << knob::velma_alpha << endl
		<< "velma_gamma " << knob::velma_gamma << endl
		<< "velma_epsilon " << knob::velma_epsilon << endl
		<< "velma_max_actions " << knob::velma_max_actions << endl
		<< "velma_max_states " << knob::velma_max_states << endl
		<< "velma_seed " << knob::velma_seed << endl
		<< "velma_policy " << knob::velma_policy << endl
		<< "velma_learning_type " << knob::velma_learning_type << endl
		<< "velma_brain_zero_init " << knob::velma_brain_zero_init << endl
		<< "velma_state_type " << knob::velma_state_type << endl
		<< "velma_actions ";
	
	for(uint32_t index = 0; index < knob::velma_actions.size(); ++index)
	{
		cout << knob::velma_actions[index] << ",";
	}
	
	cout << endl
		<< "velma_pt_size " << knob::velma_pt_size << endl
		<< "velma_reward_accurate " << knob::velma_reward_accurate << endl
		<< "velma_reward_inaccurate " << knob::velma_reward_inaccurate << endl
		<< endl;

	if(m_fred)
	{
		m_fred->print_config();
		cout << endl;
	}
}

void Velma::invoke_prefetcher(uint64_t pc, uint64_t address, uint8_t cache_hit, uint8_t type, vector<uint64_t> &pref_addr)
{
	uint64_t page = address >> LOG2_PAGE_SIZE;
	uint32_t offset = (address >> LOG2_BLOCK_SIZE) & ((1ull << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE)) - 1);

	MYLOG("---------------------------------------------------------------------");
	MYLOG("%s %lx pc %lx page %lx off %u", GetAccessType(type), address, pc, page, offset);

	stats.invoke_prefetcher.called++;

	/* compute reward on demand access */
	if(knob::velma_pref_selection_type == 2)
	{
		reward(address);
	}
	
	/* call invoke prefetcher for each individual candidate prefetchers */
	vector<vector<uint64_t> > pref_candidates;
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		vector<uint64_t> pref_addr_local;
		pref_addr_local.clear();
		
		m_prefetchers[index]->invoke_prefetcher(pc, address, cache_hit, type, pref_addr_local);
		pref_candidates.push_back(pref_addr_local);

		stats.invoke_prefetcher.called_dist[index]++;
		stats.invoke_prefetcher.pref_dist[index] += pref_addr_local.size();
	}
	assert(pref_candidates.size() == m_prefetchers.size());
	MYLOG("generated prefetches: %lu %lu", pref_candidates[0].size(), pref_candidates[1].size()); /* WARNING: hard-coded array access */

	/* call Fred */
	if(knob::velma_pref_selection_type == 3)
	{
		m_fred->invoke(pc, address, cache_hit, type, pref_candidates);
	}

	/* record the state attributes */
	VelmaState *vstate = new VelmaState();
	vstate->pc = pc;
	vstate->page = page;
	vstate->offset = offset;

	/* Select prefetchers whose candidates will be issued finally.
	 * In a multi-prefetcher framework, this list can contain multiple prefetchers */
	uint32_t action_index = 0;
	unordered_set<uint32_t> selected_pref_list = select_prefetchers(vstate, action_index);

	/* prepare the final prefetch candidate list */
	populate_prefetch_list(vstate, action_index, selected_pref_list, pref_candidates, pref_addr);
	
	/* destroy vstate object, as the clone is already stored */
	delete vstate;
}

unordered_set<uint32_t> Velma::select_prefetchers(VelmaState *vstate, uint32_t &action_index)
{
	action_index = 0;
	stats.selection.called++;
	unordered_set<uint32_t> selected_pref_list;

	if(knob::velma_pref_selection_type == 1) /* Select ALL */
	{
		for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
		{
			selected_pref_list.insert(index);
			stats.selection.dist[index]++;
		}
	}
	else if (knob::velma_pref_selection_type == 2) /* RL-based selection */
	{
		uint32_t state_value = vstate->value();
		assert(state_value < knob::velma_max_states);

		/* call the RL agent to choose prefetcher(s) */
		action_index = m_brain->chooseAction(state_value);
		assert(action_index < m_actions.size());
		assert(m_actions[action_index] < (int32_t)m_prefetchers.size());

		selected_pref_list.insert(m_actions[action_index]);
		stats.selection.dist[m_actions[action_index]]++;
	}
	else if (knob::velma_pref_selection_type == 3) /* Bloom filter based selection */
	{
		vector<bool> active_prefetchers = m_fred->get_active_prefetchers();
		assert(active_prefetchers.size() == m_prefetchers.size());
		for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
		{
			if(active_prefetchers[index])
			{
				selected_pref_list.insert(index);
				stats.selection.dist[index]++;
			}
		}
	}
	else
	{
		cout << "[Velma] Unsupported velma_pref_selection_type " << knob::velma_pref_selection_type << endl;
		assert(false); 		
	}
	return selected_pref_list;
}

void Velma::populate_prefetch_list(VelmaState *vstate, uint32_t action_index, unordered_set<uint32_t> selected_pref_list, vector<vector<uint64_t> > pref_candidates, vector<uint64_t> &pref_addr)
{
	assert(pref_candidates.size() == m_prefetchers.size());
	MYLOG("state %x act_idx %u", vstate->value(), action_index);

	for(uint32_t index = 0; index < pref_candidates.size(); ++index)
	{
		if(selected_pref_list.find(index) != selected_pref_list.end()) /* means the prefetcher is selected */
		{
			for(uint32_t index2 = 0; index2 < pref_candidates[index].size(); ++index2)
			{
				pref_addr.push_back(pref_candidates[index][index2]);
				stats.issue.pref_dist[index]++;
				stats.issue.total++;

				/* track prefetch requests in case of RL selector */
				if(knob::velma_pref_selection_type == 2)
				{
					/* create a copy of state object, to avoid any problem in freeing pointers later */ 
					VelmaState *tmp_vstate = new VelmaState(*vstate); 
					track_prefetch(tmp_vstate, action_index, pref_candidates[index][index2]);
				}
			}
		}
	}
}

bool Velma::track_prefetch(VelmaState *vstate, uint32_t action_index, uint64_t pref_addr)
{
	MYLOG("addr@%lx state %x act_idx %u", pref_addr, vstate->value(), action_index);
	stats.track.called++;
	
	/* search for the address in PT */
	auto pt_index = find_if(prefetch_tracker.begin(), prefetch_tracker.end(), [pref_addr](VelmaPTEntry *ptentry){return (ptentry->address == pref_addr);});
	bool pt_hit = (pt_index != prefetch_tracker.end());
	if(pt_index == prefetch_tracker.end())
	{
		VelmaPTEntry *ptentry = NULL;
		if(prefetch_tracker.size() >= knob::velma_pt_size)
		{
			stats.track.evict++;
			ptentry = prefetch_tracker.front();
			prefetch_tracker.pop_front();
			MYLOG("victim_state %x victim_act_idx %u victim_addr %lx has_reward %u reward %d", ptentry->vstate->value(), ptentry->action_index, ptentry->address, ptentry->has_reward, ptentry->reward);
			if(last_evicted_tracker)
			{
				MYLOG("last_victim_state %x last_victim_act_idx %u last_victim_addr %lx has_reward %u reward %d", last_evicted_tracker->vstate->value(), last_evicted_tracker->action_index, last_evicted_tracker->address, last_evicted_tracker->has_reward, last_evicted_tracker->reward);
				train(ptentry, last_evicted_tracker);
				delete last_evicted_tracker->vstate;
				delete last_evicted_tracker;
			}
			last_evicted_tracker = ptentry;
		}

		ptentry = new VelmaPTEntry();
		ptentry->vstate = vstate;
		ptentry->action_index = action_index;
		ptentry->address = pref_addr;
		prefetch_tracker.push_back(ptentry);
		stats.track.insert++;
	}
	else
	{
		/* the same cacheline address is prefetched again */
		stats.track.tracker_hit++;
	}
	
	MYLOG("done@%lx", pref_addr);
	return pt_hit;
}

void Velma::train(VelmaPTEntry *curr_evicted_tracker, VelmaPTEntry *last_evicted_tracker)
{
	stats.train.called++;
	if(!last_evicted_tracker->has_reward)
	{
		stats.train.called_reward++;
		reward(last_evicted_tracker);
	}
	assert(last_evicted_tracker->has_reward);

	/* SARSA */
	MYLOG("===SARSA=== S1: %x A1: %u R1: %d S2: %x A2: %u", last_evicted_tracker->vstate->value(), last_evicted_tracker->action_index,
															last_evicted_tracker->reward,
															curr_evicted_tracker->vstate->value(), curr_evicted_tracker->action_index);
	m_brain->learn(last_evicted_tracker->vstate->value(), last_evicted_tracker->action_index, last_evicted_tracker->reward, curr_evicted_tracker->vstate->value(), curr_evicted_tracker->action_index);
	MYLOG("train done");
}

/* this reward function is called after seeing a demand access */
void Velma::reward(uint64_t address)
{
	stats.reward.demand.called++;
	auto pt_index = find_if(prefetch_tracker.begin(), prefetch_tracker.end(), [address](VelmaPTEntry *ptentry){return (ptentry->address == address);});
	if(pt_index != prefetch_tracker.end())
	{
		stats.reward.demand.hit++;
		/* demand hit on  prefetched line */
		VelmaPTEntry *ptentry = (*pt_index);
		if(!ptentry->has_reward)
		{
			ptentry->reward = knob::velma_reward_accurate;
			ptentry->has_reward = true;
			stats.reward.dist[VelmaRewardType::Accurate]++;
			stats.reward.action_reward_dist[ptentry->action_index][VelmaRewardType::Accurate]++;
		}
		else
		{
			stats.reward.demand.has_reward++;
		}
	}
	else
	{
		stats.reward.demand.pt_miss++;
	}
}

/* this reward function is called when pt_entry is getting evicted from tracker list
 * and yet has not computed reward, i.e., hasn't seen any demand access to the cacheline */
void Velma::reward(VelmaPTEntry *ptentry)
{
	stats.reward.pt_evict.called++;
	assert(!ptentry->has_reward);
	ptentry->reward = knob::velma_reward_inaccurate;
	ptentry->has_reward = true;
	stats.reward.dist[VelmaRewardType::Inaccurate]++;
	stats.reward.action_reward_dist[ptentry->action_index][VelmaRewardType::Inaccurate]++;
}

void Velma::register_fill(uint64_t address)
{
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		if(!m_prefetchers[index]->get_type().compare("scooby"))
		{
			Scooby *pref_scooby = (Scooby*)m_prefetchers[index];
			pref_scooby->register_fill(address);
		}
	}
}

void Velma::register_prefetch_hit(uint64_t address)
{
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		if(!m_prefetchers[index]->get_type().compare("scooby"))
		{
			Scooby *pref_scooby = (Scooby*)m_prefetchers[index];
			pref_scooby->register_prefetch_hit(address);
		}
	}
}

void Velma::dump_stats()
{
	/* print stats of individual candidate prefetchers first */
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		m_prefetchers[index]->dump_stats();
	}

	/* TODO: print own stats */
	cout << "velma_invoke_prefetcher_called " << stats.invoke_prefetcher.called << endl;
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << "velma_invoke_prefetcher_called_dist_" << m_prefetchers[index]->get_type() << " " << stats.invoke_prefetcher.called_dist[index] << endl;
		cout << "velma_invoke_prefetcher_pref_dist_" << m_prefetchers[index]->get_type() << " " << stats.invoke_prefetcher.pref_dist[index] << endl;
	}

	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << "velma_issue_pref_dist_" << m_prefetchers[index]->get_type() << " " << stats.issue.pref_dist[index] << endl; 
	}
	cout << endl;
	
	cout << "velma_selection_called " << stats.selection.called << endl;
	for(uint32_t index = 0; index < m_prefetchers.size(); ++index)
	{
		cout << "velma_selection_prefetcher_index_" << index << " " << stats.selection.dist[index] << endl;
	}

	cout << endl
		<< "velma_track_called " << stats.track.called << endl
		<< "velma_track_evict " << stats.track.evict << endl
		<< "velma_track_insert " << stats.track.insert << endl
		<< "velma_track_tracker_hit " << stats.track.tracker_hit << endl
		<< endl
		<< "velma_train_called " << stats.train.called << endl
		<< "velma_train_called_reward " << stats.train.called_reward << endl
		<< endl
		<< "velma_reward_demand_called " << stats.reward.demand.called << endl
		<< "velma_reward_demand_hit " << stats.reward.demand.hit << endl
		<< "velma_reward_demand_has_reward " << stats.reward.demand.has_reward << endl
		<< "velma_reward_demand_pt_miss " << stats.reward.demand.pt_miss << endl
		<< "velma_reward_pt_evict_called " << stats.reward.pt_evict.called << endl
		<< endl;

	for(uint32_t index = 0; index < NumVelmaRewardTypes; ++index)
	{
		cout << "velma_reward_dist_type_" << MapVelmaRewardTypeString((VelmaRewardType)index) << " " << stats.reward.dist[index] << endl;
	}
	cout << endl;
	for(uint32_t index = 0; index < knob::velma_max_actions; ++index)
	{
		cout << "velma_action_reward_dist_action_" << index << " ";
		for(uint32_t index2 = 0; index2 < NumVelmaRewardTypes; ++index2)
		{
			cout << stats.reward.action_reward_dist[index][index2] << ",";
		}
		cout << endl;
	}
	cout << endl;

	/* dump stats from Velma's brain */
	if(m_brain)
	{	
		m_brain->dump_stats();
	}

	/* Fred stats */
	if(m_fred)
	{
		m_fred->dump_stats();
	}
}
#include <iostream>
#include <vector>
#include <cassert>
#include <deque>
#include <strings.h>
#include "learning_engine_cmac2.h"
#include "util.h"
#include "scooby.h"
#include "statezoo.h"


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
	extern bool 		le_cmac2_enable_trace;
	extern string 		le_cmac2_trace_state;
	extern uint32_t 	le_cmac2_trace_interval;
	extern bool 		le_cmac2_enable_score_plot;
	extern vector<int32_t> le_cmac2_plot_actions;
	extern std::string 	le_cmac2_trace_file_name;
	extern std::string 	le_cmac2_plot_file_name;
	extern bool         le_cmac2_state_action_debug;
	extern vector<float> le_cmac2_qvalue_threshold_levels;
	extern uint32_t 	le_cmac2_max_to_avg_q_ratio_type;
	extern float		le_cmac2_max_q_thresh;
	extern uint32_t 	le_cmac2_state_type;
	extern vector<int32_t> le_cmac2_active_features;
}

LearningEngineCMAC2::LearningEngineCMAC2(CMACConfig config, Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type, bool zero_init, uint64_t early_exploration_window)
	: LearningEngineBase(p, alpha, gamma, epsilon, actions, states, seed, policy, type)
{
	m_num_planes = config.num_planes;
	m_num_entries_per_plane = config.num_entries_per_plane;
	m_plane_offsets = config.plane_offsets;
	m_feature_granularities = config.feature_granularities;
	m_hash_type = config.hash_type;

	/* check integrity */
	assert(m_num_planes <= MAX_CMAC_PLANES);
	assert(m_plane_offsets.size() == m_num_planes);
	if(m_feature_granularities.size() != Feature::NumFeatures)
	{
		cout << "m_feature_granularities.size() " << m_feature_granularities.size() << " NumFeatures " << Feature::NumFeatures << endl;
		assert(false);
	}
	assert(knob::le_cmac2_qvalue_threshold_levels.size() < NUM_MAX_THRESHOLDS-1);
	if(knob::le_cmac2_state_type == 0 && !knob::le_cmac2_active_features.empty())
	{
		cout << "le_cmac2_state_type is set to 0, yet le_cmac2_active_features is populated!" << endl;
		assert(false);
	}
	for(uint32_t index = 0; index < knob::le_cmac2_active_features.size(); ++index)
	{
		if((Feature)knob::le_cmac2_active_features[index] > NumFeatures)
		{
			cout << "le_cmac2_active_features index " << index << " is not valid!" << endl;
			assert(false);
		}
	}

	/* init Q-tables
	 * Unlike CMAC engine 1.0, each Q-table is a two-dimentional array in CMAC 2.0*/
	m_qtables = (float***)calloc(m_num_planes, sizeof(float**));
	assert(m_qtables);
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		m_qtables[plane] = (float**)calloc(m_num_entries_per_plane, sizeof(float*));
		assert(m_qtables[plane]);
		for(uint32_t entry = 0; entry < m_num_entries_per_plane; ++entry)
		{
			m_qtables[plane][entry] = (float*)calloc(m_actions, sizeof(float));
			assert(m_qtables[plane][entry]);
		}
	}

	/* init Q-value */
	if(zero_init)
	{
		m_init_value = 0.0;
	}
	else
	{
		m_init_value = (float)1ul/(1-gamma);

		/* init Q-value buckets */
		m_q_value_buckets.push_back((-1) * 0.50 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((-1) * 0.25 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((-1) * 0.00 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((+1) * 0.25 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((+1) * 0.50 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((+1) * 1.00 * m_init_value * m_num_planes);
		m_q_value_buckets.push_back((+1) * 2.00 * m_init_value * m_num_planes);

		/* init histogram */
		m_q_value_histogram.resize(m_q_value_buckets.size()+1, 0);
	}


	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		for(uint32_t entry = 0; entry < m_num_entries_per_plane; ++entry)
		{
			for(uint32_t action = 0; action < m_actions; ++action)
			{
				m_qtables[plane][entry][action] = m_init_value;
			}
		}
	}

	/* init random generators */
	m_generator.seed(m_seed);
	m_explore = new std::bernoulli_distribution(epsilon);
	m_actiongen = new std::uniform_int_distribution<int>(0, m_actions-1);

	/* reward tracing */
	if(knob::le_cmac2_enable_trace)
	{
		trace_interval = 0;
		trace_timestamp = 0;
		trace = fopen(knob::le_cmac2_trace_file_name.c_str(), "w");
		assert(trace);
	}

	m_early_exploration_window = early_exploration_window;
	m_action_counter = 0;

	/* init stats */
	bzero(&stats, sizeof(stats));
}

LearningEngineCMAC2::~LearningEngineCMAC2()
{
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		delete m_qtables[plane];
	}
}

uint32_t LearningEngineCMAC2::chooseAction(State *state, float &max_to_avg_q_ratio)
{
	stats.action.called++;
	m_action_counter++;

	uint32_t action = 0;
	max_to_avg_q_ratio = (knob::le_cmac2_max_to_avg_q_ratio_type == 1) ? 1.0 : 0.0;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		if(m_action_counter < m_early_exploration_window ||
			(*m_explore)(m_generator))
		{
			/* EXPLORE */
			action = (*m_actiongen)(m_generator);
			stats.action.explore++;
			stats.action.dist[action][0]++;
			MYLOG("action taken %u explore, state %s", action, state->to_string().c_str());
		}
		else
		{
			/* EXPLOIT */
			float max_q = 0.0;
			action = getMaxAction(state, max_q, max_to_avg_q_ratio);
			stats.action.exploit++;
			stats.action.dist[action][1]++;
			gather_stats(max_q, max_to_avg_q_ratio); /* for only stats collection's sake */
			MYLOG("action taken %u exploit, state %s", action, state->to_string().c_str());
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

	return action;
}

uint32_t LearningEngineCMAC2::getMaxAction(State *state, float &max_q, float &max_to_avg_q_ratio)
{
	uint32_t selected_action = 0;
	float q_value = 0.0, total_q_value = 0.0;
	float max_q_value = consultQ(state, 0); /* Major bug fix: init max Q-value */
	float second_max_q_value = max_q_value;
	total_q_value += max_q_value;
	
	for(uint32_t action = 1; action < m_actions; ++action)
	{
		q_value = consultQ(state, action);
		total_q_value += q_value;

		if(q_value > max_q_value)
		{
			second_max_q_value = max_q_value;
			max_q_value = q_value;
			selected_action = action;
		}
		else if(q_value > second_max_q_value && q_value != max_q_value)
		{
			second_max_q_value = q_value;
		}
	}

	float avg_q_value = total_q_value/m_actions;
	switch(knob::le_cmac2_max_to_avg_q_ratio_type)
	{
		case 1:
			max_to_avg_q_ratio = abs(max_q_value)/abs(avg_q_value);
			break;
		case 2:
			if((max_q_value > 0 && avg_q_value > 0) || (max_q_value < 0 && avg_q_value < 0))
			{
				max_to_avg_q_ratio = abs(max_q_value)/abs(avg_q_value) - 1;
			}
			else
			{
				max_to_avg_q_ratio = (max_q_value - avg_q_value)/abs(avg_q_value); 
			}
			break;
		case 3:
			if((max_q_value > 0 && avg_q_value > 0) || (max_q_value < 0 && avg_q_value < 0))
			{
				max_to_avg_q_ratio = abs(max_q_value)/abs(avg_q_value) - 1;
			}
			else
			{
				max_to_avg_q_ratio = (max_q_value - avg_q_value)/abs(avg_q_value); 
			}
			/* Similar to ratio type 2, but a bit more conservative.
			 * Only considers the ratio if the max_q_value is more than the init_value */
			if(max_q_value < knob::le_cmac2_max_q_thresh*m_init_value*m_num_planes)
			{
				max_to_avg_q_ratio = 0.0;
			}
			break;
		case 4:
			/* This is not really MAX to AVG ratio. This is MAX to 2nd MAX ratio */
			max_to_avg_q_ratio = (max_q_value/second_max_q_value);
			break;
		default:
			assert(false);
	}
	
	bool counted = false;
	uint32_t th = 0;
	for(th = 0; th < knob::le_cmac2_qvalue_threshold_levels.size(); ++th)
	{
		if(max_to_avg_q_ratio <= knob::le_cmac2_qvalue_threshold_levels[th])
		{
			stats.action.threshold_dist[selected_action][th]++;
			counted = true;
			break;
		}
	}
	if(!counted)
	{
		stats.action.threshold_dist[selected_action][th]++;
	}

	max_q = max_q_value;
	return selected_action;
}

float LearningEngineCMAC2::consultQ(State *state, uint32_t action)
{
	assert(action < m_actions);
	float q_value = 0.0;
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		q_value += consultPlane(plane, state, action);
	}
	return q_value;
}

float LearningEngineCMAC2::consultPlane(uint32_t plane, State *state, uint32_t action)
{
	assert(plane < m_num_planes);
	assert(action < m_actions);
	uint32_t index = generatePlaneIndex(plane, state, action);
	assert(index < m_num_entries_per_plane);
	return m_qtables[plane][index][action];
}

void LearningEngineCMAC2::updatePlane(uint32_t plane, State *state, uint32_t action, float value)
{
	assert(plane < m_num_planes);
	assert(action < m_actions);
	uint32_t index = generatePlaneIndex(plane, state, action);
	assert(index < m_num_entries_per_plane);
	m_qtables[plane][index][action] = value;
}

uint32_t LearningEngineCMAC2::generatePlaneIndex(uint32_t plane, State *state, uint32_t action)
{
	uint32_t initial_index = generateInitialIndex(plane, state);

	/* Finally hash */
	uint32_t hashed_index = getHash(initial_index);
	// uint32_t hashed_index = HashZoo::getHash(m_hash_type, initial_index);
	return (hashed_index % m_num_entries_per_plane);
}

void LearningEngineCMAC2::learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	stats.learn.called++;

	/* debugging */
	float Qsa1_total = 0.0, Qsa2_total = 0.0, Qsa1_total_new = 0.0;
	Qsa1_total = consultQ(state1, action1);
	Qsa2_total = consultQ(state2, action2);
	
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		/* update each plane */
		for(uint32_t plane = 0; plane < m_num_planes; ++plane)
		{
			float Qsa1, Qsa2, Qsa1_old;
			Qsa1 = consultPlane(plane, state1, action1);
			Qsa2 = consultPlane(plane, state2, action2);
			Qsa1_old = Qsa1;

			/* SARSA */
			Qsa1 = Qsa1 + m_alpha * ((float)reward + m_gamma * Qsa2 - Qsa1);

			/* update back */
			updatePlane(plane, state1, action1, Qsa1);

			MYLOG("<plane_%u> Q(%s,%u) = %.2f, R = %d, Q(%s,%u) = %.2f, Q(%s,%u) = %.2f", plane, state1->to_string().c_str(), action1, Qsa1_old, reward, state2->to_string().c_str(), action2, Qsa2, state1->to_string().c_str(), action1, Qsa1);
		}
		
		Qsa1_total_new = consultQ(state1, action1);
		MYLOG("<overall> Q(%s,%u) = %.2f, R = %d, Q(%s,%u) = %.2f, Q(%s,%u) = %.2f", state1->to_string().c_str(), action1, Qsa1_total, reward, state2->to_string().c_str(), action2, Qsa2_total, state1->to_string().c_str(), action1, Qsa1_total_new);

		/* debugging */
		// if(!state1->to_string().compare("401442|62|1"))
		// {
		// 	printf("Q(%s,%u) = %.2f, R = %d, Q(%s,%u) = %.2f, Q(%s,%u) = %.2f\n", state1->to_string().c_str(), action1, Qsa1_total, reward, state2->to_string().c_str(), action2, Qsa2_total, state1->to_string().c_str(), action1, Qsa1_total_new);
		// }

		if(knob::le_cmac2_enable_trace && !state1->to_string().compare(knob::le_cmac2_trace_state) && trace_interval++ == knob::le_cmac2_trace_interval)
		{
			dump_state_trace(state1);
			trace_interval = 0;
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

void LearningEngineCMAC2::dump_state_trace(State *state)
{
	trace_timestamp++;
	fprintf(trace, "%lu,", trace_timestamp);
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(trace, "%.2f,", consultQ(state, action));
	}
	fprintf(trace, "\n");
	fflush(trace);
}

void LearningEngineCMAC2::dump_stats()
{
	Scooby *scooby = (Scooby*)m_parent;
	fprintf(stdout, "learning_engine_cmac2.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine_cmac2.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine_cmac2.action.exploit %lu\n", stats.action.exploit);
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(stdout, "learning_engine_cmac2.action.index_%d_explored %lu\n", scooby->getAction(action), stats.action.dist[action][0]);
		fprintf(stdout, "learning_engine_cmac2.action.index_%d_exploited %lu\n", scooby->getAction(action), stats.action.dist[action][1]);
	}
	fprintf(stdout, "learning_engine_cmac2.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");
	
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(stdout, "learning_engine_cmac2.action.index_%d_threshold_buckets ", scooby->getAction(action));		
		for(uint32_t th = 0; th < knob::le_cmac2_qvalue_threshold_levels.size()+1; ++th)
		{
			fprintf(stdout, "%lu,", stats.action.threshold_dist[action][th]);			
		}
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");

	/* plot histogram */
	for(uint32_t index = 0; index < m_q_value_histogram.size(); ++index)
	{
		fprintf (stdout, "learning_engine_cmac2.q_value_histogram.bucket_%u %lu\n", index, m_q_value_histogram[index]);
	}
	fprintf(stdout, "\n");

	/* score plotting */
	if(knob::le_cmac2_enable_trace && knob::le_cmac2_enable_score_plot)
	{
		plot_scores();
	}
}

uint32_t LearningEngineCMAC2::getHash(uint32_t key)
{
	switch(m_hash_type)
	{
		case 1: 	return key;
		case 2: 	return HashZoo::jenkins(key);
		case 3: 	return HashZoo::knuth(key);
		case 4: 	return HashZoo::murmur3(key);
		case 5: 	return HashZoo::jenkins32(key);
		case 6: 	return HashZoo::hash32shift(key);
		case 7: 	return HashZoo::hash32shiftmult(key);
		case 8: 	return HashZoo::hash64shift(key);
		case 9: 	return HashZoo::hash5shift(key);
		case 10: 	return HashZoo::hash7shift(key);
		case 11: 	return HashZoo::Wang6shift(key);
		case 12: 	return HashZoo::Wang5shift(key);
		case 13: 	return HashZoo::Wang4shift(key);
		case 14: 	return HashZoo::Wang3shift(key);
		case 15: 	return HashZoo::hybrid1(key);
		default: 	assert(false);
	}
}

void LearningEngineCMAC2::plot_scores()
{
	Scooby *scooby = (Scooby*)m_parent;
	
	char *script_file = (char*)malloc(16*sizeof(char));
	assert(script_file);
	gen_random(script_file, 16);
	FILE *script = fopen(script_file, "w");
	assert(script);

	fprintf(script, "set term png size 960,720 font 'Helvetica,12'\n");
	fprintf(script, "set datafile sep ','\n");
	fprintf(script, "set output '%s'\n", knob::le_cmac2_plot_file_name.c_str());
	fprintf(script, "set title \"Reward over time\"\n");
	fprintf(script, "set xlabel \"Time\"\n");
	fprintf(script, "set ylabel \"Score\"\n");
	fprintf(script, "set grid y\n");
	fprintf(script, "set key right bottom Left box 3\n");
	fprintf(script, "plot ");
	for(uint32_t index = 0; index < knob::le_cmac2_plot_actions.size(); ++index)
	{
		if(index) fprintf(script, ", ");
		fprintf(script, "'%s' using 1:%u with lines title \"action(%d)\"", knob::le_cmac2_trace_file_name.c_str(), (knob::le_cmac2_plot_actions[index]+2), scooby->getAction(knob::le_cmac2_plot_actions[index]));
	}
	fprintf(script, "\n");
	fclose(script);

	std::string cmd = "gnuplot " + std::string(script_file);
	system(cmd.c_str());

	std::string cmd2 = "rm " + std::string(script_file);
	system(cmd2.c_str());
}

uint32_t LearningEngineCMAC2::generateInitialIndex(uint32_t plane, State *state)
{
	switch(knob::le_cmac2_state_type)
	{
		case 0: 	return gen_state_original(plane, state);
		case 1: 	return gen_state_generic(plane, state);
		default:
			cout << "invalid le_cmac2_state_type " << knob::le_cmac2_state_type << endl;
			assert(false);
	}
}

/* Just to maintain backward compatibility */
uint32_t LearningEngineCMAC2::gen_state_original(uint32_t plane, State *state)
{
	/* extract state dimensions */
	uint64_t pc = state->pc;
	uint32_t offset = state->offset;
	int32_t delta = state->delta;

	/* extract features */
	/* 1. PC */
	uint32_t folded_pc = folded_xor(pc, 2); /* 32b folded XOR */
	folded_pc += m_plane_offsets[plane]; /* add CMAC plane offset */
	folded_pc >>= m_feature_granularities[Feature::PC];

	/* 2. Offset */
	offset += m_plane_offsets[plane];
	offset >>= m_feature_granularities[Feature::Offset];

	/* 3. Delta */
	uint32_t unsigned_delta = (delta < 0) ? (((-1) * delta) + (1 << (DELTA_BITS - 1))) : delta; /* converts into 7 bit signed representation */ 
	unsigned_delta += m_plane_offsets[plane];
	unsigned_delta >>= m_feature_granularities[Feature::Delta];

	uint32_t initial_index = (((folded_pc << 4) + offset) << 4) + unsigned_delta;
	return initial_index;
}

void LearningEngineCMAC2::gather_stats(float max_q, float max_to_avg_q_ratio)
{
	float high = 0.0, low = 0.0;
	for(uint32_t index = 0; index < m_q_value_buckets.size(); ++index)
	{
		low = index ? m_q_value_buckets[index-1] : -1000000000;
		high = (index < m_q_value_buckets.size() - 1) ? m_q_value_buckets[index+1] : +1000000000;
		if(max_q >= low && max_q < high)
		{
			m_q_value_histogram[index]++;
			break;
		}
	}
}

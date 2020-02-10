#include <vector>
#include <cassert>
#include <deque>
#include <strings.h>
#include "learning_engine_cmac.h"
#include "util.h"
#include "scooby.h"

#define DELTA_BITS 7

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

}

LearningEngineCMAC::LearningEngineCMAC(CMACConfig config, Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type, bool zero_init)
	: LearningEngineBase(p, alpha, gamma, epsilon, actions, states, seed, policy, type)
{
	m_num_planes = config.num_planes;
	m_num_entries_per_plane = config.num_entries_per_plane;
	m_plane_offsets = config.plane_offsets;
	m_dim_granularities = config.dim_granularities;
	m_action_factors = config.action_factors;
	m_hash_type = config.hash_type;

	/* check integrity */
	assert(m_num_planes <= MAX_CMAC_PLANES);
	assert(m_plane_offsets.size() == m_num_planes);
	assert(m_dim_granularities.size() == Dimension::NumDimensions);
	assert(m_action_factors.size() == m_actions);

	/* init Q-tables
	 * Each Q-table is a one-dimentional array.
	 * It uses a random constant per action to separate learnings for each action */ 
	m_qtables = (float**)calloc(m_num_planes, sizeof(float*));
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		m_qtables[plane] = (float*)calloc(m_num_entries_per_plane, sizeof(float));
	}

	/* init Q-value */
	if(zero_init)
	{
		m_init_value = 0.0;
	}
	else
	{
		m_init_value = (float)1ul/(1-gamma);
	}
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		for(uint32_t entry = 0; entry < m_num_entries_per_plane; ++entry)
		{
			m_qtables[plane][entry] = m_init_value;
		}
	}

	/* init random generators */
	m_generator.seed(m_seed);
	m_explore = new std::bernoulli_distribution(epsilon);
	m_actiongen = new std::uniform_int_distribution<int>(0, m_actions-1);

	/* init stats */
	bzero(&stats, sizeof(stats));
}

LearningEngineCMAC::~LearningEngineCMAC()
{
	for(uint32_t plane = 0; plane < m_num_planes; ++plane)
	{
		delete m_qtables[plane];
	}
}

uint32_t LearningEngineCMAC::chooseAction(State *state)
{
	stats.action.called++;
	uint32_t action = 0;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		if((*m_explore)(m_generator))
		{
			action = (*m_actiongen)(m_generator); // take random action
			stats.action.explore++;
			stats.action.dist[action][0]++;
			MYLOG("action taken %u m_explore, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
		}
		else
		{
			action = getMaxAction(state);
			stats.action.exploit++;
			stats.action.dist[action][1]++;
			MYLOG("action taken %u exploit, state %s, scores %s", action, state->to_string().c_str(), getStringQ(state).c_str());
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

	return action;
}

uint32_t LearningEngineCMAC::getMaxAction(State *state)
{
	float max_q_value = 0.0, q_value = 0.0;
	uint32_t selected_action = 0;

	for(uint32_t action = 0; action < m_actions; ++action)
	{
		q_value = 0.0;
		for(uint32_t plane = 0; plane < m_num_planes; ++plane)
		{
			/* aggregate */
			q_value += consultPlane(plane, state, action);
		}

		/* select max */
		if(q_value >= max_q_value)
		{
			max_q_value = q_value;
			selected_action = action;
		}
	}

	return selected_action;
}

float LearningEngineCMAC::consultPlane(uint32_t plane, State *state, uint32_t action)
{
	assert(plane < m_num_planes);
	assert(action < m_actions);
	uint32_t index = generatePlaneIndex(plane, state, action);
	assert(index < m_num_entries_per_plane);
	return m_qtables[plane][index];
}

void LearningEngineCMAC::updatePlane(uint32_t plane, State *state, uint32_t action, float value)
{
	assert(plane < m_num_planes);
	assert(action < m_actions);
	uint32_t index = generatePlaneIndex(plane, state, action);
	assert(index < m_num_entries_per_plane);
	m_qtables[plane][index] = value;
}

uint32_t LearningEngineCMAC::generatePlaneIndex(uint32_t plane, State *state, uint32_t action)
{
	/* extract state dimensions */
	uint64_t pc = state->pc;
	uint32_t offset = state->offset;
	int32_t delta = state->delta;

	/* extract features */
	/* 1. PC */
	uint32_t folded_pc = folded_xor(pc, 2); /* 32b folded XOR */
	folded_pc += m_plane_offsets[plane]; /* add CMAC plane offset */
	folded_pc >>= m_dim_granularities[Dimension::PC];

	/* 2. Offset */
	offset += m_plane_offsets[plane];
	offset >>= m_dim_granularities[Dimension::Offset];

	/* 3. Delta */
	uint32_t unsigned_delta = (delta < 0) ? (((-1) * delta) + (1 << (DELTA_BITS - 1))) : delta; /* converts into 7 bit signed representation */ 
	unsigned_delta += m_plane_offsets[plane];
	unsigned_delta >>= m_dim_granularities[Dimension::Delta];

	/* Concatenate all features */
	uint32_t initial_index = (((folded_pc << 4) + offset) << 4) + unsigned_delta;

	/* XOR the constant factor for action */
	initial_index = initial_index ^ m_action_factors[action];

	/* Finally hash */
	uint32_t hashed_index = getHash(initial_index);
	return (hashed_index % m_num_entries_per_plane);
}

void LearningEngineCMAC::learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	stats.learn.called++;
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

			MYLOG("Q(%s,%u) = %.2f, R = %d, Q(%s,%u) = %.2f, Q(%s,%u) = %.2f", state1->to_string(), action1, Qsa1_old, reward, state2->to_string(), action2, Qsa2, state1->to_string(), action1, Qsa1);
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

void LearningEngineCMAC::dump_stats()
{
	fprintf(stdout, "learning_engine_cmac.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine_cmac.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine_cmac.action.exploit %lu\n", stats.action.exploit);
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		Scooby *scooby = (Scooby*)m_parent;
		fprintf(stdout, "learning_engine_cmac.action.index_%d_explored %lu\n", scooby->getAction(action), stats.action.dist[action][0]);
		fprintf(stdout, "learning_engine_cmac.action.index_%d_exploited %lu\n", scooby->getAction(action), stats.action.dist[action][1]);
	}
	fprintf(stdout, "learning_engine_cmac.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");
}

uint32_t LearningEngineCMAC::getHash(uint32_t key)
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
#include <assert.h>
#include "feature_knowledge.h"
#include "feature_knowledge_helper.h"

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

FeatureKnowledge::FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, uint32_t states, bool zero_init, uint32_t hash_type)
	: m_feature_type(feature_type), m_alpha(alpha), m_gamma(gamma), m_actions(actions), m_states(states), m_weight(1.00), m_hash_type(hash_type)
{
	/* create Q-table */
	m_qtable = (float**)calloc(m_states, sizeof(float*));
	assert(m_qtable);
	for(uint32_t state = 0; state < m_states; ++state)
	{
		m_qtable[state] = (float*)calloc(m_actions, sizeof(float));
		assert(m_qtable[state]);
	}

	/* init Q-table */
	if(zero_init)
	{
		m_init_value = 0;
	}
	else
	{
		m_init_value = (float)1ul/(1-gamma);
	}
	for(uint32_t state = 0; state < m_states; ++state)
	{
		for(uint32_t action = 0; action < m_actions; ++action)
		{
			m_qtable[state][action] = m_init_value;
		}
	}
}

FeatureKnowledge::~FeatureKnowledge()
{
	for(uint32_t row = 0; row < m_states; ++row)
	{
		free(m_qtable[row]);
	}
	free(m_qtable);
}

float FeatureKnowledge::getQ(uint32_t state_index, uint32_t action)
{
	assert(state_index < m_states);
	assert(action < m_actions);
	return m_qtable[state_index][action];
}

void FeatureKnowledge::setQ(uint32_t state_index, uint32_t action, float value)
{
	assert(state_index < m_states);
	assert(action < m_actions);
	m_qtable[state_index][action] = value;	
}

float FeatureKnowledge::retrieveQ(State *state, uint32_t action)
{
	uint32_t state_index = get_state_index(state);
	return m_weight * getQ(state_index, action);
}

void FeatureKnowledge::updateQ(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	uint32_t state_index1 = get_state_index(state1);
	uint32_t state_index2 = get_state_index(state2);
	float Qsa1, Qsa2, Qsa1_old;
	
	Qsa1 = getQ(state_index1, action1);
	Qsa2 = getQ(state_index2, action2);
	Qsa1_old = Qsa1;	
	/* SARSA */
	Qsa1 = Qsa1 + m_alpha * ((float)reward + m_gamma * Qsa2 - Qsa1);
	setQ(state_index1, action1, Qsa1);

	MYLOG("Q(%x,%u) = %.2f, R = %d, Q(%x,%u) = %.2f, Q(%x,%u) = %.2f", state1, action1, Qsa1_old, reward, state2, action2, Qsa2, state1, action1, Qsa1);
	MYLOG("state %x, scores %s", state1, getStringQ(state1).c_str());
}

uint32_t FeatureKnowledge::get_state_index(State *state)
{
	uint32_t raw_index = 0;
	uint64_t pc = state->pc;
	uint64_t page = state->page;
	uint64_t address = state->address;
	uint32_t offset = state->offset;
	int32_t  delta = state->delta;
	uint32_t delta_path = state->local_delta_sig2;
	uint32_t pc_path = state->local_pc_sig;
	uint32_t offset_path = state->local_offset_sig;

	switch(m_feature_type)
	{
		case F_PC:				return process_PC(pc);
		case F_Offset:			return process_offset(offset);
		case F_Delta:			return process_delta(delta);
		case F_Address:			return process_address(address);
		case F_PC_Offset:		return process_PC_offset(pc, offset);
		case F_PC_Address:		return process_PC_address(pc, address);
		case F_PC_Page:			return process_PC_page(pc, page);
		case F_PC_Path:			return process_PC_path(pc_path);
		case F_Delta_Path:		return process_delta_path(delta_path);
		case F_Offset_Path:		return process_offset_path(offset_path);
	}
}
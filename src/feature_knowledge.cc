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

const char* MapFeatureTypeString[] = {"PC", "Offset", "Delta", "Address", "PC_Offset", "PC_Address", "PC_Page", "PC_Path", "Delta_Path", "Offset_Path"};

string FeatureKnowledge::getFeatureString(FeatureType feature)
{
	assert(feature < FeatureType::NumFeatureTypes);
	return MapFeatureTypeString[(uint32_t)feature];
}

FeatureKnowledge::FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, uint32_t num_tilings, uint32_t num_tiles, bool zero_init, uint32_t hash_type, int32_t enable_tiling_offset)
	: m_feature_type(feature_type), m_alpha(alpha), m_gamma(gamma), m_actions(actions), m_num_tilings(num_tilings), m_num_tiles(num_tiles), m_weight(1.00), m_hash_type(hash_type), m_enable_tiling_offset(enable_tiling_offset ? true : false)
{
	assert(m_num_tilings < FK_MAX_TILINGS);
	assert(m_num_tilings == 1 || m_enable_tiling_offset); /* enforce the use of tiling offsets in case of multiple tilings */

	/* create Q-table */
	m_qtable = (float***)calloc(m_num_tilings, sizeof(float**));
	assert(m_qtable);
	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		m_qtable[tiling] = (float**)calloc(m_num_tiles, sizeof(float*));
		assert(m_qtable[tiling]);
		for(uint32_t tile = 0; tile < m_num_tiles; ++tile)
		{
			m_qtable[tiling][tile] = (float*)calloc(m_actions, sizeof(float));
			assert(m_qtable[tiling][tile]);
		}
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
	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		for(uint32_t tile = 0; tile < m_num_tiles; ++tile)
		{
			for(uint32_t action = 0; action < m_actions; ++action)
			{
				m_qtable[tiling][tile][action] = m_init_value;
			}
		}
	}
}

FeatureKnowledge::~FeatureKnowledge()
{

}

float FeatureKnowledge::getQ(uint32_t tiling, uint32_t tile_index, uint32_t action)
{
	assert(tiling < m_num_tilings);
	assert(tile_index < m_num_tiles);
	assert(action < m_actions);
	return m_qtable[tiling][tile_index][action];
}

void FeatureKnowledge::setQ(uint32_t tiling, uint32_t tile_index, uint32_t action, float value)
{
	assert(tiling < m_num_tilings);
	assert(tile_index < m_num_tiles);
	assert(action < m_actions);
	m_qtable[tiling][tile_index][action] = value;	
}

float FeatureKnowledge::retrieveQ(State *state, uint32_t action)
{
	uint32_t tile_index = 0;
	float q_value = 0.0;

	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		tile_index = get_tile_index(tiling, state);
		q_value += getQ(tiling, tile_index, action);
	}

	return m_weight * q_value;
}

void FeatureKnowledge::updateQ(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2)
{
	uint32_t tile_index1 = 0, tile_index2 = 0;
	float Qsa1, Qsa2, Qsa1_old;

	for(uint32_t tiling = 0; tiling < m_num_tilings; ++tiling)
	{
		tile_index1 = get_tile_index(tiling, state1);
		tile_index2 = get_tile_index(tiling, state2);
		Qsa1 = getQ(tiling, tile_index1, action1);
		Qsa2 = getQ(tiling, tile_index2, action2);
		Qsa1_old = Qsa1;	
		/* SARSA */
		Qsa1 = Qsa1 + m_alpha * ((float)reward + m_gamma * Qsa2 - Qsa1);
		setQ(tiling, tile_index1, action1, Qsa1);
		MYLOG("<tiling %u> Q(%x,%u) = %.2f, R = %d, Q(%x,%u) = %.2f, Q(%x,%u) = %.2f", tiling,  state1, action1, Qsa1_old, reward, state2, action2, Qsa2, state1, action1, Qsa1);
	}
	MYLOG("state %x, scores %s", state1, getStringQ(state1).c_str());
}

uint32_t FeatureKnowledge::get_tile_index(uint32_t tiling, State *state)
{
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
		case F_PC:				return process_PC(tiling, pc);
		case F_Offset:			return process_offset(tiling, offset);
		case F_Delta:			return process_delta(tiling, delta);
		case F_Address:			return process_address(tiling, address);
		case F_PC_Offset:		return process_PC_offset(tiling, pc, offset);
		case F_PC_Address:		return process_PC_address(tiling, pc, address);
		case F_PC_Page:			return process_PC_page(tiling, pc, page);
		case F_PC_Path:			return process_PC_path(tiling, pc_path);
		case F_Delta_Path:		return process_delta_path(tiling, delta_path);
		case F_Offset_Path:		return process_offset_path(tiling, offset_path);
	}
}
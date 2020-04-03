#ifndef FEATURE_KNOWLEDGE
#define FEATURE_KNOWLEDGE

#include "scooby_helper.h"

typedef enum
{
	F_PC = 0,		// 0
	F_Offset,		// 1
	F_Delta,		// 2
	F_Address,		// 3
	F_PC_Offset,	// 4
	F_PC_Address,	// 5
	F_PC_Page,		// 6
	F_PC_Path,		// 7
	F_Delta_Path,	// 8
	F_Offset_Path,	// 9

	NumFeatureTypes
} FeatureType;

class FeatureKnowledge
{
private:
	FeatureType m_feature_type;
	float **m_qtable;
	float m_alpha, m_gamma;
	uint32_t m_actions, m_states;
	bool m_init_value;
	float m_weight;
	uint32_t m_hash_type;

private:
	float getQ(uint32_t state_index, uint32_t action);
	void setQ(uint32_t state_index, uint32_t action, float value);
	uint32_t get_state_index(State *state);

	/* feature index generators */
	uint32_t process_PC(uint64_t pc);
	uint32_t process_offset(uint32_t offset);
	uint32_t process_delta(int32_t delta);
	uint32_t process_address(uint64_t address);
	uint32_t process_PC_offset(uint64_t pc, uint32_t offset);
	uint32_t process_PC_address(uint64_t pc, uint64_t address);
	uint32_t process_PC_page(uint64_t pc, uint64_t page);
	uint32_t process_PC_path(uint32_t pc_path);
	uint32_t process_delta_path(uint32_t delta_path);
	uint32_t process_offset_path(uint32_t offset_path);

public:
	FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, uint32_t states, bool zero_init, uint32_t hash_type);
	~FeatureKnowledge();
	float retrieveQ(State *state, uint32_t action_index);
	void updateQ(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
};

#endif /* FEATURE_KNOWLEDGE */


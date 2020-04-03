#ifndef FEATURE_KNOWLEDGE
#define FEATURE_KNOWLEDGE

#include "scooby_helper.h"
#define FK_MAX_TILINGS 32

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
	float m_alpha, m_gamma;
	uint32_t m_actions;
	bool m_init_value;
	float m_weight;
	uint32_t m_hash_type;
	
	uint32_t m_num_tilings, m_num_tiles;
	float ***m_qtable;
	bool m_enable_tiling_offset;

private:
	float getQ(uint32_t tiling, uint32_t tile_index, uint32_t action);
	void setQ(uint32_t tiling, uint32_t tile_index, uint32_t action, float value);
	uint32_t get_tile_index(uint32_t tiling, State *state);
	
	/* feature index generators */
	uint32_t process_PC(uint32_t tiling, uint64_t pc);
	uint32_t process_offset(uint32_t tiling, uint32_t offset);
	uint32_t process_delta(uint32_t tiling, int32_t delta);
	uint32_t process_address(uint32_t tiling, uint64_t address);
	uint32_t process_PC_offset(uint32_t tiling, uint64_t pc, uint32_t offset);
	uint32_t process_PC_address(uint32_t tiling, uint64_t pc, uint64_t address);
	uint32_t process_PC_page(uint32_t tiling, uint64_t pc, uint64_t page);
	uint32_t process_PC_path(uint32_t tiling, uint32_t pc_path);
	uint32_t process_delta_path(uint32_t tiling, uint32_t delta_path);
	uint32_t process_offset_path(uint32_t tiling, uint32_t offset_path);

public:
	FeatureKnowledge(FeatureType feature_type, float alpha, float gamma, uint32_t actions, uint32_t num_tilings, uint32_t num_tiles, bool zero_init, uint32_t hash_type, int32_t enable_tiling_offset);
	~FeatureKnowledge();
	float retrieveQ(State *state, uint32_t action_index);
	void updateQ(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
	static string getFeatureString(FeatureType type);
};

#endif /* FEATURE_KNOWLEDGE */

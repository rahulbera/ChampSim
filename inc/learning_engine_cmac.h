#ifndef LEARNING_ENGINE_CMAC_H
#define LEARNING_ENGINE_CMAC_H

#include <random>
#include "learning_engine_base.h"
#include "scooby_helper.h"

#define MAX_CMAC_PLANES 16

typedef enum
{
	InvalDim = 0,
	PC,
	Offset,
	Delta,

	NumDimensions
} Dimension;

class CMACConfig
{
public:
	uint32_t num_planes;
	uint32_t num_entries_per_plane;
	vector<int32_t> plane_offsets;
	vector<int32_t> dim_granularities;
	vector<int32_t> action_factors;
	uint32_t hash_type;

	CMACConfig()
	{
		num_planes = 0;
		num_entries_per_plane = 0;
		plane_offsets.clear();
		dim_granularities.clear();
		action_factors.clear();
		hash_type = 0;
	}
	~CMACConfig();
};

class LearningEngineCMAC : public LearningEngineBase
{
private:
	uint32_t m_num_planes;
	uint32_t m_num_entries_per_plane;
	vector<int32_t> m_plane_offsets;
	vector<int32_t> m_dim_granularities;
	vector<int32_t> m_action_factors;
	uint32_t m_hash_type;

	float m_init_value;

    std::default_random_engine m_generator;
    std::bernoulli_distribution *m_explore;
    std::uniform_int_distribution<int> *m_actiongen;

	float **m_qtables;

	vector<uint32_t> plane_offset, dimension_granularity, action_factor;

	/* stats */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t explore;
			uint64_t exploit;
			uint64_t dist[MAX_ACTIONS][2]; /* 0:explored, 1:exploited */
		} action;

		struct
		{
			uint64_t called;
		} learn;
	} stats;

private:
	uint32_t getMaxAction(State *state);
	float consultPlane(uint32_t plane, State *state, uint32_t action);
	void updatePlane(uint32_t plane, State *state, uint32_t action, float value);
	uint32_t generatePlaneIndex(uint32_t plane, State *state, uint32_t action);
	uint32_t getHash(uint32_t key);

public:
	LearningEngineCMAC(CMACConfig config, Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type, bool zero_init);
	~LearningEngineCMAC();

	uint32_t chooseAction(State *state);
	void learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
	void dump_stats();
};

#endif /* LEARNING_ENGINE_CMAC_H */


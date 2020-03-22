#ifndef LEARNING_ENGINE_CMAC2_H
#define LEARNING_ENGINE_CMAC2_H

#include "learning_engine_cmac.h"

#define NUM_MAX_THRESHOLDS 16

class LearningEngineCMAC2 : public LearningEngineBase
{
private:
	uint32_t m_num_planes;
	uint32_t m_num_entries_per_plane;
	vector<int32_t> m_plane_offsets;
	vector<int32_t> m_feature_granularities;
	vector<int32_t> m_action_factors;
	uint32_t m_hash_type;

	float m_init_value;
	vector<float> m_q_value_buckets;

    std::default_random_engine m_generator;
    std::bernoulli_distribution *m_explore;
    std::uniform_int_distribution<int> *m_actiongen;

	float ***m_qtables;

	/* tracing related knobs */
	uint32_t trace_interval;
	uint64_t trace_timestamp;
	FILE *trace;

	uint64_t m_action_counter;
	uint64_t m_early_exploration_window;

	/* stats */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t explore;
			uint64_t exploit;
			uint64_t dist[MAX_ACTIONS][2]; /* 0:explored, 1:exploited */
			uint64_t threshold_dist[MAX_ACTIONS][NUM_MAX_THRESHOLDS];
		} action;

		struct
		{
			uint64_t called;
		} learn;

	} stats;

	vector<uint64_t> m_q_value_histogram;

private:
	uint32_t getMaxAction(State *state, float &max_q, float &max_to_avg_q_ratio);
	float consultQ(State *state, uint32_t action);
	float consultPlane(uint32_t plane, State *state, uint32_t action);
	void updatePlane(uint32_t plane, State *state, uint32_t action, float value);
	uint32_t generatePlaneIndex(uint32_t plane, State *state, uint32_t action);
	uint32_t getHash(uint32_t key);
	void dump_state_trace(State *state);
	void plot_scores();
	void gather_stats(float max_q, float max_to_avg_q_ratio);

	uint32_t generateInitialIndex(uint32_t plane, State *state);
	uint32_t gen_state_original(uint32_t plane, State *state);
	uint32_t gen_state_generic(uint32_t plane, State *state);
	uint32_t get_processed_feature(Feature feature, State *state, uint32_t plane);

	uint32_t process_PC(uint64_t pc, uint32_t plane);
	uint32_t process_offset(uint32_t offset, uint32_t plane);
	uint32_t process_delta(int32_t delta, uint32_t plane);
	uint32_t process_PC_path(uint32_t pc_path, uint32_t plane);
	uint32_t process_offset_path(uint32_t offset, uint32_t plane);
	uint32_t process_delta_path(uint32_t delta, uint32_t plane);
	uint32_t process_address(uint64_t address, uint32_t plane);
	uint32_t process_page(uint64_t page, uint32_t plane);

public:
	LearningEngineCMAC2(CMACConfig config, Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type, bool zero_init, uint64_t early_exploration_window);
	~LearningEngineCMAC2();

	uint32_t chooseAction(State *state, float &max_to_avg_q_ratio);
	void learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
	void dump_stats();
};

#endif /* LEARNING_ENGINE_CMAC_H */


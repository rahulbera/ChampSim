#ifndef LEARNING_ENGINE_FEATUREWISE_H
#define LEARNING_ENGINE_FEATUREWISE_H

#include <random>
#include "learning_engine_base.h"
#include "feature_knowledge.h"

class LearningEngineFeaturewise : public LearningEngineBase
{
private:
	FeatureKnowledge* m_feature_knowledges[NumFeatureTypes];
	float m_max_q_value;
	
	std::default_random_engine m_generator;
	std::bernoulli_distribution *m_explore;
	std::uniform_int_distribution<int> *m_actiongen;

	vector<float> m_q_value_buckets;
	vector<uint64_t> m_q_value_histogram;

	/* stats */
	struct
	{
		struct
		{
			uint64_t called;
			uint64_t explore;
			uint64_t exploit;
			uint64_t dist[MAX_ACTIONS][2]; /* 0:explored, 1:exploited */
			uint64_t fallback;
		} action;

		struct
		{
			uint64_t called;
		} learn;

		struct
		{
			uint64_t total;
			uint64_t feature_align_dist[NumFeatureTypes];
			uint64_t feature_align_all;
		} consensus;

	} stats;

private:
	void init_knobs();
	void init_stats();
	uint32_t getMaxAction(State *state, float &max_q, float &max_to_avg_q_ratio);
	float consultQ(State *state, uint32_t action);
	void gather_stats(float max_q, float max_to_avg_q_ratio);
	void action_selection_consensus(State *state, uint32_t selected_action);

public:
	LearningEngineFeaturewise(Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint64_t seed, std::string policy, std::string type, bool zero_init);
	~LearningEngineFeaturewise();
	uint32_t chooseAction(State *state, float &max_to_avg_q_ratio);
	void learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
	void dump_stats();
};

#endif /* LEARNING_ENGINE_FEATUREWISE_H */


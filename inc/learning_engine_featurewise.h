#ifndef LEARNING_ENGINE_FEATUREWISE_H
#define LEARNING_ENGINE_FEATUREWISE_H

#include <random>
#include "learning_engine_base.h"
#include "feature_knowledge.h"

class LearningEngineFeaturewise : public LearningEngineBase
{
private:
	FeatureKnowledge* m_feature_knowledges[NumFeatureTypes];
	
	std::default_random_engine m_generator;
	std::bernoulli_distribution *m_explore;
	std::uniform_int_distribution<int> *m_actiongen;

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
	void init_knobs();
	void init_stats();
	uint32_t getMaxAction(State *state);
	float consultQ(State *state, uint32_t action);

public:
	LearningEngineFeaturewise(Prefetcher *p, float alpha, float gamma, float epsilon, uint32_t actions, uint64_t seed, std::string policy, std::string type, bool zero_init);
	~LearningEngineFeaturewise();
	uint32_t chooseAction(State *state);
	void learn(State *state1, uint32_t action1, int32_t reward, State *state2, uint32_t action2);
	void dump_stats();
};

#endif /* LEARNING_ENGINE_FEATUREWISE_H */


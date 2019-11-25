#ifndef LEARNING_ENGINE
#define LEARNING_ENGINE

#include <random>
#include <string.h>

/*
 * table format
 *      |action 0| action 1| action 2|...| action n
state 0 |
state 1 |
		|	  ____         _        _     _      
		|	 / __ \       | |      | |   | |     
		|	| |  | |______| |_ __ _| |__ | | ___ 
		|	| |  | |______| __/ _` | '_ \| |/ _ \
		|	| |__| |      | || (_| | |_) | |  __/
		|	 \___\_\       \__\__,_|_.__/|_|\___|
		|                             
state m |
*/

enum Policy
{
	EGreedy = 0,
	NumPolicies
};

enum LearningType
{
	QLearning = 0,
	SARSA,
	NumLearningTypes
};

const char* MapPolicyString(Policy policy);
const char* MapLearningTypeString(LearningType type);

class LearningEngine
{
private:
	float m_alpha;
	float m_gamma;
	float m_epsilon;
	uint32_t m_actions;
	uint32_t m_states;
	uint64_t m_seed;
	Policy m_policy;
	LearningType m_type;

    std::default_random_engine generator;
    std::bernoulli_distribution *explore;
    std::uniform_int_distribution<int> *actiongen;

	float **qtable;

	LearningType parseLearningType(std::string str);
	Policy parsePolicy(std::string str);
	float consultQ(uint32_t state, uint32_t action);
	void updateQ(uint32_t state, uint32_t action, float value);
	uint32_t getMaxAction(uint32_t state);

public:
	LearningEngine(float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type);
	~LearningEngine();

	inline void setAlpha(float alpha){m_alpha = alpha;}
	inline float getAlpha(){return m_alpha;}
	inline void setGamma(float gamma){m_gamma = gamma;}
	inline float getGamma(){return m_gamma;}
	inline void setEpsilon(float epsilon){m_epsilon = epsilon;}
	inline float getEpsilon(){return m_epsilon;}
	inline void setStates(uint32_t states){m_states = states;}
	inline uint32_t getStates(){return m_states;}
	inline void setActions(uint32_t actions){m_actions = actions;}
	inline uint32_t getActions(){return m_actions;}

	uint32_t chooseAction(uint32_t state);
	void learn(uint32_t state1, uint32_t action1, uint32_t reward, uint32_t state2, uint32_t action2);
};

#endif /* LEARNING_ENGINE */


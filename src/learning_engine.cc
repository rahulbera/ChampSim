#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <strings.h>
#include "learning_engine.h"

#define TRACE_INTERVAL 5

const char* PolicyString[] = {"EGreddy"};
const char* MapPolicyString(Policy policy)
{
	assert((uint32_t)policy < Policy::NumPolicies);
	return PolicyString[(uint32_t)policy];
}

const char* LearningTypeString[] = {"QLearning", "SARSA"};
const char* MapLearningTypeString(LearningType type)
{
	assert((uint32_t)type < LearningType::NumLearningTypes);
	return LearningTypeString[(uint32_t)type];
}


LearningEngine::LearningEngine(float alpha, float gamma, float epsilon, uint32_t actions, uint32_t states, uint64_t seed, std::string policy, std::string type, bool zero_init)
	: m_alpha(alpha)
	, m_gamma(gamma)
	, m_epsilon(epsilon) // make it small, as true value indicates exploration
	, m_actions(actions)
	, m_states(states)
	, m_seed(seed)
	, m_policy(parsePolicy(policy))
	, m_type(parseLearningType(type))
{
	qtable = (float**)calloc(m_states, sizeof(float*));
	assert(qtable);
	for(uint32_t index = 0; index < m_states; ++index)
	{
		qtable[index] = (float*)calloc(m_actions, sizeof(float));
		assert(qtable);
	}

	/* init Q-table */
	if(zero_init)
	{
		init_value = 0;
	}
	else
	{
		init_value = (float)1ul/(1-gamma);
	}
	for(uint32_t row = 0; row < m_states; ++row)
	{
		for(uint32_t col = 0; col < m_actions; ++col)
		{
			qtable[row][col] = init_value;
		}
	}

	generator.seed(m_seed);
	explore = new std::bernoulli_distribution(epsilon);
	actiongen = new std::uniform_int_distribution<int>(0, m_actions-1);

#ifdef LE_TRACE
	trace_interval = 0;
	trace = fopen("trace.let", "w");
	assert(trace);
#endif

	bzero(&stats, sizeof(stats));
}

LearningEngine::~LearningEngine()
{
	for(uint32_t row = 0; row < m_states; ++row)
	{
		free(qtable[row]);
	}
	free(qtable);
#ifdef LE_TRACE
	fclose(trace);
#endif
}

LearningType LearningEngine::parseLearningType(std::string str)
{
	if(!str.compare("QLearning"))	return LearningType::QLearning;
	if(!str.compare("SARSA"))		return LearningType::SARSA;

	printf("unsupported learning_type %s\n", str.c_str());
	assert(false);
}

Policy LearningEngine::parsePolicy(std::string str)
{
	if(!str.compare("EGreedy"))		return Policy::EGreedy;

	printf("unsupported policy %s\n", str.c_str());
	assert(false);
}

uint32_t LearningEngine::chooseAction(uint32_t state)
{
	stats.action.called++;
	assert(state < m_states);
	uint32_t action = 0;
	bool do_explore = false;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{
		if((*explore)(generator))
		{
			action = (*actiongen)(generator); // take random action
			stats.action.explore++;
			stats.action.dist[action][0]++;
			do_explore = true;
		}
		else
		{
			action = getMaxAction(state);
			stats.action.exploit++;
			stats.action.dist[action][1]++;
		}
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}

#ifdef LE_DEBUG
	if(action == 2) /* selects -1 */
	{
		printf("[state: %x, action: %u] ", state, action);
		for(uint32_t index = 0; index < m_actions; ++index)
		{
			printf("%8.3f,", qtable[state][index]);
		}
		if(do_explore)
			printf("EXPLORE\n");
		else
			printf("EXPLOIT\n");
	}
#endif

	return action;
}

void LearningEngine::learn(uint32_t state1, uint32_t action1, uint32_t reward, uint32_t state2, uint32_t action2)
{
	stats.learn.called++;
	if(m_type == LearningType::SARSA && m_policy == Policy::EGreedy)
	{	
		float Qsa1, Qsa2;
		Qsa1 = consultQ(state1, action1);
		Qsa2 = consultQ(state2, action2);
		/* SARSA */
		Qsa1 = Qsa1 + m_alpha * (reward + m_gamma * Qsa2 - Qsa1);
		updateQ(state1, action1, Qsa1);

#ifdef LE_TRACE
		if(state1 == 0x401442 && trace_interval++ == TRACE_INTERVAL) /* some libquantum PC */
		{
			dump_state_trace(state1);
			trace_interval = 0;
		}
#endif
	}
	else
	{
		printf("learning_type %s policy %s not supported!\n", MapLearningTypeString(m_type), MapPolicyString(m_policy));
		assert(false);
	}
}

float LearningEngine::consultQ(uint32_t state, uint32_t action)
{
	assert(state < m_states && action < m_actions);
	float value = qtable[state][action];
	return value;
}

void LearningEngine::updateQ(uint32_t state, uint32_t action, float value)
{
	assert(state < m_states && action < m_actions);
	qtable[state][action] = value;	
}

uint32_t LearningEngine::getMaxAction(uint32_t state)
{
	assert(state < m_states);
	float max = qtable[state][0];
	uint32_t action = 0;
	for(uint32_t index = 1; index < m_actions; ++index)
	{
		if(qtable[state][index] > max)
		{
			max = qtable[state][index];
			action = index;
		}
	}
	return action;
}

void LearningEngine::print_aux_stats()
{
	/* compute state-action table usage
	 * how mane state entries are actually used?
	 * heat-map dump, etc. etc. */
	uint32_t state_used = 0;
	for(uint32_t state = 0; state < m_states; ++state)
	{
		for(uint32_t action = 0; action < m_actions; ++action)
		{
			if(qtable[state][action] != init_value)
			{
				state_used++;
				break;
			}
		}
	}

	fprintf(stdout, "learning_engine.state_used %u\n", state_used);
	fprintf(stdout, "\n");
}

void LearningEngine::dump_stats()
{
	fprintf(stdout, "learning_engine.action.called %lu\n", stats.action.called);
	fprintf(stdout, "learning_engine.action.explore %lu\n", stats.action.explore);
	fprintf(stdout, "learning_engine.action.exploit %lu\n", stats.action.exploit);
	for(uint32_t action = 0; action < m_actions; ++action)
	{
		fprintf(stdout, "learning_engine.action.index_%u_explored %lu\n", action, stats.action.dist[action][0]);
		fprintf(stdout, "learning_engine.action.index_%u_exploited %lu\n", action, stats.action.dist[action][1]);
	}
	fprintf(stdout, "learning_engine.learn.called %lu\n", stats.learn.called);
	fprintf(stdout, "\n");

	print_aux_stats();
}

void LearningEngine::dump_state_trace(uint32_t state)
{
#ifdef LE_TRACE
	fprintf(trace, "%x,", state);
	for(uint32_t index = 0; index < m_actions; ++index)
	{
		fprintf(trace, "%.2f,", qtable[state][index]);
	}
	fprintf(trace, "\n");
#endif
}
#include <iostream>
#include <string>
#include <string.h>
#include <math.h>
#include "knobs.h"
#include "ini.h"
using namespace std;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

namespace knob
{
	uint64_t warmup_instructions = 1000000;
	uint64_t simulation_instructions = 1000000;
	bool  	 knob_cloudsuite = false;
	bool     knob_low_bandwidth = false;
	string 	 l2c_prefetcher_type;

	uint32_t sms_at_size = 32;
	uint32_t sms_ft_size = 64;
	uint32_t sms_pht_size = 16384;
	uint32_t sms_pref_degree = 4;
	uint32_t sms_region_size = 2048;
	uint32_t sms_region_size_log = 11;
	bool     sms_enable_pref_buffer = true;
	uint32_t sms_pref_buffer_size = 256;

	uint32_t spp_st_size = 256;
	uint32_t spp_pt_size = 512;
	uint32_t spp_max_outcomes = 4;
	double   spp_max_confidence = 25.0;
	uint32_t spp_max_depth = 64;
	uint32_t spp_max_prefetch_per_level = 1;
	uint32_t spp_max_confidence_counter_value = 16;
	uint32_t spp_max_global_counter_value = 1024;
	uint32_t spp_pf_size = 1024;
	bool     spp_enable_alpha = true;
	bool     spp_enable_pref_buffer = true;
	uint32_t spp_pref_buffer_size = 256;
	uint32_t spp_pref_degree = 4;
	bool     spp_enable_ghr = true;
	uint32_t spp_ghr_size = 8;
	uint32_t spp_signature_bits = 12;
	uint32_t spp_alpha_epoch = 1024;

	/* Scooby */
	float    scooby_alpha;
	float    scooby_gamma;
	float    scooby_epsilon;
	uint32_t scooby_max_states;
	uint32_t scooby_seed;
	string   scooby_policy;
	string   scooby_learning_type;
	vector<int32_t> scooby_actions;
	uint32_t scooby_max_actions;
	uint32_t scooby_pt_size;
	uint32_t scooby_st_size;
	int32_t  scooby_reward_none;
	int32_t  scooby_reward_incorrect;
	int32_t  scooby_reward_correct_untimely;
	int32_t  scooby_reward_correct_timely;
	uint32_t scooby_max_pcs;
	uint32_t scooby_max_offsets;
	uint32_t scooby_max_deltas;
}

char config_file_name[MAX_LEN];

void parse_args(int argc, char *argv[])
{
	for(int index = 0; index < argc; ++index)
	{
		string arg = string(argv[index]);
		if(arg.compare(0, 2, "--") == 0)
		{
			arg = arg.substr(2);
		}
		if(ini_parse_string(arg.c_str(), handler, NULL) < 0)
		{
			printf("error parsing commandline %s\n", argv[index]);
			exit(1);
		}
	}	
}

int handler(void* user, const char* section, const char* name, const char* value)
{
	if(MATCH("", "config"))
	{
		strcpy(config_file_name, value);
		parse_config(config_file_name);
	}
	else
	{
		parse_knobs(user, section, name, value);
	}
	return 1;
}

void parse_config(char *config_file_name)
{
	cout << "parsing config file: " << string(config_file_name) << endl;
	if (ini_parse(config_file_name, parse_knobs, NULL) < 0) 
	{
        printf("Failed to load %s\n", config_file_name);
        exit(1);
    }
}

int parse_knobs(void* user, const char* section, const char* name, const char* value)
{
    if (MATCH("", "warmup_instructions"))
    {
		knob::warmup_instructions = atol(value);
    }
    else if (MATCH("", "simulation_instructions"))
    {
		knob::simulation_instructions = atol(value);
    }
    else if (MATCH("", "knob_cloudsuite"))
    {
		knob::knob_cloudsuite = atoi(value);
    }
    else if (MATCH("", "knob_low_bandwidth"))
    {
		knob::knob_low_bandwidth = atoi(value);
    }
    else if (MATCH("", "l2c_prefetcher_type"))
    {
		knob::l2c_prefetcher_type = string(value);
    }

    /* SMS */
	else if(MATCH("", "sms_at_size"))
	{
		knob::sms_at_size = atoi(value);
	}
	else if(MATCH("", "sms_ft_size"))
	{
		knob::sms_ft_size = atoi(value);
	}
	else if(MATCH("", "sms_pht_size"))
	{
		knob::sms_pht_size = atoi(value);
	}
	else if(MATCH("", "sms_pref_degree"))
	{
		knob::sms_pref_degree = atoi(value);
	}
	else if(MATCH("", "sms_region_size"))
	{
		knob::sms_region_size = atoi(value);
		knob::sms_region_size_log = log2(knob::sms_region_size);
	}
	else if(MATCH("", "sms_enable_pref_buffer"))
	{
		knob::sms_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if(MATCH("", "sms_pref_buffer_size"))
	{
		knob::sms_pref_buffer_size = atoi(value);
	}

	/* SPP */
	else if (MATCH("", "spp_st_size"))
	{
		knob::spp_st_size = atoi(value);
	}
	else if (MATCH("", "spp_pt_size"))
	{
		knob::spp_pt_size = atoi(value);
	}
	else if (MATCH("", "spp_max_outcomes"))
	{
		knob::spp_max_outcomes = atoi(value);
	}
	else if (MATCH("", "spp_max_confidence"))
	{
		knob::spp_max_confidence = strtod(value, NULL);
	}
	else if (MATCH("", "spp_max_depth"))
	{
		knob::spp_max_depth = atoi(value);
	}
	else if (MATCH("", "spp_max_prefetch_per_level"))
	{
		knob::spp_max_prefetch_per_level = atoi(value);
	}
	else if (MATCH("", "spp_max_confidence_counter_value"))
	{
		knob::spp_max_confidence_counter_value = atoi(value);
	}
	else if (MATCH("", "spp_max_global_counter_value"))
	{
		knob::spp_max_global_counter_value = atoi(value);
	}
	else if (MATCH("", "spp_pf_size"))
	{
		knob::spp_pf_size = atoi(value);
	}
	else if (MATCH("", "spp_enable_alpha"))
	{
		knob::spp_enable_alpha = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_enable_pref_buffer"))
	{
		knob::spp_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_pref_buffer_size"))
	{
		knob::spp_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "spp_pref_degree"))
	{
		knob::spp_pref_degree = atoi(value);
	}
	else if (MATCH("", "spp_enable_ghr"))
	{
		knob::spp_enable_ghr = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "spp_ghr_size"))
	{
		knob::spp_ghr_size = atoi(value);
	}
	else if (MATCH("", "spp_signature_bits"))
	{
		knob::spp_signature_bits = atoi(value);
	}
	else if (MATCH("", "spp_alpha_epoch"))
	{
		knob::spp_alpha_epoch = atoi(value);
	}

	/* Scooby */
	else if (MATCH("", "scooby_alpha"))
	{
		knob::scooby_alpha = atof(value);
	}
	else if (MATCH("", "scooby_gamma"))
	{
		knob::scooby_gamma = atof(value);
	}
	else if (MATCH("", "scooby_epsilon"))
	{
		knob::scooby_epsilon = atof(value);
	}
	else if (MATCH("", "scooby_max_states"))
	{
		knob::scooby_max_states = atoi(value);
	}
	else if (MATCH("", "scooby_seed"))
	{
		knob::scooby_seed = atoi(value);
	}
	else if (MATCH("", "scooby_policy"))
	{
		knob::scooby_policy = string(value);
	}
	else if (MATCH("", "scooby_learning_type"))
	{
		knob::scooby_learning_type = string(value);
	}
	else if (MATCH("", "scooby_actions"))
	{
		knob::scooby_actions = get_array_int(value);
		knob::scooby_max_actions = knob::scooby_actions.size();
	}
	else if (MATCH("", "scooby_pt_size"))
	{
		knob::scooby_pt_size = atoi(value);
	}
	else if (MATCH("", "scooby_st_size"))
	{
		knob::scooby_st_size = atoi(value);
	}
	else if (MATCH("", "scooby_reward_none"))
	{
		knob::scooby_reward_none = atoi(value);
	}
	else if (MATCH("", "scooby_reward_incorrect"))
	{
		knob::scooby_reward_incorrect = atoi(value);
	}
	else if (MATCH("", "scooby_reward_correct_untimely"))
	{
		knob::scooby_reward_correct_untimely = atoi(value);
	}
	else if (MATCH("", "scooby_reward_correct_timely"))
	{
		knob::scooby_reward_correct_timely = atoi(value);
	}
	else if (MATCH("", "scooby_max_pcs"))
	{
		knob::scooby_max_pcs = atoi(value);
	}
	else if (MATCH("", "scooby_max_offsets"))
	{
		knob::scooby_max_offsets = atoi(value);
	}
	else if (MATCH("", "scooby_max_deltas"))
	{
		knob::scooby_max_deltas = atoi(value);
	}

    else 
    {
    	printf("unable to parse section: %s, name: %s, value: %s\n", section, name, value);
        return 0;
    }
    return 1;
}

std::vector<int32_t> get_array_int(const char *str)
{
	std::vector<int32_t> value;
	char *tmp_str = strdup(str);
	char *pch = strtok(tmp_str, ",");
	while(pch)
	{
		value.push_back(atoi(pch));
		pch = strtok(NULL, ",");
	}
	free(tmp_str);
	return value;
}
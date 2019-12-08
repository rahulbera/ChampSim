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
	bool     l1d_perfect = false;
	bool     l2c_perfect = false;
	bool     llc_perfect = false;

	/* next-line */
	vector<int32_t>  next_line_deltas;
	vector<float>  next_line_delta_prob;
	uint32_t next_line_seed = 255;
	uint32_t next_line_pt_size = 256;
	bool     next_line_enable_prefetch_tracking = true;
	bool     next_line_enable_trace = false;
	uint32_t next_line_trace_interval = 5;
	string   next_line_trace_name = string("next_line_trace.csv");

	/* SMS */
	uint32_t sms_at_size = 32;
	uint32_t sms_ft_size = 64;
	uint32_t sms_pht_size = 16384;
	uint32_t sms_pref_degree = 4;
	uint32_t sms_region_size = 2048;
	uint32_t sms_region_size_log = 11;
	bool     sms_enable_pref_buffer = true;
	uint32_t sms_pref_buffer_size = 256;

	/* SPP */
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
	bool     scooby_brain_zero_init;
	bool     scooby_enable_reward_all;
	bool     scooby_enable_track_multiple;
	bool     scooby_enable_reward_for_out_of_bounds;
	int32_t  scooby_reward_out_of_bounds;
	uint32_t scooby_state_type;
	bool     scooby_access_debug;
	bool     scooby_enable_state_action_stats;

	/* Learning Engine */
	bool     le_enable_trace;
	uint32_t le_trace_interval;
	string   le_trace_file_name;
	uint32_t le_trace_state;
	bool     le_enable_score_plot;
	vector<int32_t> le_plot_actions;
	string   le_plot_file_name;
	bool     le_enable_action_trace;
	uint32_t le_action_trace_interval;
	std::string le_action_trace_name;
	bool     le_enable_action_plot;
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
    else if (MATCH("", "l1d_perfect"))
    {
		knob::l1d_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "l2c_perfect"))
    {
		knob::l2c_perfect = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "llc_perfect"))
    {
		knob::llc_perfect = !strcmp(value, "true") ? true : false;
    }

    /* next-line */
    else if (MATCH("", "next_line_deltas"))
    {
		knob::next_line_deltas = get_array_int(value);
    }
    else if (MATCH("", "next_line_delta_prob"))
    {
		knob::next_line_delta_prob = get_array_float(value);
    }
    else if (MATCH("", "next_line_seed"))
    {
		knob::next_line_seed = atoi(value);
    }
    else if (MATCH("", "next_line_pt_size"))
    {
		knob::next_line_pt_size = atoi(value);
    }
    else if (MATCH("", "next_line_enable_prefetch_tracking"))
    {
		knob::next_line_enable_prefetch_tracking = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "next_line_enable_trace"))
    {
		knob::next_line_enable_trace = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "next_line_trace_interval"))
    {
		knob::next_line_trace_interval = atoi(value);
    }
    else if (MATCH("", "next_line_trace_name"))
    {
		knob::next_line_trace_name = string(value);
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
	else if (MATCH("", "scooby_brain_zero_init"))
	{
		knob::scooby_brain_zero_init = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_all"))
	{
		knob::scooby_enable_reward_all = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_track_multiple"))
	{
		knob::scooby_enable_track_multiple = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_for_out_of_bounds"))
	{
		knob::scooby_enable_reward_for_out_of_bounds = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_reward_out_of_bounds"))
	{
		knob::scooby_reward_out_of_bounds = atoi(value);
	}
	else if (MATCH("", "scooby_state_type"))
	{
		knob::scooby_state_type = atoi(value);
	}
	else if (MATCH("", "scooby_access_debug"))
	{
		knob::scooby_access_debug = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_state_action_stats"))
	{
		knob::scooby_enable_state_action_stats = !strcmp(value, "true") ? true : false;
	}

	/* Learning Engine */
	else if (MATCH("", "le_enable_trace"))
	{
		knob::le_enable_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_trace_interval"))
	{
		knob::le_trace_interval = atoi(value);
	}
	else if (MATCH("", "le_trace_file_name"))
	{
		knob::le_trace_file_name = string(value);
	}
	else if (MATCH("", "le_trace_state"))
	{
		knob::le_trace_state = strtoul(value, NULL, 0);
	}
	else if (MATCH("", "le_enable_score_plot"))
	{
		knob::le_enable_score_plot = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_plot_actions"))
	{
		knob::le_plot_actions = get_array_int(value);
	}
	else if (MATCH("", "le_plot_file_name"))
	{
		knob::le_plot_file_name = string(value);
	}
	else if (MATCH("", "le_enable_action_trace"))
	{
		knob::le_enable_action_trace = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "le_action_trace_interval"))
	{
		knob::le_action_trace_interval = atoi(value);
	}
	else if (MATCH("", "le_action_trace_name"))
	{
		knob::le_action_trace_name = string(value);
	}
	else if (MATCH("", "le_enable_action_plot"))
	{
		knob::le_enable_action_plot = !strcmp(value, "true") ? true : false;
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

std::vector<float> get_array_float(const char *str)
{
	std::vector<float> value;
	char *tmp_str = strdup(str);
	char *pch = strtok(tmp_str, ",");
	while(pch)
	{
		value.push_back(atof(pch));
		pch = strtok(NULL, ",");
	}
	free(tmp_str);
	return value;
}
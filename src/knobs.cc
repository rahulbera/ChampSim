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
	vector<string> 	 l2c_prefetcher_types;
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
	uint32_t sms_pht_assoc = 16;
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

	/* BOP */
	vector<int32_t> bop_candidates;
	uint32_t bop_max_rounds = 100;
	uint32_t bop_max_score = 31;
	uint32_t bop_top_n = 1;
	bool     bop_enable_pref_buffer = false;
	uint32_t bop_pref_buffer_size = 256;
	uint32_t bop_pref_degree = 4;
	uint32_t bop_rr_size = 256;

	/* Sandbox */
	uint32_t sandbox_pref_degree = 4;
	bool     sandbox_enable_stream_detect = false;
	uint32_t sandbox_stream_detect_length = 4;
	uint32_t sandbox_num_access_in_phase = 256;
	uint32_t sandbox_num_cycle_offsets = 4;
	uint32_t sandbox_bloom_filter_size = 2048;
	uint32_t sandbox_seed = 200;

	/* Scooby */
	float    scooby_alpha;
	float    scooby_gamma;
	float    scooby_epsilon;
	uint32_t scooby_state_num_bits;
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
	bool     scooby_enable_reward_out_of_bounds;
	int32_t  scooby_reward_out_of_bounds;
	uint32_t scooby_state_type;
	bool     scooby_access_debug;
	bool     scooby_print_access_debug;
	bool     scooby_enable_state_action_stats;
	bool     scooby_enable_reward_tracker_hit;
	int32_t  scooby_reward_tracker_hit;
	bool     scooby_enable_shaggy;
	uint32_t scooby_state_hash_type;
	bool     scooby_prefetch_with_shaggy;
	bool     scooby_enable_cmac_engine;

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

	/* CMAC Engine */
	uint32_t scooby_cmac_num_planes;
	uint32_t scooby_cmac_num_entries_per_plane;
	vector<int32_t> scooby_cmac_plane_offsets;
	vector<int32_t> scooby_cmac_dim_granularities;
	vector<int32_t> scooby_cmac_action_factors;
	uint32_t scooby_cmac_hash_type;

	/* Shaggy */
	uint32_t shaggy_pb_size;
	uint32_t shaggy_st_size;
	uint32_t shaggy_degree;
	uint32_t shaggy_sig_length;
	uint32_t shaggy_sig_type;
	uint32_t shaggy_page_access_threshold;

	/* Velma */
	vector<string> velma_candidate_prefetchers;
	uint32_t velma_pref_selection_type;
	double   velma_alpha;
	double   velma_gamma;
	double   velma_epsilon;
	uint32_t velma_max_actions;
	uint32_t velma_max_states;
	uint32_t velma_seed;
	string	 velma_policy;
	string	 velma_learning_type;
	bool	 velma_brain_zero_init;
	uint32_t velma_state_type;
	vector<int32_t> velma_actions;
	uint32_t velma_pt_size;
	int32_t  velma_reward_accurate;
	int32_t  velma_reward_inaccurate;

	/* Fred */
	uint32_t fred_bloom_filter_size = 4096;
	uint32_t fred_num_access_in_round = 256;
	uint32_t fred_seed = 200;
	uint32_t fred_init_pref_selection_type = 1;
	uint32_t fred_num_access_in_apply_phase = 10000;
	float    fred_score_cutoff = 0.5;
}

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
	char config_file_name[MAX_LEN];

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
	char config_file_name[MAX_LEN];

	if(MATCH("", "config"))
	{
		strcpy(config_file_name, value);
		parse_config(config_file_name);
	}
    else if (MATCH("", "warmup_instructions"))
    {
		knob::warmup_instructions = atol(value);
    }
    else if (MATCH("", "simulation_instructions"))
    {
		knob::simulation_instructions = atol(value);
    }
    else if (MATCH("", "knob_cloudsuite"))
    {
		knob::knob_cloudsuite = !strcmp(value, "true") ? true : false;
    }
    else if (MATCH("", "knob_low_bandwidth"))
    {
		knob::knob_low_bandwidth = atoi(value);
    }
    else if (MATCH("", "l2c_prefetcher_types"))
    {
		knob::l2c_prefetcher_types.push_back(string(value));
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
	else if(MATCH("", "sms_pht_assoc"))
	{
		knob::sms_pht_assoc = atoi(value);
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

	/* BOP */
	else if (MATCH("", "bop_candidates"))
	{
		knob::bop_candidates = get_array_int(value);
	}
	else if (MATCH("", "bop_max_rounds"))
	{
		knob::bop_max_rounds = atoi(value);
	}
	else if (MATCH("", "bop_max_score"))
	{
		knob::bop_max_score = atoi(value);
	}
	else if (MATCH("", "bop_top_n"))
	{
		knob::bop_top_n = atoi(value);
	}
	else if (MATCH("", "bop_enable_pref_buffer"))
	{
		knob::bop_enable_pref_buffer = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "bop_pref_buffer_size"))
	{
		knob::bop_pref_buffer_size = atoi(value);
	}
	else if (MATCH("", "bop_pref_degree"))
	{
		knob::bop_pref_degree = atoi(value);
	}
	else if (MATCH("", "bop_rr_size"))
	{
		knob::bop_rr_size = atoi(value);
	}

	/* Sandbox */
	else if (MATCH("", "sandbox_pref_degree"))
	{
		knob::sandbox_pref_degree = atoi(value);
	}
	else if (MATCH("", "sandbox_enable_stream_detect"))
	{
		knob::sandbox_enable_stream_detect = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "sandbox_stream_detect_length"))
	{
		knob::sandbox_stream_detect_length = atoi(value);
	}
	else if (MATCH("", "sandbox_num_access_in_phase"))
	{
		knob::sandbox_num_access_in_phase = atoi(value);
	}
	else if (MATCH("", "sandbox_num_cycle_offsets"))
	{
		knob::sandbox_num_cycle_offsets = atoi(value);
	}
	else if (MATCH("", "sandbox_bloom_filter_size"))
	{
		knob::sandbox_bloom_filter_size = atoi(value);
	}
	else if (MATCH("", "sandbox_seed"))
	{
		knob::sandbox_seed = atoi(value);
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
	else if (MATCH("", "scooby_state_num_bits"))
	{
		knob::scooby_state_num_bits = atoi(value);
		knob::scooby_max_states = pow(2.0, knob::scooby_state_num_bits);
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
	else if (MATCH("", "scooby_enable_reward_out_of_bounds"))
	{
		knob::scooby_enable_reward_out_of_bounds = !strcmp(value, "true") ? true : false;
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
	else if (MATCH("", "scooby_print_access_debug"))
	{
		knob::scooby_print_access_debug = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_state_action_stats"))
	{
		knob::scooby_enable_state_action_stats = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_reward_tracker_hit"))
	{
		knob::scooby_enable_reward_tracker_hit = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_reward_tracker_hit"))
	{
		knob::scooby_reward_tracker_hit = atoi(value);
	}
	else if (MATCH("", "scooby_enable_shaggy"))
	{
		knob::scooby_enable_shaggy = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_state_hash_type"))
	{
		knob::scooby_state_hash_type = atoi(value);
	}
	else if (MATCH("", "scooby_prefetch_with_shaggy"))
	{
		knob::scooby_prefetch_with_shaggy = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "scooby_enable_cmac_engine"))
	{
		knob::scooby_enable_cmac_engine = !strcmp(value, "true") ? true : false;
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

	/* CMAC Engine */
	else if (MATCH("", "scooby_cmac_num_planes"))
	{
		knob::scooby_cmac_num_planes = atoi(value);
	}
	else if (MATCH("", "scooby_cmac_num_entries_per_plane"))
	{
		knob::scooby_cmac_num_entries_per_plane = atoi(value);
	}
	else if (MATCH("", "scooby_cmac_plane_offsets"))
	{
		knob::scooby_cmac_plane_offsets = get_array_int(value);
	}
	else if (MATCH("", "scooby_cmac_dim_granularities"))
	{
		knob::scooby_cmac_dim_granularities = get_array_int(value);
	}
	else if (MATCH("", "scooby_cmac_action_factors"))
	{
		knob::scooby_cmac_action_factors = get_array_int(value);
	}
	else if (MATCH("", "scooby_cmac_hash_type"))
	{
		knob::scooby_cmac_hash_type = atoi(value);
	}

	/* Shaggy knobs */
	else if (MATCH("", "shaggy_pb_size"))
	{
		knob::shaggy_pb_size = atoi(value);
	}
	else if (MATCH("", "shaggy_st_size"))
	{
		knob::shaggy_st_size = atoi(value);
	}
	else if (MATCH("", "shaggy_degree"))
	{
		knob::shaggy_degree = atoi(value);
	}
	else if (MATCH("", "shaggy_sig_length"))
	{
		knob::shaggy_sig_length = atoi(value);
	}
	else if (MATCH("", "shaggy_sig_type"))
	{
		knob::shaggy_sig_type = atoi(value);
	}
	else if (MATCH("", "shaggy_page_access_threshold"))
	{
		knob::shaggy_page_access_threshold = atoi(value);
	}

	/* Velma */
	else if (MATCH("", "velma_candidate_prefetchers"))
	{
		knob::velma_candidate_prefetchers.push_back(string(value));
	}
	else if (MATCH("", "velma_pref_selection_type"))
	{
		knob::velma_pref_selection_type = atoi(value);
	}
	else if (MATCH("", "velma_alpha"))
	{
		knob::velma_alpha = atof(value);
	}
	else if (MATCH("", "velma_gamma"))
	{
		knob::velma_gamma = atof(value);
	}
	else if (MATCH("", "velma_epsilon"))
	{
		knob::velma_epsilon = atof(value);
	}
	else if (MATCH("", "velma_max_states"))
	{
		knob::velma_max_states = atoi(value);
	}
	else if (MATCH("", "velma_seed"))
	{
		knob::velma_seed = atoi(value);
	}
	else if (MATCH("", "velma_policy"))
	{
		knob::velma_policy = string(value);
	}
	else if (MATCH("", "velma_learning_type"))
	{
		knob::velma_learning_type = string(value);
	}
	else if (MATCH("", "velma_brain_zero_init"))
	{
		knob::velma_brain_zero_init = !strcmp(value, "true") ? true : false;
	}
	else if (MATCH("", "velma_state_type"))
	{
		knob::velma_state_type = atoi(value);
	}
	else if (MATCH("", "velma_actions"))
	{
		knob::velma_actions = get_array_int(value);
		knob::velma_max_actions = knob::velma_actions.size();
	}
	else if (MATCH("", "velma_pt_size"))
	{
		knob::velma_pt_size = atoi(value);
	}
	else if (MATCH("", "velma_reward_accurate"))
	{
		knob::velma_reward_accurate = atoi(value);
	}
	else if (MATCH("", "velma_reward_inaccurate"))
	{
		knob::velma_reward_inaccurate = atoi(value);
	}

	/* Fred */
	else if (MATCH("", "fred_bloom_filter_size"))
	{
		knob::fred_bloom_filter_size = atoi(value);
	}
	else if (MATCH("", "fred_num_access_in_round"))
	{
		knob::fred_num_access_in_round = atoi(value);
	}
	else if (MATCH("", "fred_seed"))
	{
		knob::fred_seed = atoi(value);
	}
	else if (MATCH("", "fred_init_pref_selection_type"))
	{
		knob::fred_init_pref_selection_type = atoi(value);
	}
	else if (MATCH("", "fred_num_access_in_apply_phase"))
	{
		knob::fred_num_access_in_apply_phase = atoi(value);
	}
	else if (MATCH("", "fred_score_cutoff"))
	{
		knob::fred_score_cutoff = atof(value);
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

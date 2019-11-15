#ifndef KNOBS_H
#define KNOBS_H

#define MAX_LEN 256

void parse_args(int argc, char* argv[]);
void parse_config(char *config_file_name);
int parse_knobs(void* user, const char* section, const char* name, const char* value);
int handler(void* user, const char* section, const char* name, const char* value);

#endif /* KNOBS_H */
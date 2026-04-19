#ifndef BHARAT_SYSTEM_SHELL_PARSER_H
#define BHARAT_SYSTEM_SHELL_PARSER_H

#include "shell.h"

shell_status_code_t shell_parse_line(char* line, shell_argv_t* out_argv);

#endif

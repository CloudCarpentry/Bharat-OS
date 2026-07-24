#include "bench_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    bench_runner_config_t config;
    memset(&config, 0, sizeof(config));
    config.format = OUTPUT_FORMAT_CONSOLE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) {
            config.format = OUTPUT_FORMAT_JSON;
        } else if (strcmp(argv[i], "--csv") == 0) {
            config.format = OUTPUT_FORMAT_CSV;
        } else if (strcmp(argv[i], "--group") == 0 && i + 1 < argc) {
            config.filter_group = argv[++i];
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            config.filter_name = argv[++i];
        } else if (strcmp(argv[i], "--iters") == 0 && i + 1 < argc) {
            config.override_iterations = strtoull(argv[++i], NULL, 10);
        }
    }

    bench_runner_init(&config);
    int res = bench_runner_execute_all();
    bench_runner_print_summary();
    return res;
}

with open("kernel/src/sched.c", "r") as f:
    content = f.read()

part1 = """    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
        g_runqueues[i].current_thread = NULL;
        for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
            list_init(&g_runqueues[i].ready_queue[p]);
        }
        g_runqueues[i].active_weight = 0U;
        g_runqueues[i].total_ticks = 0U;
        g_runqueues[i].throttled = 0U;
    }

    kprocess_t* idle_process = process_create("idle_process");

    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
        kthread_t* idle_th = thread_create(idle_process, sched_idle_task);"""

part2 = """    kprocess_t* idle_process = process_create("idle_process");

    for (uint32_t i = 0; i < MAX_SUPPORTED_CORES; ++i) {
        g_runqueues[i].current_thread = NULL;
        for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
            list_init(&g_runqueues[i].ready_queue[p]);
        }
        g_runqueues[i].active_weight = 0U;
        g_runqueues[i].total_ticks = 0U;
        g_runqueues[i].throttled = 0U;

        kthread_t* idle_th = thread_create(idle_process, sched_idle_task);"""

content = content.replace(part2, part1)

with open("kernel/src/sched.c", "w") as f:
    f.write(content)

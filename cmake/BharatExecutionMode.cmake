# BharatExecutionMode.cmake
# Stub implementation to unblock build
# Full execution-mode framework not yet integrated

option(BHARAT_ENABLE_EXECUTION_MODE_FRAMEWORK "Enable execution mode framework" OFF)

set(BHARAT_SYSTEM_PROFILE "GP" CACHE STRING "System profile (RT, GP, MIX)")
set(BHARAT_EXECUTION_MODE "SMP" CACHE STRING "Execution mode (SMP, AMP, PARTITIONED)")

# Logical CPU count (fallback)
set(BHARAT_LOGICAL_CPU_COUNT 1 CACHE STRING "Logical CPU count")

# Scheduler class (default fallback)
set(BHARAT_SCHEDULER_CLASS "DEFAULT" CACHE STRING "Scheduler class")

message(STATUS "[ExecutionMode] Framework: STUB (disabled)")
message(STATUS "[ExecutionMode] Profile: ${BHARAT_SYSTEM_PROFILE}")
message(STATUS "[ExecutionMode] Mode: ${BHARAT_EXECUTION_MODE}")

#!/bin/bash
set -e
cd /app
mkdir -p build/test_net
cd build/test_net
cmake ../.. -DBHARAT_PROFILE_DESKTOP=1 -DBHARAT_PERSONALITY_NONE=1
make net_service
echo "Testing build for net_service successful!"

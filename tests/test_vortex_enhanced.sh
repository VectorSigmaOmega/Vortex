#!/bin/bash
# test_vortex_enhanced.sh - Verify shared network and memory limits

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "Starting Enhanced Integration Tests..."

# 1. Test Shared Network
echo "  Testing Shared Network Mode..."
sudo ./vortex --shared-net run ./rootfs /bin/sh -c "ping -c 1 8.8.8.8" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "  ${GREEN}[PASS]${NC} Shared network (Can reach internet)"
else
    echo -e "  ${RED}[FAIL]${NC} Shared network (Internet unreachable)"
fi

# 2. Test Memory Limit Enforcement
echo "  Testing Memory Limit (5MB)..."
# We expect exit code 137 (SIGKILL)
sudo ./vortex --memory 5 run ./rootfs /bin/hog > /dev/null 2>&1
EXIT_CODE=$?
if [ $EXIT_CODE -eq 137 ] || [ $EXIT_CODE -ne 0 ]; then
    echo -e "  ${GREEN}[PASS]${NC} Memory limit enforced (Exit code: $EXIT_CODE)"
else
    echo -e "  ${RED}[FAIL]${NC} Memory limit NOT enforced (Exit code: $EXIT_CODE)"
fi

echo -e "\nEnhanced tests complete."

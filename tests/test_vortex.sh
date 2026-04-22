#!/bin/bash
# test_vortex.sh - Automated Integration Tests for Vortex (Silent Mode)

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' 

function assert_equals() {
    if [ "$1" == "$2" ]; then
        echo -e "  ${GREEN}[PASS]${NC} $3"
    else
        echo -e "  ${RED}[FAIL]${NC} $3 (Expected '$1', got '$2')"
    fi
}

echo -e "  Starting Vortex Automated Tests...\n"

# 1. Test UTS Namespace (Hostname)
CONTAINER_HOSTNAME=$(sudo ./vortex run ./rootfs /bin/sh -c "hostname" 2>/dev/null | tail -n 1)
assert_equals "vortex-container" "$CONTAINER_HOSTNAME" "Hostname isolation"

# 2. Test PID Namespace (PID 1)
CONTAINER_PID=$(sudo ./vortex run ./rootfs /bin/sh -c "echo \$$" 2>/dev/null | tail -n 1)
assert_equals "1" "$CONTAINER_PID" "PID isolation (PID 1)"

# 3. Test Filesystem isolation
sudo ./vortex run ./rootfs /bin/sh -c "cat /etc/alpine-release" >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo -e "  ${GREEN}[PASS]${NC} Filesystem isolation (Alpine identified)"
else
    echo -e "  ${RED}[FAIL]${NC} Filesystem isolation"
fi

# 4. Test Network isolation
IFACE_COUNT=$(sudo ./vortex run ./rootfs /bin/sh -c "ls /sys/class/net | wc -l" 2>/dev/null | tail -n 1)
assert_equals "1" "$IFACE_COUNT" "Network isolation"

echo -e "\n  Test suite complete."

#!/bin/bash
# test_vortex.sh - Automated Integration Tests for Vortex

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

function assert_equals() {
    if [ "$1" == "$2" ]; then
        echo -e "${GREEN}[PASS]${NC} $3"
    else
        echo -e "${RED}[FAIL]${NC} $3 (Expected '$1', got '$2')"
        exit 1
    fi
}

echo "Starting Vortex Automated Tests..."

# 1. Test UTS Namespace (Hostname)
CONTAINER_HOSTNAME=$(sudo ./vortex run ./rootfs /bin/sh -c "hostname")
assert_equals "vortex-container" "$CONTAINER_HOSTNAME" "Hostname isolation"

# 2. Test PID Namespace (PID 1)
# We check if the process sees itself as PID 1
CONTAINER_PID=$(sudo ./vortex run ./rootfs /bin/sh -c "echo \$$")
assert_equals "1" "$CONTAINER_PID" "PID isolation (Container thinks it is PID 1)"

# 3. Test Filesystem isolation (Check for a host file that shouldn't be there)
# We'll check if /etc/shadow is accessible or if we are in the Alpine root
IS_ALPINE=$(sudo ./vortex run ./rootfs /bin/sh -c "cat /etc/alpine-release 2>/dev/null")
if [ ! -z "$IS_ALPINE" ]; then
    echo -e "${GREEN}[PASS]${NC} Filesystem isolation (Alpine identified)"
else
    echo -e "${RED}[FAIL]${NC} Filesystem isolation (Alpine NOT identified)"
    exit 1
fi

# 4. Test Network isolation (Should not see host interfaces)
# Host likely has eth0/wlan0, container should only have lo
IFACE_COUNT=$(sudo ./vortex run ./rootfs /bin/sh -c "ls /sys/class/net | wc -l")
assert_equals "1" "$IFACE_COUNT" "Network isolation (Only 1 interface found)"

echo -e "\n${GREEN}All tests passed successfully!${NC}"

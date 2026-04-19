#!/bin/bash
set -e

ROOTFS_DIR="./rootfs"
ALPHINE_VERSION="3.19.1"
ALPHINE_URL="https://dl-cdn.alpinelinux.org/alpine/v3.19/releases/x86_64/alpine-minirootfs-${ALPHINE_VERSION}-x86_64.tar.gz"

if [ ! -f "alpine-minirootfs.tar.gz" ]; then
    echo "Downloading Alpine RootFS..."
    curl -L -o alpine-minirootfs.tar.gz ${ALPHINE_URL}
fi

echo "Extracting RootFS to ${ROOTFS_DIR}..."
mkdir -p ${ROOTFS_DIR}
tar -xzf alpine-minirootfs.tar.gz -C ${ROOTFS_DIR}

echo "RootFS setup complete."

#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DL_URL="https://helldiner.re"
SUPPORTED_ARCHS=("x86" "x86-64" "arm" "arm64")

ARCH=$1

if [[ -z "$ARCH" ]]; then
    echo "Usage: $0 <arch_name|all>"
    exit 1
fi

if [[ "$ARCH" == "all" ]]; then
    for TARGET_ARCH in "${SUPPORTED_ARCHS[@]}"; do
        "$0" "$TARGET_ARCH"
    done
    exit 0
fi

ARCH_SUPPORTED=false
for TARGET_ARCH in "${SUPPORTED_ARCHS[@]}"; do
    if [[ "$ARCH" == "$TARGET_ARCH" ]]; then
        ARCH_SUPPORTED=true
        break
    fi
done

if ! $ARCH_SUPPORTED; then
    echo "Unknown/Unsupported architecture. List of supported architectures :"
    for TARGET_ARCH in "${SUPPORTED_ARCHS[@]}"; do
        echo "- ${TARGET_ARCH}"
    done
    exit 1
fi

ARCH_DIR="$PROJECT_ROOT/rootfs/$ARCH"
if [[ -d "$ARCH_DIR" ]]; then
    rm -rf "$ARCH_DIR"
fi

cd "$PROJECT_ROOT/rootfs"

if [[ -f "rootfs_${ARCH}.tar.gz" ]]; then
    rm "rootfs_${ARCH}.tar.gz"
fi

echo "Downloading $ARCH rootfs..."
curl -LO "$DL_URL/rootfs_${ARCH}.tar.gz"

echo "Extracting $ARCH rootfs..."
tar -xf "rootfs_${ARCH}.tar.gz"
rm "rootfs_${ARCH}.tar.gz"

echo "Done."

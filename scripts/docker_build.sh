#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

PLATFORM=$1
VERSION=$2

if [[ -z "$PLATFORM" || -z "$VERSION" ]]; then
    echo "Usage: $0 <platform|all|all-sync> <version>"
    exit 1
fi

if [[ "$PLATFORM" == "all" ]]; then
    declare -a pids=()

    for DOCKERFILE in "$PROJECT_ROOT"/docker/Dockerfile.*; do
        BASE_NAME=$(basename "$DOCKERFILE")
        TARGET_PLATFORM="${BASE_NAME#Dockerfile.}"
        "$0" "$TARGET_PLATFORM" "$VERSION" &
        pids+=($!)
    done

    for pid in "${pids[@]}"; do
        wait $pid || echo "Build with PID $pid failed"
    done

    exit 0
fi

if [[ "$PLATFORM" == "all-sync" ]]; then
    for DOCKERFILE in "$PROJECT_ROOT"/docker/Dockerfile.*; do
        BASE_NAME=$(basename "$DOCKERFILE")
        TARGET_PLATFORM="${BASE_NAME#Dockerfile.}"

        echo "Starting build for $TARGET_PLATFORM..."
        "$0" "$TARGET_PLATFORM" "$VERSION"

        if [[ $? -ne 0 ]]; then
            echo "Build for $TARGET_PLATFORM failed"
            exit 1
        fi
    done

    exit 0
fi

if [[ "$PLATFORM" =~ ^(ubuntu|debian) ]]; then
    PACKAGE_EXT="deb"
elif [[ "$PLATFORM" =~ ^(fedora|centos|rhel) ]]; then
    PACKAGE_EXT="rpm"
else
    echo "Unsupported platform: $PLATFORM"
    docker rm "$CONTAINER_ID"
    exit 1
fi

PACKAGE_NAME="arion-${PLATFORM}-${VERSION}.${PACKAGE_EXT}"

DOCKERFILE="$PROJECT_ROOT/docker/Dockerfile.$PLATFORM"

if [[ ! -f "$DOCKERFILE" ]]; then
    echo "Error: Dockerfile for $PLATFORM not found!"
    exit 1
fi

IMAGE_NAME="arion_$PLATFORM"

mkdir -p "$PROJECT_ROOT/build"

echo "Building Docker image for $PLATFORM..."
docker build --build-arg VERSION=$VERSION -t $IMAGE_NAME -f $DOCKERFILE "$PROJECT_ROOT"

echo "Running build inside container..."
CONTAINER_ID=$(docker create $IMAGE_NAME)

echo "Extracting package..."
docker cp "$CONTAINER_ID:/app/docker_build/arion-$VERSION.$PACKAGE_EXT" "$PROJECT_ROOT/build/$PACKAGE_NAME"

echo "Cleaning up..."
docker rm "$CONTAINER_ID"
docker rmi "$IMAGE_NAME"

echo "Build completed: $PROJECT_ROOT/build/$PACKAGE_NAME"

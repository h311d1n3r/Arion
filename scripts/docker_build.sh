#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

PLATFORM=$1
VERSION=$2

if [[ -z "$PLATFORM" ]]; then
    echo "Please specify target platform as parameter."
    exit 1
fi

if [[ -z "$VERSION" ]]; then
    echo "Please specify build version as parameter."
    exit 1
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

docker cp "$CONTAINER_ID:/app/docker_build/arion-$VERSION.$PACKAGE_EXT" "$PROJECT_ROOT/build/$PACKAGE_NAME"

docker rm "$CONTAINER_ID"

echo "Build completed: $PROJECT_ROOT/build/$PACKAGE_NAME"

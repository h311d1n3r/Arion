#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

ARCH=$1
BR_PATH=$2

if [[ -z "$ARCH" || -z "$BR_PATH" ]]; then
    echo "Usage: $0 <arch_name|all> <buildroot_path>"
    exit 1
fi

if [[ "$ARCH" == "all" ]]; then
    declare -a pids=()

    for CONFIG in "$PROJECT_ROOT"/rootfs/br_configs/arion_*_defconfig; do
        BASE_NAME=$(basename "$CONFIG")
        TARGET_ARCH="${BASE_NAME#arion_}"
        TARGET_ARCH="${TARGET_ARCH%_defconfig}"
        "$0" "$TARGET_ARCH" "$BR_PATH" &
        pids+=($!)
    done

    for pid in "${pids[@]}"; do
        wait $pid || echo "Build with PID $pid failed"
    done

    exit 0
fi

BR_CONFIG="$PROJECT_ROOT/rootfs/br_configs/arion_${ARCH}_defconfig"

if [[ ! -f "$BR_CONFIG" ]]; then
    echo "Error: Buildroot config for $ARCH not found!"
    exit 1
fi

if [[ ! -d "$BR_PATH" ]]; then
    echo "Error: Buildroot path does not exist !"
    exit 1
fi

BR_OUTPUT="${BR_PATH}/output"
if [[ -d "$BR_OUTPUT" ]]; then
    echo "Cleaning up Buildroot output..."
    rm -rf $BR_OUTPUT
fi

cd "$BR_PATH"

echo "Configuring Buildroot for $ARCH..."
make BR2_DEFCONFIG="$BR_CONFIG" defconfig

echo "Building rootfs for $ARCH..."
make -j$(($(nproc)-1))

ARCH_ROOTFS="${BR_OUTPUT}/${ARCH}_rootfs"

echo "Processing post-build patches..."
if [[ -d "$ARCH_ROOTFS" ]]; then
    rm -rf "$ARCH_ROOTFS"
fi
mkdir $ARCH_ROOTFS
cd $BR_OUTPUT
tar -xf images/rootfs.tar -C "$ARCH_ROOTFS"
cp -r "${BR_OUTPUT}/host/${ARCH}-buildroot-linux-gnu/sysroot/"* "${ARCH_ROOTFS}/"
BR_GCC_DIR=$(find ${BR_OUTPUT}/host/lib/gcc/${ARCH}-buildroot-linux-gnu/ -mindepth 1 -maxdepth 1 -type d | head -n1)
cp -r "${BR_GCC_DIR}/"* "${ARCH_ROOTFS}/usr/lib/"

TOOLCHAINS_DIR="${BR_OUTPUT}/${ARCH}_toolchains"
if [[ -d "$TOOLCHAINS_DIR" ]]; then
    rm -rf "$TOOLCHAINS_DIR"
fi
mkdir $TOOLCHAINS_DIR

echo "Building static Binutils cross toolchains..."
BINUTILS_BUILD_DIR="${BR_OUTPUT}/${ARCH}_binutils"
if [[ -d "$BINUTILS_BUILD_DIR" ]]; then
    rm -rf "$BINUTILS_BUILD_DIR"
fi
mkdir $BINUTILS_BUILD_DIR
BINUTILS_VERSION=$(curl -s https://ftp.gnu.org/gnu/binutils/ | grep -oP 'binutils-\d+\.\d+\.tar\.xz' | sed -E 's|binutils-([0-9.]+)\.tar\.xz|\1|' | sort -V | tail -n1)
echo "Identified Binutils LTS version: $BINUTILS_VERSION. Downloading source code..."
BINUTILS_SOURCE_DIR="${BR_OUTPUT}/binutils-${BINUTILS_VERSION}"
if [[ -d "$BINUTILS_SOURCE_DIR" ]]; then
    rm -rf "$BINUTILS_SOURCE_DIR"
fi
curl -LO "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz"
tar -xf "binutils-${BINUTILS_VERSION}.tar.xz"
rm "binutils-${BINUTILS_VERSION}.tar.xz"
cd "$BINUTILS_BUILD_DIR"
echo "Configuring Binutils..."
$BINUTILS_SOURCE_DIR/configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=${ARCH}-linux-gnu --prefix="$TOOLCHAINS_DIR" --with-build-sysroot="$ARCH_ROOTFS" --disable-nls --program-suffix=-"$BINUTILS_VERSION"
make configure-host
echo "Building Binutils..."
make -j$(($(nproc)-1)) LDFLAGS="-all-static"
echo "Deploying Binutils..."
make install
rm -rf $BINUTILS_BUILD_DIR $BINUTILS_SOURCE_DIR

cd $BR_OUTPUT

echo "Building static GCC and G++ cross toolchains..."
GCC_BUILD_DIR="${BR_OUTPUT}/${ARCH}_gcc"
if [[ -d "$GCC_BUILD_DIR" ]]; then
    rm -rf "$GCC_BUILD_DIR"
fi
mkdir $GCC_BUILD_DIR
GCC_VERSION=$(curl -s https://ftp.gnu.org/gnu/gcc/ | grep -oP 'gcc-\d+\.\d+\.\d+/' | sed -E 's|gcc-([0-9.]+)/|\1|' | sort -V | tail -n1)
echo "Identified GCC LTS version: $GCC_VERSION. Downloading source code..."
GCC_SOURCE_DIR="${BR_OUTPUT}/gcc-${GCC_VERSION}"
if [[ -d "$GCC_SOURCE_DIR" ]]; then
    rm -rf "$GCC_SOURCE_DIR"
fi
curl -LO "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"
tar -xf "gcc-${GCC_VERSION}.tar.xz"
rm "gcc-${GCC_VERSION}.tar.xz"
cd "$GCC_BUILD_DIR"
echo "Configuring GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" $GCC_SOURCE_DIR/configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=${ARCH}-linux-gnu --prefix="$TOOLCHAINS_DIR" --with-build-sysroot="$ARCH_ROOTFS" --enable-checking=release --enable-languages=c,c++ --program-suffix=-${GCC_VERSION} --without-headers --disable-hosted-libstdcxx
echo "Building GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make -j$(($(nproc)-1)) all-gcc
echo "Deploying GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make install-gcc
echo "Creating GCC/G++ scripts..."
echo -e '#!/bin/bash\nDIR="$(dirname "$0")"\n$DIR/gcc-'${GCC_VERSION}' --sysroot="$DIR/../../'"${ARCH}_rootfs"'" -I"$DIR/../../'"${ARCH}_rootfs/usr/include"'" "$@"' > "$TOOLCHAINS_DIR/bin/gcc"
echo -e '#!/bin/bash\nDIR="$(dirname "$0")"\n$DIR/g++-'${GCC_VERSION}' --sysroot="$DIR/../../'"${ARCH}_rootfs"'" -I"$DIR/../../'"${ARCH}_rootfs/usr/include"'" "$@"' > "$TOOLCHAINS_DIR/bin/g++"
chmod +x "$TOOLCHAINS_DIR/bin/gcc"
chmod +x "$TOOLCHAINS_DIR/bin/g++"
rm -rf $GCC_BUILD_DIR $GCC_SOURCE_DIR

ROOTFS_BUILD_DIR="${PROJECT_ROOT}/rootfs/build/${ARCH}"
echo "Moving ${ARCH}_rootfs and ${ARCH}_toolchains to Arion's rootfs/build/${ARCH} directory..."
if [[ ! -d "$ROOTFS_BUILD_DIR" ]]; then
    mkdir -p "$ROOTFS_BUILD_DIR"
fi
mv "$ARCH_ROOTFS" "$ROOTFS_BUILD_DIR/"
mv "$TOOLCHAINS_DIR" "$ROOTFS_BUILD_DIR/"

echo "Cleaning up Buildroot output..."
rm -rf $BR_OUTPUT
echo "Build completed: $ROOTFS_BUILD_DIR"
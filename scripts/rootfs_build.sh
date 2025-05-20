#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SUPPORTED_ARCHS=("x86" "x86-64" "arm" "arm64")

ARCH=$1

if [[ -z "$ARCH" ]]; then
    echo "Usage: $0 <arch_name|all>"
    exit 1
fi

if [[ "$ARCH" == "all" ]]; then
    declare -a pids=()

    for TARGET_ARCH in "${SUPPORTED_ARCHS[@]}"; do
        "$0" "$TARGET_ARCH" &
        pids+=($!)
    done

    for pid in "${pids[@]}"; do
        wait $pid || echo "Build with PID $pid failed"
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

echo "Building $ARCH rootfs..."

ARCH_DIR="$PROJECT_ROOT/rootfs/$ARCH"
if [[ -d "$ARCH_DIR" ]]; then
    rm -rf "$ARCH_DIR"
fi
mkdir -p "$ARCH_DIR"

ROOTFS_DIR="$ARCH_DIR/rootfs"
if [[ -d "$ROOTFS_DIR" ]]; then
    rm -rf "$ROOTFS_DIR"
fi
mkdir -p "$ROOTFS_DIR"

TOOLCHAINS_DIR="$ARCH_DIR/toolchains"
if [[ -d "$TOOLCHAINS_DIR" ]]; then
    rm -rf "$TOOLCHAINS_DIR"
fi
mkdir -p "$TOOLCHAINS_DIR"

case "$ARCH" in
  x86)
    GNU_ARCH="i686-linux-gnu"
    ;;
  x86-64)
    GNU_ARCH="x86_64-linux-gnu"
    ;;
  arm)
    GNU_ARCH="arm-linux-gnueabi"
    ;;
  arm64)
    GNU_ARCH="aarch64-linux-gnu"
    ;;
  *)
    echo "Unsupported architecture: $ARCH"
    exit 1
    ;;
esac

mkdir -p "${ROOTFS_DIR}"/{bin,sbin,etc,lib,lib64,proc,sys,dev,tmp,usr/{bin,sbin,lib,include},var,root,home}

cd "$ARCH_DIR"

echo "Building static Binutils cross toolchains..."
BINUTILS_BUILD_DIR="$ARCH_DIR/binutils-build"
if [[ -d "$BINUTILS_BUILD_DIR" ]]; then
    rm -rf "$BINUTILS_BUILD_DIR"
fi
mkdir -p "$BINUTILS_BUILD_DIR"
BINUTILS_VERSION=$(curl -s https://ftp.gnu.org/gnu/binutils/ | grep -oP 'binutils-\d+\.\d+\.tar\.xz' | sed -E 's|binutils-([0-9.]+)\.tar\.xz|\1|' | sort -V | tail -n1)
BINUTILS_VERSION="2.44"
echo "Identified Binutils LTS version: $BINUTILS_VERSION. Downloading source code..."
BINUTILS_SOURCE_DIR="$ARCH_DIR/binutils-${BINUTILS_VERSION}"
if [[ -d "$BINUTILS_SOURCE_DIR" ]]; then
    rm -rf "$BINUTILS_SOURCE_DIR"
fi
curl -LO "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz"
tar -xf "binutils-${BINUTILS_VERSION}.tar.xz"
rm "binutils-${BINUTILS_VERSION}.tar.xz"
cd "$BINUTILS_BUILD_DIR"
echo "Configuring Binutils..."
$BINUTILS_SOURCE_DIR/configure -v --build=x86_64-build-linux-gnu --host=x86_64-build-linux-gnu --target=${GNU_ARCH} --prefix="$TOOLCHAINS_DIR" --disable-nls
make configure-host
echo "Building Binutils..."
make -j$(($(nproc)-1)) LDFLAGS="-all-static"
echo "Deploying Binutils..."
make install
rm -rf $BINUTILS_BUILD_DIR $BINUTILS_SOURCE_DIR

cd "$ARCH_DIR"

echo "[STAGE 1] Building static GCC and G++ cross toolchains..."
GCC_BUILD_DIR="$ARCH_DIR/gcc-build"
if [[ -d "$GCC_BUILD_DIR" ]]; then
    rm -rf "$GCC_BUILD_DIR"
fi
mkdir -p "$GCC_BUILD_DIR"
GCC_VERSION="15.1.0"
GCC_VERSION=$(curl -s https://ftp.gnu.org/gnu/gcc/ | grep -oP 'gcc-\d+\.\d+\.\d+/' | sed -E 's|gcc-([0-9.]+)/|\1|' | sort -V | tail -n1)
echo "Identified GCC LTS version: $GCC_VERSION. Downloading source code..."
GCC_SOURCE_DIR="$ARCH_DIR/gcc-${GCC_VERSION}"
if [[ -d "$GCC_SOURCE_DIR" ]]; then
    rm -rf "$GCC_SOURCE_DIR"
fi
curl -LO "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"
tar -xf "gcc-${GCC_VERSION}.tar.xz"
rm "gcc-${GCC_VERSION}.tar.xz"
cd "$GCC_SOURCE_DIR"
echo "[STAGE 1] Downloading GCC dependencies..."
./contrib/download_prerequisites
cd "$GCC_BUILD_DIR"
echo "[STAGE 1] Configuring GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" "$GCC_SOURCE_DIR/configure" -v --build=x86_64-build-linux-gnu --host=x86_64-build-linux-gnu --target=${GNU_ARCH} --prefix="$TOOLCHAINS_DIR" --enable-checking=release --enable-languages=c --without-headers --disable-shared --disable-threads --disable-multilib --disable-libatomic --disable-libgomp --disable-libquadmath --disable-libssp --disable-nls
echo "[STAGE 1] Building GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make -j$(($(nproc)-1)) all-gcc
echo "[STAGE 1] Deploying GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make install-gcc
rm -rf *
echo "[STAGE 1] Configuring libgcc..."
"$GCC_SOURCE_DIR/configure" -v --build=x86_64-build-linux-gnu --host=x86_64-build-linux-gnu --target=${GNU_ARCH} --prefix="$TOOLCHAINS_DIR" --enable-checking=release --enable-languages=c --without-headers --disable-threads --disable-multilib --disable-libatomic --disable-libgomp --disable-shared --disable-libquadmath --disable-libssp --disable-nls
echo "[STAGE 1] Building libgcc..."
make -j$(($(nproc)-1)) all-target-libgcc
echo "[STAGE 1] Deploying libgcc..."
make install-target-libgcc

cd "$ARCH_DIR"

KERNEL_DIR_VERSION=$(curl -s https://cdn.kernel.org/pub/linux/kernel/ | grep -oP 'v\d+\.x' | sort -V | tail -n1)
KERNEL_VERSION=$(curl -s https://cdn.kernel.org/pub/linux/kernel/${KERNEL_DIR_VERSION}/ | grep -oP '\d+\.\d+\.*\d*\.tar\.xz' | sed 's/^linux-//' | sed 's/\.tar\.xz$//' | sort -V | tail -n1)
echo "Identified Linux kernel LTS version: $KERNEL_VERSION. Downloading source code..."
curl -LO https://cdn.kernel.org/pub/linux/kernel/${KERNEL_DIR_VERSION}/linux-${KERNEL_VERSION}.tar.xz
tar -xf "linux-${KERNEL_VERSION}.tar.xz"
rm "linux-${KERNEL_VERSION}.tar.xz"
KERNEL_SOURCE_DIR="$ARCH_DIR/linux-${KERNEL_VERSION}"
cd $KERNEL_SOURCE_DIR
echo "Installing kernel headers in rootfs..."
make ARCH=${ARCH//-/_} INSTALL_HDR_PATH=${ROOTFS_DIR}/usr headers_install
rm -rf "$KERNEL_SOURCE_DIR"

cd "$ARCH_DIR"

export PATH="$TOOLCHAINS_DIR/bin:$PATH"

GLIBC_BUILD_DIR="$ARCH_DIR/glibc-build"
if [[ -d "$GLIBC_BUILD_DIR" ]]; then
    rm -rf "$GLIBC_BUILD_DIR"
fi
mkdir -p "$GLIBC_BUILD_DIR"
GLIBC_VERSION="2.41"
GLIBC_VERSION=$(curl -s https://ftp.gnu.org/gnu/libc/ | grep -oP 'glibc-\d+\.\d+\.tar\.xz' | sort -V | tail -n1 | sed -E 's/^glibc-([0-9.]+)\.tar\.xz$/\1/')
echo "Identified glibc LTS version: $GLIBC_VERSION. Downloading source code..."
curl -LO "https://ftp.gnu.org/gnu/libc/glibc-${GLIBC_VERSION}.tar.xz"
tar -xf "glibc-${GLIBC_VERSION}.tar.xz"
rm "glibc-${GLIBC_VERSION}.tar.xz"
GLIBC_SOURCE_DIR="$ARCH_DIR/glibc-${GLIBC_VERSION}"
cd "$GLIBC_BUILD_DIR"
"$GLIBC_SOURCE_DIR/configure" --prefix=/usr --build=x86_64-build-linux-gnu --host=${GNU_ARCH} --target=${GNU_ARCH} --with-headers="$ROOTFS_DIR/usr/include" --without-selinux --disable-multilib --disable-werror --enable-kernel="$KERNEL_VERSION" --disable-sanity-checks BUILD_CC="gcc" CXX= CC="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-gcc" AR="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ar" RANLIB="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ranlib" AS="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-as" LD="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ld" STRIP="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-strip"
make install-bootstrap-headers=yes install-headers install_root=${ROOTFS_DIR} CXX= CC="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-gcc" AR="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ar" RANLIB="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ranlib" AS="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-as" LD="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ld" STRIP="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-strip"
make csu/subdir_lib CXX= CC="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-gcc" AR="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ar" RANLIB="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ranlib" AS="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-as" LD="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ld" STRIP="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-strip"
install csu/crt1.o csu/crti.o csu/crtn.o ${ROOTFS_DIR}/usr/lib
make -j$(($(nproc)-1)) CXX= CC="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-gcc" AR="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ar" RANLIB="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ranlib" AS="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-as" LD="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ld" STRIP="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-strip"
make install install_root=${ROOTFS_DIR} CXX= CC="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-gcc" AR="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ar" RANLIB="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ranlib" AS="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-as" LD="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-ld" STRIP="${TOOLCHAINS_DIR}/bin/${GNU_ARCH}-strip"

rm -rf "$GLIBC_SOURCE_DIR"
rm -rf "$GLIBC_BUILD_DIR"

cd "$GCC_BUILD_DIR"
rm -rf *
echo "[STAGE 2] Configuring GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" "$GCC_SOURCE_DIR/configure" -v --build=x86_64-build-linux-gnu --host=x86_64-build-linux-gnu --target=${GNU_ARCH} --prefix="$TOOLCHAINS_DIR" --with-sysroot="$ROOTFS_DIR" --enable-checking=release --enable-languages=c,c++ --disable-multilib --disable-bootstrap --disable-nls
echo "[STAGE 2] Building GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make -j$(($(nproc)-1)) all-gcc
echo "[STAGE 2] Deploying GCC..."
LDFLAGS_FOR_TARGET="-static" LDFLAGS="-static" make install-gcc
rm -rf *
echo "[STAGE 2] Configuring libgcc/libstdc++..."
"$GCC_SOURCE_DIR/configure" -v --build=x86_64-build-linux-gnu --host=x86_64-build-linux-gnu --target=${GNU_ARCH} --prefix="$TOOLCHAINS_DIR" --with-sysroot="$ROOTFS_DIR" --enable-checking=release --enable-languages=c,c++ --disable-multilib --disable-bootstrap --disable-nls
echo "[STAGE 2] Building libgcc..."
make -j$(($(nproc)-1)) all-target-libgcc
echo "[STAGE 2] Building libstdc++..."
make -j$(($(nproc)-1)) all-target-libstdc++-v3
echo "[STAGE 2] Deploying libgcc..."
make install-target-libgcc
echo "[STAGE 2] Deploying libstdc++..."
make install-target-libstdc++-v3
cp -r "$TOOLCHAINS_DIR/$GNU_ARCH/"* "$ROOTFS_DIR/"
echo "Creating GCC/G++ scripts..."
echo -e '#!/bin/bash\nDIR="$(dirname "$0")"\n$DIR/'${GNU_ARCH}'-gcc --sysroot="$DIR/../../'"rootfs"'" "$@"' > "$TOOLCHAINS_DIR/bin/gcc"
echo -e '#!/bin/bash\nDIR="$(dirname "$0")"\n$DIR/'${GNU_ARCH}'-g++ --sysroot="$DIR/../../'"rootfs"'" "$@"' > "$TOOLCHAINS_DIR/bin/g++"
chmod +x "$TOOLCHAINS_DIR/bin/gcc"
chmod +x "$TOOLCHAINS_DIR/bin/g++"

rm -rf "$GCC_SOURCE_DIR"
rm -rf "$GCC_BUILD_DIR"

echo "Done."

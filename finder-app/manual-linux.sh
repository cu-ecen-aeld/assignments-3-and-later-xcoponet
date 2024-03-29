#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# find cross compiler rootfs
CROSS_COMPILE_NO_DASH=${CROSS_COMPILE%-}
CROSS_COMPILE_ROOTFS=$(dirname  $(which ${CROSS_COMPILE}gcc))/../${CROSS_COMPILE_NO_DASH}/libc/
echo "CROSS_COMPILE_ROOTFS: ${CROSS_COMPILE_ROOTFS}"


if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
    # make OUTDIR an absolute path
    OUTDIR=$(realpath $OUTDIR)
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Building kernel"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
fi

echo "Adding the Image in ${OUTDIR}"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p ${OUTDIR}/rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var}
mkdir -p ${OUTDIR}/rootfs/usr/{bin,sbin,lib}
mkdir -p ${OUTDIR}/rootfs/var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"


# Add library dependencies to rootfs
# Extract the library names from readelf output
LIBRARIES=$(readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library" | awk -F 'Shared library: ' '{print $2}')
echo "libraries: ${LIBRARIES}"
for LIBRARY in $LIBRARIES
do
    # Extract the library filename from the path, removing brackets
    LIBRARY_NAME=$(basename $LIBRARY | tr -d '[]')
    echo "library name: ${LIBRARY_NAME}"
    
    # Copy the library to the destination directory
    cp -v $CROSS_COMPILE_ROOTFS/lib64/$LIBRARY_NAME $OUTDIR/rootfs/lib64/$LIBRARY_NAME
done

# Copy the dynamic linker to the rootfs
DYNAMIC_LINKER=$(readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | awk -F 'program interpreter: ' '{print $2}' | tr -d '[]')
cp -v $CROSS_COMPILE_ROOTFS/$DYNAMIC_LINKER $OUTDIR/rootfs/$DYNAMIC_LINKER


# Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/Console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -r ${FINDER_APP_DIR}/{finder.sh,finder-test.sh,autorun-qemu.sh} ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/home/conf
cp -r ${FINDER_APP_DIR}/conf/{username.txt,assignment.txt} ${OUTDIR}/rootfs/home/conf


# Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs

# Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

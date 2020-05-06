#!/bin/bash

make odroidn1_defconfig

make

tools/loaderimage --pack --uboot ./u-boot-dtb.bin uboot.img

tools/mkimage -n rk3399 -T rksd -d ../rkbin/bin/rk33/rk3399_ddr_800MHz_v1.15.bin idbloader.img
cat ../rkbin/bin/rk33/rk3399_miniloader_v1.15.bin >> idbloader.img

cp ../rkbin/bin/rk33/rk3399_loader_v1.12.112.bin sd_fuse

tools/trust_merger ${BIN_PATH_FIXUP} tools/rk_tools/trust.ini

cp idbloader.img sd_fuse
cp uboot.img sd_fuse
cp trust.img sd_fuse


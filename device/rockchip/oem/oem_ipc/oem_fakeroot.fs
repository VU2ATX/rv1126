#!/bin/sh
set -e
[ -d /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/output/rockchip_rv1126_rv1109_spi_nand/oem/www ] && chown -R www-data:www-data /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/output/rockchip_rv1126_rv1109_spi_nand/oem/www
[ -d /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/output/rockchip_rv1126_rv1109_spi_nand/oem/usr/www ] && chown -R www-data:www-data /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/output/rockchip_rv1126_rv1109_spi_nand/oem/usr/www
mkdir -p /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/../rockdev
/home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/../device/rockchip/common/mk-image.sh /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/output/rockchip_rv1126_rv1109_spi_nand/oem /home/lwx/workspace/sdk/rv1126_rv1109_linux_sdk_v1.2.0_20200807/buildroot/../rockdev/oem.img ubi 0x6400000 oem 2048 0x20000

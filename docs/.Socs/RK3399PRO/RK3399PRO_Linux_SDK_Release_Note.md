# RK3399PRO Linux SDK Release Note

---

**Versions**

[TOC]

---

## rk3399pro_linux_release_v1.4.0_20201010.xml Release Note

**NPU**:

	- NPU fw upgrade to v1.4.0
	- NPU Transfer Server: 2.0.1
	- Update rknn_server to 1.4.0
	- Update librknn_runtime to 1.4.0
	  ChangeLog:
	    1) Support get output tensor name (rknn_query)
	    2) Support rknn model with multiple std/scale
	    3) Refine rknn_inputs_set()
	    4) Support mish op (for yolov4)
	    5) Refine deconvolution

**libmali**:

	- Upgrade Midgard DDK to r18p0-01rel0

**Kernel (4.4)**:

	- Upgrade mali t86x to r18
	- Supoort legacy api to set property

**docs/tools**:

	- Update recovery/Graphics documents
	- Add deviceIo Bluetooth document
	- Add mpp/weston/chromecast/debian10 document
	- Update Kernel/Linux/AVL/Socs/Others documents
	- Update Linux_Upgrade_Tool: update from v1.38 to v1.49
	- Secureboottool: update to v1.95
	- SecurityAVB: update to v2.7
	- Upgrade SDDiskTool to v1.62
	- Rename AndroidTool to RKDevTool, and version upgrade to v2.73
	- Update DDR/ NAND/ eMMC AVL
	- Remove outdated documents
	- Windows: add tool for modifying parameter
	- Add RMSL developer guide

**Yocto**:

	- Upgrade to 3.0.3

**Debian9**:

	- Update npu fw to v1.4.0
	- Update libmali/ffmpeg/mpp/mpv/xserver to fix some bugs
	- Update xserver to fix some bugs
	- Update gst-rkmpp to fix the mppvideodec

**Debian10**:

	- Support debian10 for rk3399pro firstly

**Buildroot (2018.02-rc3)**:

	- Support ntfs for recovery
	- Upgrade mali to r18
	- Support Rockchip's RMSL
	- Support QT 5.14.2 version
	- Update weston to v8.0.0
	- Update chromium-wayland to 85.0.4183.102

## rk3399pro_linux_release_v1.3.0_20200324.xml Release Note

**NPU**:

	- update rknn_server to 1.3.1 (6ebb4d7)
	- NPU Transfer Server: 1.9.8 (40e4a8a@2020-01-02T09:16:20)
	- update librknn_runtime to 1.3.1 (7c5d3d8)
	  ChangeLog:
		1) Maxpool OP: support stride 1,2,1,1
		2) Deconvolution OP: support MxN weight
		3) Resize bilinear OP: support 'align_corners'
		4) Solve the problem of low precision of dynamic_fixed_point-i8/i16 for some models
		5) Support custom OP generated by RKNN Toolkit 1.3.0

**Kernel (4.4)**:

	- Fixes the s2r for rk817
	- Fixes the camera for rk3399pro-evb-v14-linux
	- Fixes the debian s2r issue on system_monitor

**Buildroot (2018.02-rc3)**:

	- Fixes random crash during hot plugging monitors
	- Remove npu_fw_pvtm-32k for rk3399pro v14 npu firmware
	- Fixes npu_powerctrl_combine for rk3399pro v14
	- Support rockchip_rk3399pro_combine_defconfig for rkk3399pro evb v14 board
	- Add npu_fw_pvtm-32k for rk3399pro v14 npu firmware
	- Add npu_powerctrl_combine for rk3399pro v14

**Debian9 (stretch-9.12)**:

	- Update ffmpeg/gstreamer/xserver/libdrm to support xvimagesink and mpv
	- Update test_dec-gst.sh and test_dec-mpv.sh for tests
	- Update npu image to v1.3.1 for rk3399pro v14
	- Update ffmpeg to v4.1.4 for rga issue
	- Support parole and mpv for hardware acceleration witth video
	- Add modetest and mpv for tests
	- Update mpp

**docs**:

	- Add release_patch for every Socs
	- add deian10 guide v1.0.0
	- add mpp/weston/chromecast documents
	- update v1.2.2 documents for rk3399pro

## rk3399pro_linux_release_v1.2.0_20191224.xml Release Note

**Buildroot (2018.02-rc3)**:

	- Update xserver: fix wrong rga format map and fix random crash
	- Fixes some weston render issues
	- Support rockchip RGA 2D accel
	- The logs output on br.log
	- Support Camera N4 on rk3399pro evb v13 board
	- Support ntfs filesystem for linux
	- Support controlling output's display rectangle for weston
	- Support the freerdp with X11
	- Upgrade QT verison from 5.9.4 to 5.12.2
	- Support Chromium Browser (74.0.3729.157)
	- Upgrade Xserver-xorg to v1.20.4
	- change the new qt app to replace the old apps
		new apps: qcamera  qfm  QLauncher+  qplayer  qsetting
		old apps: camera gallery  music  QLauncher  settings  video
	- Add the missing license/copyright with legal-info
	- Support x11 packages
	- Support weston rotate and scale
	- Upgrade camera_engine_rksip
		librkisp: v2.2.0
		3A lib version:
		af:  v0.2.17
		wb: v0.0.e
		aec: v0.0.e
		iq version: v1.5.0
		iq magic version code: 706729
		matched rkisp1 driver version: v0.1.5
		- Upgrade Linux NN to v1.3

**Kernel (4.4)**:

	- Upgrade to 4.4.194
		fixes sd card on rk3399pro evb board
		update rk3399pro evb v13 dts for linux
		enable iep for rk3399pro evb linux
		Support Camera N4 on rk3399pro evb v13 board
		Put the dp in vopb for rk3399pro evb
		In order to more stable, increase the minimum voltage to from 800mv to 825mv
		Fixes the HDMI status in resume
		Add the rk3399 lpddr4 dts for reference
		Fixes nvme/p-cie interface sdd
		camera stuff update...etc

**rkbin (Rockchip binary)**:

	- npu: lion: ddr: update to v1.04 20191121
          Update feature:
		1) update cpu and npu's priorty to 1, vop's priority to 3 and ohter master's priority to 2
		2) agingx0 set to 0x4, aging0 set to 0x33, aging1,2 set to 0xff.
	- rknpu_lion: bl31: update version to v1.10
	  update feature:
		8affafbd4 plat: rk1808: monitor: modify some flow
		a5a179cee plat: rk1808: suspend: fix logoff mode flow.
		d26bf38ea plat: rk1808: support monitor
		0b93630e1 rockchip: sip: add weak attribute to sip_version_handler
		76bf9e6f5 plat: rk1808: add efuse driver
		3c788026d plat: rk1808: add pll driver
		1a5b97455 plat: rk1808: change bl31_base to 0x40000
	- npu: lion: bl32: update version to v1.12
	  Update feature:
		9a5e8974 scripts: add checkbuild.sh to build all platforms
		9ae3c85f scripts: refine build scripts
		991e5c92 drivers: crypto_v2: modify des/aes finish condition

**docs/tools**:

	- Update AVL
	- Update Soc_public
	- Secureboottool: update to v1.95
	- Add Docker document
	- Update PWM document
	- Remove internal docs
	- Add Rockchip SDK Kit document
	- Update avb tool to v2.6
	- Add window/linux secure sign tool
	- Add DM tools
	- Upgrade AndroidTool from v2.67 to v2.69, support for ubifs
	- Update rk_provision_tool to RKDevInfoWriteTool_V1.0.4
	  V1.0.4:
		1.add two custom id
		2.the rk_provision_tool rename to RKDevInfoWriteTool
		- upgrade SDDiskTool to v1.59

**Debian9 (stretch-9.11)**:

	- Update xserver
	- Fixes jpeg's decode to 60fps
	- Update test_camera for uvc
	- Update mpp
	- Update rga
	- Support Camera N4 on rk3399pro evb v13 board
	    - support exa/glamor hw acceleration on xserver
	- Update camera_engine_rksip to v2.2.0
	- Add LICENSE.txt
	- QT upgraded to v5.11
	- fixes system suspend/resume for rk3399pro Socs
	- Add glmark2 normal mode
	- Add video hardware acceleration for chromium

**Yocto (thud 2.6.2)**:

	- Add rockchip-rkisp
	- Chromium-ozone-wayland: Support 78.0.3904.97g
	- Support adding extra volumes
	- Support Camera N4 on rk3399pro evb v13 board
	- Add video hardware acceleration for chromium
	- Gstreamer-rockchip: Update source and patches
	- Gstreamer1.0-plugins-base: xvimagesink: Support dma buffer rendering
	- Xserver-xorg: glamor: Update patches

**NPU**:

	- rk1808 base on kernel commit id:
	   d81e5390d0921d7fb019f3a45ab08d5e0e1d2fb0
	rk1808 fedora:
	   b75e99f54361dd71fe7896919076692ae97b7585
	rk3399pro/rk3399pro pcie:
	   577aa02a6d309d0697db079c673baf0b815f5d53
	- update rknn_server to 1.3.0 (c4f8c233)
	- update librknn_runtime to 1.3.0 (0e1d4ea6)
	  New features:
		1) Add multi-channels-mean support
		2) New op: argmax argmin gru grucell log_softma
	- Support RKNPU_FW_N4 for pcie on rk3399pro evb v13 board

## rk3399pro_linux_release_v1.1.0_20190706.xml Release Note

**device/rockchip**:

	- Support to run './build.sh debian' for debian9 and run './build.sh distro' for debian10
	- Fix broken cleanall with './build.sh cleanall'

**Kernel**:

	- Fix host mode resume fail for rk3399/rk33999pro
	- Put the edp/mipi in vopl, and hdmi in vopb
	- Fixes irq of BT on rk3399pro evb
	- Fixes the regulator the abnormal during suspend

**Uboot**:

	- Update vopl init correct
	- Fixes avb

**Debian9**:

	- Fixes vp8 encode was abnormal on debian
	- Support usb3.0 for adbd
	- Support rk809 switch configure
	- Fixes the online video crash on chromium
	- Support the camera 3a function on debian

**docs**:

	- Add the Rockchip Linux Software_Developer_Guide_CN and
	- Rockchip_Linux_Software_Developer_Guide_EN.pdf
	- Update RK3399PRO_Release_Note.txt
		* dos2unix fixes the file format on ubuntu
		* change the language from Chinese to English

## rk3399pro_linux_release_v1.0.0_20190606.xml Release Note

	- The first release version
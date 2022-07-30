# PX30 Linux4.19 SDK Release Note

---

**Versions**

[TOC]

---
## px30_linux4.19_release_v1.2.0_20220620.xml Release Note

**Buildroot(2018.02-rc3)**

```
- Fix sd upgrade and boot issue with SDDiskTool
- Support Vulkan and vkmark
- Switch weston to dispaly as launcher
- Update weston to fix some issues
- Upgrade wayland to buildroot upstream's 1.20.0
- Upgrade gstreamer1 to buildroot upstream's 1.20.0
- Upgrade glibc to buildroot upstream's 2.35
- Upgrade chromium-wayland to 101.0.4951.54
```

**Debian**

```
- Update the font to chinese by default
- Update mirrors.ustc.edu.cn for source.list
- Update mpp/gst-rkmpp/xserver
- Update rockchip-test
- Update gstreamer to fix the format issues
- Add rktoolkit and partition init for recovery
- Reduce the rootfs size
```

**Yocto**

```
- Update Yocto to 3.4.1
    - Support Chromium to 101.0.4951
    - Bump xserver/v4l-utils/v4l-mpp/gst-mpp/mpp
    - Update rkwifibt-firmware and rockchip-libmali
```

**Kernel**

```
- Add support for PX30-S/RK3326-S/RK3358
- Add SCMI for PX30-S/RK3326-S
- Add Mali quick reset support for PX30-S/RK3326-S
- Add RK817/RK809 support for PX30-S/RK3326-S
```

**rkbin**

```
- Update px30/rk3326 ddr bin to V2.07 20220531
- Update px30 bl31 version to v1.31
- Update rk3326 bl31 version to v1.31
- Update px30 bl32 version to v2.12
- Update rk3326 bl32 version to v2.12
- Update rk3358 bl32 version to v2.03
```

**u-boot**

```
- Add RK817/RK809 support for PX30-S/RK3326-S
```

**external**

```
- Add slt_gpu_light
    - Add slt_gpu_light lib for PX30-S/RK3326-S
```

## px30_linux4.19_release_v1.0.0_20210520.xml Release Note

```
- Initial release version
```
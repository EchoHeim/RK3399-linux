#include <drm_fourcc.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bo.h"
#include "dev.h"
#include "drmDsp.h"
#include "modeset.h"

struct drmDsp {
  struct fb_var_screeninfo vinfo;
  unsigned long screensize;
  char* fbp;
  struct sp_dev* dev;
  struct sp_plane** plane;
  struct sp_crtc* test_crtc;
  struct sp_plane* test_plane;
  int num_test_planes;
  struct sp_bo* bo[2];
  struct sp_bo* nextbo;
} gDrmDsp;

int initDrmDsp() {
  int ret = 0, i = 0;
  struct drmDsp* pDrmDsp = &gDrmDsp;

  memset(pDrmDsp, 0, sizeof(struct drmDsp));

  pDrmDsp->dev = create_sp_dev();
  if (!pDrmDsp->dev) {
    fprintf(stderr, "Failed to create sp_dev\n");
    return -1;
  }

  ret = initialize_screens(pDrmDsp->dev);
  if (ret) {
    fprintf(stderr, "Failed to initialize screens\n");
    return ret;
  }
  pDrmDsp->plane = (struct sp_plane**)calloc(pDrmDsp->dev->num_planes,
                                             sizeof(struct sp_plane*));
  if (!pDrmDsp->plane) {
    fprintf(stderr, "Failed to allocate plane array\n");
    return -1;
  }

  pDrmDsp->test_crtc = &pDrmDsp->dev->crtcs[0];
  pDrmDsp->num_test_planes = pDrmDsp->test_crtc->num_planes;
  for (i = 0; i < pDrmDsp->test_crtc->num_planes; i++) {
    pDrmDsp->plane[i] = get_sp_plane(pDrmDsp->dev, pDrmDsp->test_crtc);
    if (is_supported_format(pDrmDsp->plane[i], DRM_FORMAT_NV12))
      pDrmDsp->test_plane = pDrmDsp->plane[i];
  }
  if (!pDrmDsp->test_plane) {
    fprintf(stderr, "pDrmDsp->test_plane is null\n");
    return -1;
  } else {
    fprintf(stderr, "pDrmDsp->test_plane is no null\n");
    return 0;
  }
}

void deInitDrmDsp() {
  struct drmDsp* pDrmDsp = &gDrmDsp;
  if (pDrmDsp->plane) free(pDrmDsp->plane);
  if (pDrmDsp->bo[0]) {
    drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->bo[0]->fb_id);
    free_sp_bo(pDrmDsp->bo[0]);
  }
  if (pDrmDsp->bo[1]) {
    drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->bo[1]->fb_id);
    free_sp_bo(pDrmDsp->bo[1]);
  }
  destroy_sp_dev(pDrmDsp->dev);
  memset(pDrmDsp, 0, sizeof(struct drmDsp));
}

static int arm_camera_yuv420_scale_arm(char* srcbuf, char* dstbuf, int src_w,
                                       int src_h, int dst_w, int dst_h) {
  unsigned char *psY, *pdY, *psUV, *pdUV;
  unsigned char *src, *dst;
  int srcW, srcH, cropW, cropH, dstW, dstH;
  long zoomindstxIntInv, zoomindstyIntInv;
  long x, y;
  long yCoeff00, yCoeff01, xCoeff00, xCoeff01;
  long sX, sY;
  long r0, r1, a, b, c, d;
  int ret = 0;
  int nv21DstFmt = 0, mirror = 0;
  int ratio = 0;
  int top_offset = 0, left_offset = 0;

  // need crop ?
  if ((src_w * 100 / src_h) != (dst_w * 100 / dst_h)) {
    ratio = ((src_w * 100 / dst_w) >= (src_h * 100 / dst_h))
                ? (src_h * 100 / dst_h)
                : (src_w * 100 / dst_w);
    cropW = ratio * dst_w / 100;
    cropH = ratio * dst_h / 100;

    left_offset = ((src_w - cropW) >> 1) & (~0x01);
    top_offset = ((src_h - cropH) >> 1) & (~0x01);
  } else {
    cropW = src_w;
    cropH = src_h;
    top_offset = 0;
    left_offset = 0;
  }

  src = psY = (unsigned char*)(srcbuf) + top_offset * src_w + left_offset;
  // psUV = psY +src_w*src_h+top_offset*src_w/2+left_offset;
  psUV = (unsigned char*)(srcbuf) + src_w * src_h + top_offset * src_w / 2 +
         left_offset;

  srcW = src_w;
  srcH = src_h;
  //	cropW = src_w;
  //	cropH = src_h;

  dst = pdY = (unsigned char*)dstbuf;
  pdUV = pdY + dst_w * dst_h;
  dstW = dst_w;
  dstH = dst_h;

  zoomindstxIntInv = ((unsigned long)(cropW) << 16) / dstW + 1;
  zoomindstyIntInv = ((unsigned long)(cropH) << 16) / dstH + 1;
  // y
  // for(y = 0; y<dstH - 1 ; y++ ) {
  for (y = 0; y < dstH; y++) {
    yCoeff00 = (y * zoomindstyIntInv) & 0xffff;
    yCoeff01 = 0xffff - yCoeff00;
    sY = (y * zoomindstyIntInv >> 16);
    sY = (sY >= srcH - 1) ? (srcH - 2) : sY;
    for (x = 0; x < dstW; x++) {
      xCoeff00 = (x * zoomindstxIntInv) & 0xffff;
      xCoeff01 = 0xffff - xCoeff00;
      sX = (x * zoomindstxIntInv >> 16);
      sX = (sX >= srcW - 1) ? (srcW - 2) : sX;
      a = psY[sY * srcW + sX];
      b = psY[sY * srcW + sX + 1];
      c = psY[(sY + 1) * srcW + sX];
      d = psY[(sY + 1) * srcW + sX + 1];

      r0 = (a * xCoeff01 + b * xCoeff00) >> 16;
      r1 = (c * xCoeff01 + d * xCoeff00) >> 16;
      r0 = (r0 * yCoeff01 + r1 * yCoeff00) >> 16;

      if (mirror)
        pdY[dstW - 1 - x] = r0;
      else
        pdY[x] = r0;
    }
    pdY += dstW;
  }

  dstW /= 2;
  dstH /= 2;
  srcW /= 2;
  srcH /= 2;

  // UV
  // for(y = 0; y<dstH - 1 ; y++ ) {
  for (y = 0; y < dstH; y++) {
    yCoeff00 = (y * zoomindstyIntInv) & 0xffff;
    yCoeff01 = 0xffff - yCoeff00;
    sY = (y * zoomindstyIntInv >> 16);
    sY = (sY >= srcH - 1) ? (srcH - 2) : sY;
    for (x = 0; x < dstW; x++) {
      xCoeff00 = (x * zoomindstxIntInv) & 0xffff;
      xCoeff01 = 0xffff - xCoeff00;
      sX = (x * zoomindstxIntInv >> 16);
      sX = (sX >= srcW - 1) ? (srcW - 2) : sX;
      // U
      a = psUV[(sY * srcW + sX) * 2];
      b = psUV[(sY * srcW + sX + 1) * 2];
      c = psUV[((sY + 1) * srcW + sX) * 2];
      d = psUV[((sY + 1) * srcW + sX + 1) * 2];

      r0 = (a * xCoeff01 + b * xCoeff00) >> 16;
      r1 = (c * xCoeff01 + d * xCoeff00) >> 16;
      r0 = (r0 * yCoeff01 + r1 * yCoeff00) >> 16;

      if (mirror && nv21DstFmt)
        pdUV[dstW * 2 - 1 - (x * 2)] = r0;
      else if (mirror)
        pdUV[dstW * 2 - 1 - (x * 2 + 1)] = r0;
      else if (nv21DstFmt)
        pdUV[x * 2 + 1] = r0;
      else
        pdUV[x * 2] = r0;
      // V
      a = psUV[(sY * srcW + sX) * 2 + 1];
      b = psUV[(sY * srcW + sX + 1) * 2 + 1];
      c = psUV[((sY + 1) * srcW + sX) * 2 + 1];
      d = psUV[((sY + 1) * srcW + sX + 1) * 2 + 1];

      r0 = (a * xCoeff01 + b * xCoeff00) >> 16;
      r1 = (c * xCoeff01 + d * xCoeff00) >> 16;
      r0 = (r0 * yCoeff01 + r1 * yCoeff00) >> 16;

      if (mirror && nv21DstFmt)
        pdUV[dstW * 2 - 1 - (x * 2 + 1)] = r0;
      else if (mirror)
        pdUV[dstW * 2 - 1 - (x * 2)] = r0;
      else if (nv21DstFmt)
        pdUV[x * 2] = r0;
      else
        pdUV[x * 2 + 1] = r0;
    }
    pdUV += dstW * 2;
  }
  return 0;
}

int drmDspFrame(int width, int height, void* dmaFd, int fmt) {
  int ret;
  struct drm_mode_create_dumb cd;
  struct sp_bo* bo;
  struct drmDsp* pDrmDsp = &gDrmDsp;

  int ori_width = width;
  int ori_height = height;

  if (width > 1920) {
    width = 1920;
    height = 1080;
  }

  int wAlign16 = ((width + 15) & (~15));
  int hAlign16 = ((height + 15) & (~15));
  int frameSize = wAlign16 * hAlign16 * 3 / 2;
  uint32_t handles[4], pitches[4], offsets[4];

  if (DRM_FORMAT_NV12 != fmt) {
    fprintf(stderr, "%s just support NV12 to display\n", __func__);
    return -1;
  }
// create bo
#if 1
  if (!pDrmDsp->bo[0]) {
    fprintf(stderr, "%s:bo widthxheight:%dx%d\n", __func__, wAlign16, hAlign16);
    pDrmDsp->bo[0] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16, 16, 32,
                                  DRM_FORMAT_NV12, 0);
    pDrmDsp->bo[1] = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16, 16, 32,
                                  DRM_FORMAT_NV12, 0);
    if (!pDrmDsp->bo[0] || !pDrmDsp->bo[1]) {
      fprintf(stderr, "%s:create bo failed ! \n", __func__);
      return -1;
    }
    pDrmDsp->nextbo = pDrmDsp->bo[0];
  }

  if (!pDrmDsp->nextbo) {
    fprintf(stderr, "%s:no available bo ! \n", __func__);
    return -1;
  }

  bo = pDrmDsp->nextbo;
#else
  bo = create_sp_bo(pDrmDsp->dev, wAlign16, hAlign16, 16, 32, DRM_FORMAT_NV12,
                    0);
  if (!bo) fprintf(stderr, "%s:create bo failed ! \n", __func__);
#endif

  handles[0] = bo->handle;
  pitches[0] = wAlign16;
  offsets[0] = 0;
  handles[1] = bo->handle;
  pitches[1] = wAlign16;
  offsets[1] = width * height;  // wAlign16 * hAlign16;
  // copy src data to bo
  if (ori_width == width)
    memcpy(bo->map_addr, dmaFd, wAlign16 * hAlign16 * 3 / 2);
  else
    arm_camera_yuv420_scale_arm(dmaFd, bo->map_addr, ori_width, ori_height,
                                width, height);
  if (bo->fb_id <= 0) {
    ret = drmModeAddFB2(bo->dev->fd, bo->width, bo->height, bo->format, handles,
                        pitches, offsets, &bo->fb_id, bo->flags);
    if (ret) {
      fprintf(stderr, "%s:failed to create fb ret=%d\n", __func__, ret);
      fprintf(stderr,
          "fd:%d "
          ",wxh:%ux%u,format:%u,handles:%u,%u,pictches:%u,%u,offsets:%u,%u,fb_id:"
          "%u,flags:%u \n",
          bo->dev->fd, bo->width, bo->height, bo->format, handles[0], handles[1],
          pitches[0], pitches[1], offsets[0], offsets[1], bo->fb_id, bo->flags);
      return ret;
    } else {
      fprintf(stderr, "%s:create fb success fb_id:%u\n", __func__, bo->fb_id);
    }
  }

  ret = drmModeSetPlane(pDrmDsp->dev->fd, pDrmDsp->test_plane->plane->plane_id,
                        pDrmDsp->test_crtc->crtc->crtc_id, bo->fb_id, 0, 0, 0,
                        // pDrmDsp->test_crtc->crtc->mode.hdisplay,
                        wAlign16, hAlign16,
                        // pDrmDsp->test_crtc->crtc->mode.vdisplay,
                        0, 0, width << 16, height << 16);
  if (ret) {
    fprintf(stderr, "failed to set plane to crtc ret=%d\n", ret);
    return ret;
  }
// free_sp_bo(bo);
#if 0
  if (pDrmDsp->test_plane->bo) {
    if (pDrmDsp->test_plane->bo->fb_id) {
      ret = drmModeRmFB(pDrmDsp->dev->fd, pDrmDsp->test_plane->bo->fb_id);
      if (ret)
        fprintf(stderr, "Failed to rmfb ret=%d!\n", ret);
    }
    if (pDrmDsp->test_plane->bo->handle) {
      struct drm_gem_close req = {
        .handle = pDrmDsp->test_plane->bo->handle,
      };

      drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
      fprintf(stderr, "%s:close bo success!\n", __func__);
    }

    if (!pDrmDsp->nextbo)
      free_sp_bo(pDrmDsp->test_plane->bo);
  }
  pDrmDsp->test_plane->bo = bo; //last po
#endif
#if 1
  // switch bo
  if (pDrmDsp->nextbo == pDrmDsp->bo[0])
    pDrmDsp->nextbo = pDrmDsp->bo[1];
  else
    pDrmDsp->nextbo = pDrmDsp->bo[0];
#endif
}

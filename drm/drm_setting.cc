#include "drm_setting.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <iostream>
#include <cstring>

namespace wm {

namespace {

const std::string kNode = "dev/dri/card0";

}

DirectRenderingManager::DirectRenderingManager()
{
  fd_ = open(kNode.c_str(), O_RDWR | O_CLOEXEC);
  drmDropMaster(fd_);
  drmSetMaster(fd_);

  uint64_t has_dumb = 0;
  if( drmGetCap(fd_, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
      !has_dumb ) {
    std::cout << "drm device " << kNode << "does not support dumb buffers" << std::endl;
    exit(1);
  }

  Prepare();
  /* perform actual modesetting on each found connector+CRTC */

  for( auto device : device_list_ ) {
    device->saved_crtc_ = drmModeGetCrtc(fd_, device->crtc_);
    //buf = &device->bufs[iter->front_buf];
    auto&& buf = device->buffer_[device->front_buffer_];
    auto ret = drmModeSetCrtc(fd_, device->crtc_, buf.fb_, 0, 0,
                         &device->conn_, 1, &device->mode_);
    if (ret)
      fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
              device->conn_, errno);
  }
  
}

void DirectRenderingManager::Prepare()
{
  auto res = drmModeGetResources(fd_);

  if( !res ) {
    
  }

  for( int i=0 ; i < res->count_connectors; ++i ) {
    auto conn = drmModeGetConnector(fd_, res->connectors[i]);
    if (!conn) {
      fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n",
	      i, res->connectors[i], errno);
      continue;
    }

    auto device = std::make_shared<DRMDevice>();
    device->conn_ = conn->connector_id;
    auto ret = SetupDevice(fd_, res, conn, device);
    if (ret) {
      if (ret != -ENOENT) {
        errno = -ret;
        fprintf(stderr, "cannot setup device for connector %u:%u (%d): %m\n",
                i, res->connectors[i], errno);
      }
      //free(dev);
      drmModeFreeConnector(conn);
      continue;
    } else {
      /* free connector data and link device into global list */
      drmModeFreeConnector(conn);
      device_list_.emplace_back(device);
    }
  }
  /* free resources again */
  drmModeFreeResources(res);
  return;
  
}


int DirectRenderingManager::FindCRTC(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device)
{
  drmModeEncoder *enc;
  
  /* first try the currently conected encoder+crtc */
  if (conn->encoder_id)
    enc = drmModeGetEncoder(fd, conn->encoder_id);
  else
    enc = NULL;

  if (enc) {
    if (enc->crtc_id) {
      auto crtc = enc->crtc_id;
      for( auto dev : device_list_ ) {
        if (dev->crtc_ == crtc) {
          crtc = -1;
          break;
        }
      }

      if (crtc >= 0) {
        drmModeFreeEncoder(enc);
        device->crtc_ = crtc;
        return 0;
      }
    }

    drmModeFreeEncoder(enc);
  }

  /* If the connector is not currently bound to an encoder or if the
   * encoder+crtc is already used by another connector (actually unlikely
   * but lets be safe), iterate all other available encoders to find a
   * matching CRTC. */
  for (auto i = 0; i < conn->count_encoders; ++i) {
    enc = drmModeGetEncoder(fd, conn->encoders[i]);
    if (!enc) {
      fprintf(stderr, "cannot retrieve encoder %u:%u (%d): %m\n",
              i, conn->encoders[i], errno);
      continue;
    }

    /* iterate all global CRTCs */
    for (auto j = 0; j < res->count_crtcs; ++j) {
      /* check whether this CRTC works with the encoder */
      if (!(enc->possible_crtcs & (1 << j)))
        continue;

      /* check that no other device already uses this CRTC */
      auto crtc = res->crtcs[j];
      for( auto dev : device_list_ ) {
        if (dev->crtc_ == crtc) {
          crtc = -1;
          break;
        }
      }

      /* we have found a CRTC, so save it and return */
      if (crtc >= 0) {
        drmModeFreeEncoder(enc);
        device->crtc_ = crtc;
        return 0;
      }
    }

    drmModeFreeEncoder(enc);
  }

  fprintf(stderr, "cannot find suitable CRTC for connector %u\n",
          conn->connector_id);
  return -ENOENT;

}

int DirectRenderingManager::SetupDevice(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device)
{
  //auto device = std::make_shared<DRMDevice>();
  if (conn->connection != DRM_MODE_CONNECTED) {
    fprintf(stderr, "ignoring unused connector %u\n",
            conn->connector_id);
    return -1;
  }
  /* check if there is at least one valid mode */
  if (conn->count_modes == 0) {
    fprintf(stderr, "no valid mode for connector %u\n",
            conn->connector_id);
    return -1;
  }

  memcpy(&device->mode_, &conn->modes[0], sizeof(device->mode_));
  device->buffer_[0].width_ = conn->modes[0].hdisplay;
  device->buffer_[0].height_ = conn->modes[0].vdisplay;
  device->buffer_[1].width_ = conn->modes[0].hdisplay;
  device->buffer_[1].height_ = conn->modes[0].vdisplay;

  auto ret = CreateFB(fd, &device->buffer_[0]);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer for connector %u\n",
            conn->connector_id);
    return ret;
  }

  /* create framebuffer #2 for this CRTC */
  ret = CreateFB(fd, &device->buffer_[1]);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer for connector %u\n",
            conn->connector_id);
    return ret;
  }
  
  return 0;
}

int DirectRenderingManager::CreateFB(int fd, DRMBuffer* buf)
{
  drm_mode_create_dumb creq;
  drm_mode_destroy_dumb dreq;
  drm_mode_map_dumb mreq;
  int ret;

  /* create dumb buffer */
  memset(&creq, 0, sizeof(creq));
  creq.width = buf->width_;
  creq.height = buf->height_;
  creq.bpp = 32;
  ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
  if (ret < 0) {
    fprintf(stderr, "cannot create dumb buffer (%d): %m\n",
            errno);
    return -errno;
  }
  buf->stride_ = creq.pitch;
  buf->size_ = creq.size;
  buf->handle_ = creq.handle;

  /* create framebuffer object for the dumb-buffer */
  ret = drmModeAddFB(fd, buf->width_, buf->height_, 24, 32, buf->stride_,
                     buf->handle_, &buf->fb_);
  if (ret) {
    fprintf(stderr, "cannot create framebuffer (%d): %m\n",
            errno);
    ret = -errno;
    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = buf->handle_;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    return ret;
  }

  /* prepare buffer for memory mapping */
  memset(&mreq, 0, sizeof(mreq));
  mreq.handle = buf->handle_;
  ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
  if (ret) {
    fprintf(stderr, "cannot map dumb buffer (%d): %m\n",
            errno);
    ret = -errno;
    drmModeRmFB(fd, buf->fb_);
    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = buf->handle_;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    return ret;
  }

  __off_t offset = mreq.offset;
  /* perform actual memory mapping */
  buf->map_ = reinterpret_cast<uint8_t*>(
      mmap(nullptr, buf->size_, PROT_READ | PROT_WRITE, MAP_SHARED,
           fd, mreq.offset));
  if (buf->map_ == MAP_FAILED) {
    fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n",
            errno);
    ret = -errno;
    drmModeRmFB(fd, buf->fb_);
    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = buf->handle_;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    return ret;
  }

  /* clear the framebuffer to 0 */
  memset(buf->map_, 0, buf->size_);
  return 0;
}




}

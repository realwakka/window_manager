#include "drm_setting.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <iostream>
#include <cstring>

#include <drm.h>

#include <string.h>
#include <stdlib.h>

namespace wm {

namespace {

const std::string kNode = "/dev/dri/card0";

void PageFlipHandler(int fd, unsigned int frame,
                     unsigned int sec, unsigned int usec, void *data)
{
  /* suppress 'unused parameter' warnings */
  (void)fd, (void)frame, (void)sec, (void)usec;
  std::cout << "PageFlipHandler" << std::endl;
  
  int* waiting_for_flip = (int*)data;
  *waiting_for_flip = 0;
}


DrmFb GetDrmFbFromBo(gbm_bo *bo)
{
  auto drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
  
  auto width = gbm_bo_get_width(bo);
  auto height = gbm_bo_get_height(bo);

  auto handle = gbm_bo_get_handle(bo).u32;
  auto stride = gbm_bo_get_stride(bo);
  uint32_t fb = 0;
  auto ret = drmModeAddFB(drm_fd, width, height, 24, 32, stride,
			  handle, &fb);
  

  return DrmFb(bo, fb);
}


void Run(GbmDevice& gbm, EglDevice& egl, DRMDevice& drm)
{

  eglSwapBuffers(egl.GetDisplay(), egl.GetSurface());
  auto bo = gbm_surface_lock_front_buffer(gbm.GetSurface1());
  auto drm_fb = GetDrmFbFromBo(bo);

  auto drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));  
  auto ret = drmModeSetCrtc(drm_fd, drm.crtc_, drm_fb.GetFbId(), 0, 0,
                            &drm.conn_, 1, &drm.mode_);

  float color = 0.0f;

  while(true) {
    int waiting_for_flip = 1;

    color += 0.01f;
    if ( color > 1.0f )
      color = 0.0f;

    //draw
    glClearColor(color, color, color, 1.0f);
    
    eglSwapBuffers(egl.GetDisplay(), egl.GetSurface());
    auto next_bo = gbm_surface_lock_front_buffer(gbm.GetSurface1());
    auto drm_fb = GetDrmFbFromBo(next_bo);
    ret = drmModePageFlip(drm_fd, drm.crtc_, drm_fb.GetFbId(),
                          DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);


    if (ret < 0) {
      std::cout << "PageFlip Failed" << std::endl;
      abort();
    }

    fd_set fds;
    drmEventContext evctx;
    evctx.version = 2;
    evctx.page_flip_handler = PageFlipHandler;
    
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(drm_fd, &fds);
  
    while (waiting_for_flip) {
      ret = select(drm_fd + 1, &fds, NULL, NULL, NULL);
      if (ret < 0) {
        printf("select err: %s\n", strerror(errno));
        return;
      } else if (ret == 0) {
        printf("select timeout!\n");
        return;
      } else if (FD_ISSET(0, &fds)) {
        printf("user interrupted!\n");
        break;
      }
      drmHandleEvent(drm_fd, &evctx);
    }

    gbm_surface_release_buffer(gbm.GetSurface1(), bo);
    bo = next_bo;
  }
}

GbmDevice InitGbm(int drm_fd, int w, int h)
{
  auto device = gbm_create_device(drm_fd);
  auto surface1 = gbm_surface_create(device, w, h,
                                     GBM_FORMAT_XRGB8888,
                                     GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

  auto surface2 = gbm_surface_create(device, w, h,
                                     GBM_FORMAT_XRGB8888,
                                     GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  
  
  return GbmDevice(device, surface1, surface2);
}

EglDevice InitEgl(GbmDevice& gbm_device)
{
  EGLint major, minor, n;

  const EGLint context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  const EGLint config_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_ALPHA_SIZE, 0,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };

// #define get_proc(name) do {				\
//     egl->name = (void *)eglGetProcAddress(#name);	\
//   } while (0)

//   get_proc(eglGetPlatformDisplayEXT);
//   get_proc(eglCreateImageKHR);
//   get_proc(eglDestroyImageKHR);
//   get_proc(glEGLImageTargetTexture2DOES);
//   get_proc(eglCreateSyncKHR);
//   get_proc(eglDestroySyncKHR);
//   get_proc(eglWaitSyncKHR);
//   get_proc(eglDupNativeFenceFDANDROID);

  auto display = eglGetDisplay(gbm_device.GetDevice());

  // if (egl->eglGetPlatformDisplayEXT) {
  //   egl->display = egl->eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_KHR,
  // 						 gbm->dev, NULL);
  // } else {
  //   egl->display = eglGetDisplay((void *)gbm->dev);
  // }

  if (!eglInitialize(display, &major, &minor)) {
    printf("failed to initialize\n");
    abort();
    //return -1;
  }

  printf("Using display %p with EGL version %d.%d\n",
	 display, major, minor);

  printf("===================================\n");
  printf("EGL information:\n");
  printf("  version: \"%s\"\n", eglQueryString(display, EGL_VERSION));
  printf("  vendor: \"%s\"\n", eglQueryString(display, EGL_VENDOR));
  printf("  extensions: \"%s\"\n", eglQueryString(display, EGL_EXTENSIONS));
  printf("===================================\n");

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    printf("failed to bind api EGL_OPENGL_ES_API\n");
    abort();
  }

  EGLConfig config;

  if (!eglChooseConfig(display, config_attribs, &config, 1, &n) || n != 1) {
    printf("failed to choose config: %d\n", n);
    abort();
  }

  auto context = eglCreateContext(display, config,
				  EGL_NO_CONTEXT, context_attribs);
  if (context == NULL) {
    printf("failed to create context\n");
    abort();
  }

  auto surface = eglCreateWindowSurface(
      display, config,
      (EGLNativeWindowType)gbm_device.GetSurface1(),
      NULL);

  
  
  if (surface == EGL_NO_SURFACE) {
    printf("failed to create egl surface\n");
    abort();
  }

  /* connect the context to the surface */
  eglMakeCurrent(display, surface, surface, context);

  printf("OpenGL ES 2.x information:\n");
  printf("  version: \"%s\"\n", glGetString(GL_VERSION));
  printf("  shading language version: \"%s\"\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  printf("  vendor: \"%s\"\n", glGetString(GL_VENDOR));
  printf("  renderer: \"%s\"\n", glGetString(GL_RENDERER));
  printf("  extensions: \"%s\"\n", glGetString(GL_EXTENSIONS));
  printf("===================================\n");


  return EglDevice(display, config, context, surface);
}

}

DirectRenderingManager::DirectRenderingManager()
{
  fd_ = open(kNode.c_str(), O_RDWR | O_CLOEXEC);
  auto res = drmDropMaster(fd_);
  res = drmSetMaster(fd_);

  // uint64_t has_dumb = 0;
  // if( drmGetCap(fd_, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
  //     !has_dumb ) {
  //   std::cout << "drm device " << kNode << "does not support dumb buffers" << std::endl;
  //   exit(1);
  // }

  Prepare();

  if (!device_list_.empty()) {

    auto gbm = InitGbm(fd_, device_list_[0]->mode_.hdisplay, device_list_[0]->mode_.vdisplay);
    auto egl = InitEgl(gbm);

    Run(gbm, egl, *device_list_[0]);
  }

  // for( auto device : device_list_ ) {
  //   device->saved_crtc_ = drmModeGetCrtc(fd_, device->crtc_);
  //   //buf = &device->bufs[iter->front_buf];
  //   auto&& buf = device->buffer_[0];
  //   auto ret = drmModeSetCrtc(fd_, device->crtc_, buf.fb_, 0, 0,
  //                        &device->conn_, 1, &device->mode_);
  //   if (ret)
  //     fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
  //             device->conn_, errno);
  // }

  
}

void DirectRenderingManager::Prepare()
{
  auto res = drmModeGetResources(fd_);

  if( !res ) {
    exit(-1);
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
  // device->buffer_[0].width_ = conn->modes[0].hdisplay;
  // device->buffer_[0].height_ = conn->modes[0].vdisplay;
  // device->buffer_[1].width_ = conn->modes[0].hdisplay;
  // device->buffer_[1].height_ = conn->modes[0].vdisplay;

  FindCRTC(fd, res, conn, device);

  // auto ret = CreateFB(fd, &device->buffer_[0]);
  // if (ret) {
  //   fprintf(stderr, "cannot create framebuffer for connector %u\n",
  //           conn->connector_id);
  //   return ret;
  // }

  // /* create framebuffer #2 for this CRTC */
  // // ret = CreateFB(fd, &device->buffer_[1]);
  // if (ret) {
  //   fprintf(stderr, "cannot create framebuffer for connector %u\n",
  //           conn->connector_id);
  //   return ret;
  // }
  
  return 0;
}

// int DirectRenderingManager::CreateDumbFB(int fd, DRMBuffer* buf)
// {
//   // auto gbm_device = InitGbm(fd, buf->width_, buf->height_);
//   // auto egl_device = InitEgl(gbm_device);


//   // eglSwapBuffers(egl_device.GetDisplay(), egl_device.GetSurface());
//   // auto bo = gbm_surface_lock_front_buffer(gbm_device.GetSurface());
  

//   // int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
//   // ret = drmModeAddFB(fd, buf->width_, buf->height_, 24, 32, buf->stride_,
//   //                    buf->handle_, &buf->fb_);


//   return 0;
//   // drm_mode_create_dumb creq;
//   // drm_mode_destroy_dumb dreq;
//   // drm_mode_map_dumb mreq;
//   // int ret;

//   // /* create dumb buffer */
//   // memset(&creq, 0, sizeof(creq));
//   // creq.width = buf->width_;
//   // creq.height = buf->height_;
//   // creq.bpp = 32;
//   // ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
//   // if (ret < 0) {
//   //   fprintf(stderr, "cannot create dumb buffer (%d): %m\n",
//   //           errno);
//   //   return -errno;
//   // }
  
//   // buf->stride_ = creq.pitch;
//   // buf->size_ = creq.size;
//   // buf->handle_ = creq.handle;

//   // /* create framebuffer object for the dumb-buffer */
//   // ret = drmModeAddFB(fd, buf->width_, buf->height_, 24, 32, buf->stride_,
//   //                    buf->handle_, &buf->fb_);
//   // if (ret) {
//   //   fprintf(stderr, "cannot create framebuffer (%d): %m\n",
//   //           errno);
//   //   ret = -errno;
//   //   memset(&dreq, 0, sizeof(dreq));
//   //   dreq.handle = buf->handle_;
//   //   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
//   //   return ret;
//   // }

//   // /* prepare buffer for memory mapping */
//   // memset(&mreq, 0, sizeof(mreq));
//   // mreq.handle = buf->handle_;
//   // ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
//   // if (ret) {
//   //   fprintf(stderr, "cannot map dumb buffer (%d): %m\n",
//   //           errno);
//   //   ret = -errno;
//   //   drmModeRmFB(fd, buf->fb_);
//   //   memset(&dreq, 0, sizeof(dreq));
//   //   dreq.handle = buf->handle_;
//   //   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
//   //   return ret;
//   // }

//   // __off_t offset = mreq.offset;
//   // /* perform actual memory mapping */
//   // buf->map_ = reinterpret_cast<uint8_t*>(
//   //     mmap(nullptr, buf->size_, PROT_READ | PROT_WRITE, MAP_SHARED,
//   //          fd, mreq.offset));
//   // if (buf->map_ == MAP_FAILED) {
//   //   fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n",
//   //           errno);
//   //   ret = -errno;
//   //   drmModeRmFB(fd, buf->fb_);
//   //   memset(&dreq, 0, sizeof(dreq));
//   //   dreq.handle = buf->handle_;
//   //   drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
//   //   return ret;
//   // }

//   // /* clear the framebuffer to 0 */
//   // memset(buf->map_, 0, buf->size_);
//   // return 0;
// }


}

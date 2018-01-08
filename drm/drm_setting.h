#ifndef REALWAKKA_KERNEL_MODE_SETTING_H_
#define REALWAKKA_KERNEL_MODE_SETTING_H_

#include <memory>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace wm {

class GbmDevice
{
 public:
  GbmDevice(gbm_device* device, gbm_surface* surface1, gbm_surface* surface2)
      : device_(device), surface1_(surface1), surface2_(surface2) {}
  
  gbm_surface* GetSurface1() { return surface1_; }
  gbm_surface* GetSurface2() { return surface2_; }
  gbm_device* GetDevice() { return device_; }
  
 private:
  gbm_device* device_;
  gbm_surface* surface1_;
  gbm_surface* surface2_;

};

class EglDevice
{
 public:
  EglDevice(EGLDisplay display, EGLConfig config, EGLContext context, EGLSurface surface)
      : display_(display),
        config_(config),
        context_(context),
        surface_(surface)
  {}

  EGLDisplay GetDisplay() { return display_; }
  EGLConfig GetConfig() { return config_; }
  EGLContext GetContext() { return context_; }
  EGLSurface GetSurface() { return surface_; }
  
 private:
  EGLDisplay display_;
  EGLConfig config_;
  EGLContext context_;
  EGLSurface surface_;

};

class DrmFb
{
 public:
  DrmFb(gbm_bo* bo, uint32_t fb_id) : bo_(bo), fb_id_(fb_id) {}

  gbm_bo* GetGbmBo() { return bo_; }
  uint32_t GetFbId() { return fb_id_; }

 private:
  gbm_bo* bo_;
  uint32_t fb_id_;
};

class DRMDevice
{
 public:
  DRMDevice() : conn_(0), crtc_(0) {}
  drmModeModeInfo mode_;
  uint32_t conn_;
  uint32_t crtc_;

};
  
class DirectRenderingManager 
{
 public:
  DirectRenderingManager();
  void Prepare(); 
  int SetupDevice(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device);
  int FindCRTC(int fd, drmModeRes* res, drmModeConnector* conn, std::shared_ptr<DRMDevice> device);

 private:
  int fd_;
  std::vector<std::shared_ptr<DRMDevice>> device_list_;

};

}

#endif


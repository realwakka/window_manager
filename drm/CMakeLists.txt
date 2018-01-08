add_library(el_drm SHARED 
  #kms.c
  drm_setting.cc)
target_link_libraries(el_drm drm gbm EGL GL)
target_include_directories(el_drm PRIVATE /usr/include/libdrm)


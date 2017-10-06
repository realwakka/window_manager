add_library(el_drm SHARED 
  kms.c
  drm_setting.cc)
target_link_libraries(el_drm drm)
target_include_directories(el_drm PRIVATE /usr/local/include/libdrm)


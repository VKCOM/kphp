#include "runtime-headers.h"

#include "runtime-light/header.h"

const ImageInfo *vk_k2_describe() {
  static ImageInfo imageInfo {"respectful_busy_loop", 0, 2, {}};
  return &imageInfo;
}
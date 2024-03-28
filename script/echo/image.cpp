#include "runtime-headers.h"

#include "runtime-light/scheme.h"

const ImageInfo *vk_k2_describe() {
  static ImageInfo imageInfo {"echo", 1, 1, {1, 2, 3}};
  return &imageInfo;
}
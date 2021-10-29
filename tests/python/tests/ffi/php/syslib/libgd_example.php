<?php

function test() {
  $libgd = FFI::cdef('
      typedef struct gdImage gdImage;

      gdImage *gdImageCreate(int sx, int sy);
      void gdImageDestroy(gdImage *image);

      int gdImageColorAllocate(gdImage *img, int r, int g, int b);
      void gdImageLine(gdImage *img, int x1, int y1, int x2, int y2, int color);

      int gdImageFile(gdImage *img, const char *filename);
  ', 'libgd.so');

  $image = $libgd->gdImageCreate(64, 64);

  // Allocate the color black (red, green and blue all minimum).
  // Since this is the first color in a new image, it will
  // be the background color.
  $black = $libgd->gdImageColorAllocate($image, 0, 0, 0);

  var_dump($black);

  // Allocate the color white (red, green and blue all maximum).
  $white = $libgd->gdImageColorAllocate($image, 255, 255, 255);

  $libgd->gdImageLine($image, 0, 0, 63, 63, $white);

  var_dump($libgd->gdImageFile($image, __DIR__ . '/result.png'));

  $libgd->gdImageDestroy($image);

  unlink(__DIR__ . '/result.png');
}

test();

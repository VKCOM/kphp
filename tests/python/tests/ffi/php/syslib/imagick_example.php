<?php

function test() {
  $imagick = FFI::cdef('
      struct MagickWand {
        uint8_t _opaque;
      };

      void MagickWandGenesis();

      struct MagickWand *NewMagickWand();
      struct MagickWand *DestroyMagickWand(struct MagickWand *wand);

      bool MagickReadImage(struct MagickWand *wand, const char *filename);

      size_t MagickGetImageWidth(struct MagickWand *wand);
      size_t MagickGetImageHeight(struct MagickWand *wand);
  ', 'libMagickWand-6.Q16.so');

  $imagick->MagickWandGenesis();

  $handle = $imagick->NewMagickWand();
  $imagick->MagickReadImage($handle, __DIR__ . '/favicon32.png');
  var_dump($imagick->MagickGetImageWidth($handle));
  var_dump($imagick->MagickGetImageHeight($handle));

  $imagick->DestroyMagickWand($handle);
}

test();

<?php

namespace Classes;

class Photo {
  const DEFINE_A = 'A';
  const DEFINE_B = 'B';
  const DEFINE_0 = 0;
  const DEFINE_1 = 1;

  const RETINA_MAP = ['s' => 'l', 'm' => 'l', 'l' => 'y', 'y' => 'x'];
  const LIST_WITH_CONV            = [10, 9.7, 'A'];
  const LIST_WITH_DEFINES         = [self::DEFINE_A.self::DEFINE_A.self::DEFINE_B, self::DEFINE_0 & self::DEFINE_1];
  const DEFINES_A_TO_DEFINES      = [self::DEFINE_A => self::DEFINE_0, self::DEFINE_B => self::DEFINE_1];
  const DEFINES_A_TO_NUMS         = [self::DEFINE_A => 0, self::DEFINE_B => 1];
  const DEFINES_NUMS_TO_DEFINES_A = [0 => self::DEFINE_A, 1 => self::DEFINE_B];

  /**
   * @kphp-infer
   * @param string $wl
   * @return null
   */
  public static function xxx($wl) {
      var_dump(self::RETINA_MAP[$wl]);
      var_dump(self::LIST_WITH_DEFINES[0]);

      var_dump(self::DEFINES_A_TO_DEFINES['A']);
      var_dump(self::DEFINES_A_TO_NUMS[self::DEFINE_A]);

      var_dump(self::DEFINES_NUMS_TO_DEFINES_A[0]);
      var_dump(self::DEFINES_NUMS_TO_DEFINES_A[self::DEFINE_0 + self::DEFINE_0 * self::DEFINE_1]);
      return null;
  }
}

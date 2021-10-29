<?php

class VectorUtils {
  /** @param ffi_cdata<vector, struct Vector2> $vec */
  public static function dump($vec) {
    var_dump([$vec->x, $vec->y]);
  }
}

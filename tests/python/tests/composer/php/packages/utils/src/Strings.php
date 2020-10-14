<?php

namespace VK\Utils;

class Strings {
  /**
   * @param mixed $string
   * @param mixed $prefix
   * @return bool
   */
  public static function startsWith($string, $prefix) {
    return (strlen($string) >= strlen($prefix)) &&
      substr_compare($string, $prefix, 0, strlen($prefix)) == 0;
  }
}

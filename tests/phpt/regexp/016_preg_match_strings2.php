@ok
<?php

class Utils {
  public static function validateNum(string $s): ?string {
    if (preg_match('/^([0-9]+)$/', $s, $matches)) {
      return $s;
    }
    return null;
  }
}

var_dump(Utils::validateNum(''));
var_dump(Utils::validateNum('0'));
var_dump(Utils::validateNum('4394'));
var_dump(Utils::validateNum('foo'));
var_dump(Utils::validateNum('foo20'));

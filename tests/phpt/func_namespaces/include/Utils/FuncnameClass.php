<?php

namespace VK\Utils;

class FuncnameClass {
  public static function static_f() { return ['VK/Utils', __FUNCTION__]; }
  public function f() { return ['VK/Utils', __FUNCTION__]; }
}

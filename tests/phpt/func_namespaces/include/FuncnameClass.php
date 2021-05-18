<?php

namespace VK;

class FuncnameClass {
  public static function static_f() { return ['VK', __FUNCTION__]; }
  public function f() { return ['VK', __FUNCTION__]; }
}

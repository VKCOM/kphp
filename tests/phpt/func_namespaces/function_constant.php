@ok
<?php

// Testing __FUNCTION__ constant for the namespaced funcs.

use function VK\funcname as funcname2;
use function VK\Utils\funcname as funcname3;

require_once __DIR__ . '/include/funcname.php';
require_once __DIR__ . '/include/Utils/funcname.php';
require_once __DIR__ . '/include/FuncnameClass.php';
require_once __DIR__ . '/include/Utils/FuncnameClass.php';

function funcname() { return ['global', __FUNCTION__]; }

function test() {
  var_dump(funcname());
  var_dump(funcname2());
  var_dump(funcname3());

  $c = new FuncnameClass();
  var_dump(FuncnameClass::static_f());
  var_dump($c->f());

  $c2 = new \VK\FuncnameClass();
  var_dump(\VK\FuncnameClass::static_f());
  var_dump($c2->f());

  $c3 = new \VK\Utils\FuncnameClass();
  var_dump(\VK\Utils\FuncnameClass::static_f());
  var_dump($c3->f());
}

class FuncnameClass {
  public static function static_f() { return ['global', __FUNCTION__]; }
  public function f() { return ['global', __FUNCTION__]; }
}

test();

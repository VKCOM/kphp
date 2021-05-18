@ok
<?php

require __DIR__ . '/include/Utils/Foo.php';
require __DIR__ . '/include/Foo.php';

use VK\Utils;

class Foo {
  public static function f() { var_dump('f()'); }
}

\Foo::f();
var_dump(\VK\Utils\Foo::f());
var_dump(VK\Utils\Foo::f());
var_dump(\Utils\Foo::f());
var_dump(Utils\Foo::f());
var_dump(VK\Utils\Foo::f());
var_dump(Utils\Foo::f());

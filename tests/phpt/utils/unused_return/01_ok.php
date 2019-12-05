@ok
<?php
/**
 * @kphp-warn-unused-result
 */
function test() {
  return 111;
}

class Foo {
  /**
   * @kphp-warn-unused-result
   */
  public static function bar() {
    return 444;
  }
}

$a = test();
$b = Foo::bar();


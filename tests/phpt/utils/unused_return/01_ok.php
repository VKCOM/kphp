@ok
<?php
/**
 * @kphp-warn-unused-result
 */
/**
 * @return int
 */
function test() {
  return 111;
}

class Foo {
  /**
   * @kphp-warn-unused-result
   * @return int
   */
  public static function bar() {
    return 444;
  }
}

$a = test();
$b = Foo::bar();


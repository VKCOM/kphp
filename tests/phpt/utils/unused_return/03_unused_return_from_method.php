@kphp_should_fail
/Result of function call is unused, but function is marked with @kphp-warn-unused-result/
<?php
class Foo {
  /**
   * @kphp-warn-unused-result
   */
  public static function bar() {
    return 777;
  }
}

Foo::bar();


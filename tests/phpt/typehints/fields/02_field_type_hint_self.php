@ok
<?php

trait T {
    public self $s;
    public static ?self $static_s;
}

abstract class A1 {
    use T;

    function f() { echo "A1 f\n"; }
}

class A extends A1 {
}

$a = new A;
$a->s = new A;
$a->s->f();
A1::$static_s = new A;
A1::$static_s->f();


class Test {
  public static ?self $instance = null;

  public static function create() {
    if (!self::$instance) {
      self::$instance = new self();
    }
    return self::$instance;
  }

  public function just() {
    return "Do it\n";
  }
}

Test::create();
echo Test::$instance->just();


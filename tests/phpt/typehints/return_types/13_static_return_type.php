@ok
<?php
class Base {
  // :static is unsupported in PHP till 8.0, but supported in KPHP
  // test it, making two declarations — for PHP and for KPHP
  public static function foo()
#ifndef KPHP
/*
#endif
: static
#ifndef KPHP
 */
#endif
    {
    return new static();
  }
}

class D1 extends Base {
    var $d1 = 0;
}

class D2 extends Base {
    var $d2 = 0;
}

Base::foo();
D1::foo()->d1;
D2::foo()->d2;

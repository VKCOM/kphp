@ok
<?php
  class Foo
  {
    const FOO = "foo";
    const BAR = "bar";
    const FOOBAR = "foobar";

    static $x1 = null;

    static $x2 = 10;
    static $x3 = Foo::FOO;

    static $x5, $y;

    static $x6 = array(1,2,3);
    static $x7 = array(1 => 2, 2 => 3);
    static $x8 = array(Foo::FOO => Foo::BAR, Foo::FOOBAR);

    static $x9 = +5;
    static $x10 = -5;
  }

  $y = Foo::$x1;
?>

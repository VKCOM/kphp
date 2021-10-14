@kphp_should_fail php8
/Bar::A cannot override final constant Foo::A/
<?php

class Foo {
  final const A = "foo";
}

class Bar extends Foo {
  const A = "bar";
}

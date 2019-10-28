@ok
<?php

interface Foo {
  const x = 5;
  public function f();
}

interface FooBar extends Foo {
  public function fb();
}

class A implements FooBar {
  public function f() { echo "f\n"; }
  public function fb() { echo "fb\n"; }
}

abstract class D implements FooBar {
}

class C extends D  {
  public function f() { echo "f\n"; }
  public function fb() { echo "fb\n"; }
}


function demo() {
	echo Foo::x . "\n";
	echo FooBar::x . "\n";
	echo A::x . "\n";
	echo D::x . "\n";
	echo C::x . "\n";
}

demo();


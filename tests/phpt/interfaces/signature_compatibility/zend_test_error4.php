@kphp_should_fail
/Declaration of Bar5::setSelf\(\) must be compatible with Foo5::setSelf\(\)/
<?php

// Zend/tests/bug60573_2.phpt

class Foo1 {
  public function setSelf(self $s) {}
}

class Bar1 extends Foo1 {
  public function setSelf(parent $s) {}
}

class Foo2 {
  public function setSelf(Foo2 $s) {}
}

class Bar2 extends Foo2 {
  public function setSelf(parent $s) {}
}

class Base {}

class Foo3 extends Base{
  public function setSelf(parent $s) {}
}

class Bar3 extends Foo3 {
  public function setSelf(Base $s) {}
}

class Foo4 {
  public function setSelf(self $s) {}
}

class Foo5 extends Base {
  public function setSelf(parent $s) {}
}

class Bar5 extends Foo5 {
  public function setSelf(parent $s) {}
}

@ok
<?php

class I {
}

class A extends I {
  public $a = 1;
}

class B extends I {
  public $b = 2;
}

class C extends I {
}

class CC extends C {
  public $cc = 3;
}

function test_function(I $i) {
  var_dump (get_class($i));

  if ($i instanceof A) {
    var_dump($i->a);
  }
  if ($i instanceof B) {
    var_dump($i->b);
  }
  if ($i instanceof CC) {
    var_dump($i->cc);
  }
}

function test_empty_base_class() {
  test_function(new A);
  test_function(new B);
  test_function(new CC);
}

test_empty_base_class();

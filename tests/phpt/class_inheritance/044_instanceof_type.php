@ok
<?php

class A { 
  public $x = 0;
};

class B extends A { 
  public $y = 0;
};


/**
 * @return bool
 */
function f(A $x) {
  return $x instanceof B;
}

var_dump(f(new A()));
var_dump(f(new B()));

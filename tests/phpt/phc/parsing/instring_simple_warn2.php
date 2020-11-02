@kphp_should_warn
/Simple string expressions with \[\] can work wrong\. Use more \{\}/
<?php

$arr = [1, 2, 3];

class X {
  public $y = 10;
}

$x = new X();

echo "$x->y[1]\n";

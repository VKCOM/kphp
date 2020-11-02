@ok
<?php

$arr = [1, 2, 3];
echo "$arr[0][1]\n"; // "1[1]"

class X {
  public $y = 10;
}
$x = new X();
echo "$x->y[1]\n"; // "10[1]"

@ok
<?php
class A { public $x = 5; }

$x = new A();
$x = false;
if ($x instanceof A) {
  echo "A!\n";
} else {
    echo "not A!\n";
}
 

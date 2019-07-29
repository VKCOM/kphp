@ok
<?php

class A {
    var $x = 20;
}

$a = [10 => new A];
var_dump($a[10]->x);

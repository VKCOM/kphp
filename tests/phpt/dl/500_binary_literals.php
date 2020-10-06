@ok
<?php

#ifndef KPHP
function tuple(...$args) { return $args; }
#endif

var_dump(0b0);
var_dump(0b1);

var_dump(0b000);
var_dump(0b111);

var_dump(0b0010);
var_dump(0b0101);

var_dump(0b1111111111111111);
var_dump(0b1111111111111111111111111111111);

$arr = [1, 2, 3, 4];
var_dump($arr[0b01]);

class A {
    public function foo() { return 10; }
}

$a = tuple(1);
var_dump($a[0b0]);

$x = 0b10;
switch ($x) {
    case 0b00:
        var_dump(0);
        break;
    case 0b01:
        var_dump(1);
        break;
    case 0b10:
        var_dump(10);
        break;
    case 0b11:
        var_dump(11);
        break;
}

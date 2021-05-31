@ok
<?php

class Foo {
    public function __toString(): string {
        return "1";
    }
}

/**
 * @return int
 */
function data2() {
    return 2;
}

function f() {
    $array = [1, 2, 3];

    echo $array[(int)true];
    echo $array[$array[0]];
    echo $array[(string)(new Foo)];
    echo $array[data2()];
    echo $array[1];
    echo $array["1"];
}

f();

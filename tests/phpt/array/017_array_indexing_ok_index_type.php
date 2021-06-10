@ok
<?php

class Foo {
    public function __toString(): string {
        return "1";
    }
}

function data(): ?int {
    return 1;
}

/**
 * @return string|false
 */
function data2() {
    return "1";
}

function f() {
    $array = [1, 2, 3];

    echo $array[(int)true];
    echo $array[$array[0]];
    echo $array[(string)(new Foo)];
    echo $array[data()];
    echo $array[data2()];
    echo $array[1];
    echo $array["1"];
    echo $array[true];
    echo $array[false];
}

f();

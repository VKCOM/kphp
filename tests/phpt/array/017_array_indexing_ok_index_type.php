@ok
<?php

class Foo {
    public function __toString(): string {
        return "1";
    }
}

function data(): string {
    return "";
}

/**
 * @return int
 */
function data2() {
    return 12;
}

function f() {
    $array = [1, 2, 3];
    $tuple = tuple(1, 2);

    echo $array[$tuple[0]];
    echo $array[(int)true];
    echo $array[$array[0]];
    echo $array[(string)(new Foo)];
    echo $array[data()];
    echo $array[data2()];
    echo $array[1];
    echo $array["1"];
}

f();

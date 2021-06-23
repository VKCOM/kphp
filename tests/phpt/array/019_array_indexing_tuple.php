@kphp_should_fail
/but string type is passed/
/can not get element/
<?php

class Foo {
    public function __toString(): string {
        return "1";
    }
}

/**
 * @return ?string
 */
function data1() {
    return null;
}

/**
 * @return string|false
 */
function data2() {
    return "";
}

function f() {
    $tuple = tuple(1, 2, 3);

    echo $tuple["1"];
    echo $tuple[1];
}

f();

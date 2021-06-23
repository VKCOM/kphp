@kphp_should_fail
/but int\[\] type is passed/
/but Foo type is passed/
<?php

class Foo {}

function data(): ?int {
    return null;
}

/**
 * @return int|false
 */
function data2() {
    return false;
}

function f() {
    $array = [1, 2, 3];

    echo $array[$array];
    echo $array[new Foo];
}

f();

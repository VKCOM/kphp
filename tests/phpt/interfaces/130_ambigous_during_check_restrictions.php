@kphp_should_fail
/assign Foo to \$a/
<?php

interface A {}
interface B {}
interface C {}

interface AC extends A, C {}

class Foo implements A, B, C {}

function run() {
    /**@var AC*/
    $a = new Foo;
}

run();

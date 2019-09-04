@kphp_should_fail
<?php

abstract class A {
    abstract public static function foo();
}

A::foo();


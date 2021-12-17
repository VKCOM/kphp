@kphp_should_fail
/Can not call an abstract method A::foo/
<?php

abstract class A {
    abstract public static function foo();
}

A::foo();


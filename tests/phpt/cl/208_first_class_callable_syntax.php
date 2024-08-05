@kphp_should_fail
\First class callable syntax is not supported\
<?php

class A {
    public static function f_old() {}
}

$f_new = A::f_old(...);
echo $f_new();

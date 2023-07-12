@kphp_should_fail
\First class callable syntax is not supported\
<?php

class A {
    public function f_old() {}
}

$a = new A();

$f_new = $a->f_old(...);
echo $f_new();

@kphp_should_fail
<?php

class A {
    public function __clone() {
        sched_yield(); 
        var_dump(10); 
    }
}

$a = new A;
wait(fork($a->__clone()));
$b = clone $a;

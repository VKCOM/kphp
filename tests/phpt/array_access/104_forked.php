@kphp_should_fail
/Function \[ArrayAccess::offsetGet\] is a method of ArrayAccess, it cannot call resumable functions/
/Function transitively calls the resumable function along the following chain:/
/ArrayAccess::offsetGet -> A::offsetGet/

<?php

class A implements \ArrayAccess {
    public function offsetSet($offset, $value) {}
    /** @return bool */
    public function offsetExists($offset) {return true;}
    public function offsetUnset($offset) {}
    /** @return mixed */
    public function offsetGet($offset) {return null;}
}

function test() {
    $x = new A();
    $y = fork($x->offsetGet(42));

    wait($y);
}

test();

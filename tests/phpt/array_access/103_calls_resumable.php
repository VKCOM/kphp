@kphp_should_fail
/Function \[ArrayAccess::offsetGet\] is a method of ArrayAccess, it cannot call resumable functions/
/Function transitively calls the resumable function along the following chain:/
/ArrayAccess::offsetGet -> A::offsetGet -> like_rpc_write -> sched_yield/

<?php

function like_rpc_write() {
    printf("log\n");
    sched_yield(); // resumable
}

class A implements \ArrayAccess {
    public function offsetSet($offset, $value) {}
    /** @return bool */
    public function offsetExists($offset) {return true;}
    public function offsetUnset($offset) {}
    /** @return mixed */
    public function offsetGet($offset) {
        like_rpc_write();
        return null;
    }
}

function test() {
    $x = new A();
}

test();

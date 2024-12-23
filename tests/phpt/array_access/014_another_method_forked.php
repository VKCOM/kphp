@ok
<?php
require_once 'kphp_tester_include.php';

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
        return null;
    }

    public function to_be_forked() {
        printf("in to_be_forked\n");
        like_rpc_write();
        return null;
    }
}

function test() {
    $x = new A();

    $fut = fork($x->to_be_forked());
    wait($fut);
}

test();

@ok
<?php

class B {
    private function foo() {
        var_dump("B");
    }

    public function test() {
        $this->foo();
    }
}

class D extends B {
    private function foo() {
        var_dump("D");
    }
}

function test(B $b) {
    $b->test();
}

test(new B());
test(new D());


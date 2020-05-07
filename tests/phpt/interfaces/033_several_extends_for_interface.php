@ok
<?php

interface IA { public function foo(); }
interface IB { public function bar(); }

interface IAB extends IA, IB {}

class AB implements IAB {
    public function foo() { var_dump("AB::foo"); }
    public function bar() { var_dump("AB::bar"); }
}

function run_foo(IA $ia) { $ia->foo(); }
function run_bar(IB $ib) { $ib->bar(); }

$ab = new AB();
run_foo($ab);
run_bar($ab);

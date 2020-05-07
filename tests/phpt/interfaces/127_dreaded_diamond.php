@kphp_should_fail
/duplicated class: IGrandPa in hierarchy from class: Derived/
<?php

interface IGrandPa { 
    const grand_c = "Grand";
    public function grand();
}

interface IBase1 extends IGrandPa { public function base1(); }
interface IBase2 extends IGrandPa { public function base2(); }

class Derived implements IBase1, IBase2 {
    public function grand() {
        var_dump("Derived::grand");
    }

    public function base1() {
        var_dump("Derived::base1");
    }

    public function base2() {
        var_dump("Derived::base2");
    }
}

function run_grand(IGrandPa $pa) { $pa->grand(); }
function run_base1(IBase1   $b1) { $b1->base1(); }
function run_base2(IBase2   $b2) { $b2->base2(); }

function run_derived(Derived $d) {
    run_grand($d);
    run_base1($d);
    run_base2($d);
}

run_derived(new Derived());

var_dump(Derived::grand_c);


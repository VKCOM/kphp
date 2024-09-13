@ok non-idempotent k2_skip
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class */
class A {
    public $a_id = 1;
}

/** @kphp-immutable-class */
class B extends A {
    public $b_id = 2;
}

/** @kphp-immutable-class */
class C extends B {
    public $c_id = 3;
}

/** @kphp-immutable-class */
class D extends B {
    public $d_id = 4;
}


function save_derive_as_base(string $key, A $base) {
    instance_cache_store($key, $base);
}

function test_base_polymorphic() {
    save_derive_as_base("a_key", new A());
    save_derive_as_base("b_key", new B());
    save_derive_as_base("c_key", new C());


    $res_a = instance_cache_fetch(A::class, "a_key");
    if ($res_a instanceof A) {
        var_dump($res_a->a_id);
    }

    $res_b = instance_cache_fetch(A::class, "b_key");
    if ($res_b instanceof B) {
        var_dump($res_b->b_id);
    }

    $res_c = instance_cache_fetch(A::class, "c_key");
    if ($res_c instanceof C) {
        var_dump($res_c->c_id);
    }
}

test_base_polymorphic();

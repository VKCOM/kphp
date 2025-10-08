@kphp_should_fail k2_skip
/Can not store polymorphic type A with mutable derived class D/
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class
 *  @kphp-serializable */
class A {
  /** @kphp-serialized-field 0 */
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

// D is derived from A but not marked immutable
class D extends B {
    public $d_id = 4;
}


function save_derive_as_base(string $key, A $base) {
    instance_cache_store($key, $base);
}

function test_base_polymorphic() {
    $class = new B();
    save_derive_as_base("simple_key", $class);

    $res = instance_cache_fetch(A::class, "simple_key");
    if ($res instanceof B) {
        var_dump($res->b_id);
    }
}

test_base_polymorphic();

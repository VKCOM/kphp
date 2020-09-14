@ok
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class B {
    /**
     * @var int
     * @kphp-serialized-field 1
     */
    public $id = 10;

    /**
     * @var int
     * @kphp-serialized-field 2
     */
    public $remote_id = 7770;
}

/** @kphp-serializable */
class A {
    /**
     * @var B[]
     * @kphp-serialized-field 1
     */
    public $x;

    public function __construct() {
        $this->x = [46411 => new B()];
    }
}

function run() {
    $a = new A();
    $serialized = instance_serialize($a);
    var_dump(base64_encode($serialized));
    $a_new = instance_deserialize($serialized, A::class);

    var_dump(array_keys($a->x));
    var_dump(array_keys($a_new->x));

    var_dump(array_values($a->x)[0]->id);
    var_dump(array_values($a_new->x)[0]->id);
}

run();

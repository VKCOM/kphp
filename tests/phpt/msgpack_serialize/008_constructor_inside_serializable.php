@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/** 
 * @kphp-serializable
 **/
class A {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $y = 10;

    public function __construct(int $new_y) { $this->y = $new_y; var_dump("here"); }
}

function run() {
    $a = new A(2000);
    $serialized = instance_serialize($a);
    var_dump(base64_encode($serialized));
    $a_new = instance_deserialize($serialized, A::class);

    var_dump($a->y === $a_new->y);
}

run();

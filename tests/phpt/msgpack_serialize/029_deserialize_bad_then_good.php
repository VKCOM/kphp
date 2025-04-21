@ok
<?php
require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class X {
    /** @kphp-serialized-field 0 */
    public string $x;
    public function __construct(string $x) {
        $this->x = $x;
    }
}

/** @kphp-serializable */
class Y {
    /** @kphp-serialized-field 0 */
    public int $y;
    public function __construct(int $y) {
        $this->y = $y;
    }
}

function test() {
    $serialized = instance_serialize(new X("stroka"));
    // var_dump($serialized);
    // $failed_deser = instance_deserialize($serialized, Y::class);
    // assert_true($failed_deser === null);
    $succ_deser = instance_deserialize($serialized, X::class);
    assert_true($succ_deser !== null);
}
test();

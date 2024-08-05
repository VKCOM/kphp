@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable **/
class A {
    /**
     * @kphp-serialized-field 1
     * @var ?bool
     */
    public $bool_f = null;

    public function __construct() {
        $this->bool_f = true;
    }
}

$a = new A();
$s = instance_serialize($a);
$d = instance_deserialize($s, A::class);
var_dump($d === null);

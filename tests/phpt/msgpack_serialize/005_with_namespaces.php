@ok
<?php

require_once 'kphp_tester_include.php';

use Classes\A, Classes2\A as A2;

/** 
 * @kphp-serializable
 **/
class AHolder {
    /**
     * @kphp-serialized-field 1
     * @var A
     */
    public $a_f;

    /**
     * @kphp-serialized-field 2
     * @var \Classes\A
     */
    public $a_f2;

    /**
     * @kphp-serialized-field 3
     * @var A2
     */
    public $a2_f;

    public function __construct() {
      $this->a_f = new \Classes\A;
      $this->a_f2 = new A();
      $this->a2_f = new A2();
    }

  public function equal_to(AHolder $another): bool {
        return $this->a_f->equal_to($another->a_f)
            && $this->a_f2->equal_to($another->a_f2)
            && $this->a2_f->equal_to($another->a2_f);
    }
}

function run() {
    $aholder = new AHolder();
    $serialized = instance_serialize($aholder);
    var_dump(base64_encode($serialized));
    $aholder_new = instance_deserialize($serialized, AHolder::class);

    var_dump($aholder->equal_to($aholder_new));

    $a = new Classes\A();
    $serialized = instance_serialize($a);
    var_dump(base64_encode($serialized));
    $a_new = instance_deserialize($serialized, Classes\A::class);

    var_dump($a->equal_to($a_new));
}

run();

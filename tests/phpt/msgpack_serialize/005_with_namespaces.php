@ok
<?php

require_once 'polyfills.php';

/** 
 * @kphp-serializable
 **/
class AHolder {
    /**
     * @kphp-serialized-field 1
     * @var \Classes\A
     */
    public $a_f;

    public function __construct() {
      $this->a_f = new \Classes\A;
    }

  public function equal_to(AHolder $another): bool {
        var_dump($another->a_f->int_f);
        return $this->a_f->equal_to($another->a_f);
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

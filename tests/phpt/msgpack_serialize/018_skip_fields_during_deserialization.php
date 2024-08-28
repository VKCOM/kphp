@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/** @kphp-serializable */
class Before {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $b = 10;

   /**
    * @kphp-serialized-field 1
    * @var int
    */
   public $a = 228;
}

/** @kphp-serializable */
class After {
    /**
     * @kphp-serialized-field 2
     * @var int
     */
    public $b = 10;
}

$b = new Before();
$b_raw = instance_serialize($b);

$a_new = instance_deserialize($b_raw, After::class);
var_dump($a_new->b);

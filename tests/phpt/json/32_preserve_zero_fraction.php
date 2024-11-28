@ok
<?php
require_once 'kphp_tester_include.php';

class MyJsonEncoder extends JsonEncoder {
    const float_precision = 5;
}

class AA {
    public float $a1 = 1.23456789;
    public float $a2 = 1;
    /** @var mixed */
    public $a3 = 0;
    /** @var mixed */
    public $a4 = 0.0;
    /** @var float[] */
    public $a5 = [-5.0, -6.0 - 1];
    /** @kphp-json float_precision = 3 */
    public float $a6 = -10;
}

echo JsonEncoder::encode(new AA, JSON_PRESERVE_ZERO_FRACTION), "\n";
echo MyJsonEncoder::encode(new AA, JSON_PRESERVE_ZERO_FRACTION), "\n";
echo MyJsonEncoder::encode(new AA, 0), "\n";

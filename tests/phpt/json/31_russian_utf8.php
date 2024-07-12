@ok k2_skip
<?php
// important! this file is saved as utf-8
require_once 'kphp_tester_include.php';


class A {
    public string $s = 'привет';
    public array $s_arr = ['как', 'дела', '😀'];
}

$json = JsonEncoder::encode(new A);
echo $json, "\n";
$restored_obj = JsonEncoder::decode($json, A::class);
var_dump(to_array_debug($restored_obj));

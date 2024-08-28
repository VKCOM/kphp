@ok k2_skip
<?php
// important! this file is saved as cp1251
require_once 'kphp_tester_include.php';

class A {
    public string $s = 'привет';
    public array $s_arr = ['как', 'дела'];
}

function test_noconv_in_kphp() {
    $json = JsonEncoder::encode(new A);
    #ifndef KPHP
    $json = iconv('cp1251', 'utf-8', $json);
    #endif
    $restored_obj = JsonEncoder::decode($json, A::class);
    #ifndef KPHP
    $restored_obj->s = (string)iconv('utf-8', 'cp1251', $restored_obj->s);
    #endif
    var_dump($restored_obj->s);
}

function test_conv_after() {
    $json = JsonEncoder::encode(new A);
    $json = vk_win_to_utf8($json);
    echo $json, "\n";
    $restored_obj = JsonEncoder::decode($json, A::class);
    var_dump($restored_obj->s);
}

function test_conv_before() {
    $a = new A;
    $a->s = vk_win_to_utf8($a->s);
    foreach ($a->s_arr as &$s)
        $s = vk_win_to_utf8($s);
    $json = JsonEncoder::encode($a);
    echo $json, "\n";
    $restored_obj = JsonEncoder::decode($json, A::class);
    var_dump(to_array_debug($restored_obj));
}

test_noconv_in_kphp();
test_conv_after();
test_conv_before();

@ok k2_skip
<?php
require_once 'kphp_tester_include.php';
require __DIR__ . '/include/MyJsonEncoder.php';

use VK\Utils;

class A {
  public int $pub_f;
  private int $priv_f = 0;

  function __construct(int $priv_f) {
    $this->pub_f = 42;
    $this->priv_f = $priv_f;
  }
}

class MyJsonEncoder extends \JsonEncoder {
  const rename_policy = 'camelCase';
}

function test_encoder_namespaces() {
  $json_public_fields1 = VK\Utils\MyJsonEncoder::encode(new A(1));
  $json_public_fields2 = Utils\MyJsonEncoder::encode(new A(2));
  $json_camel_case1 = MyJsonEncoder::encode(new A(3));
  $json_camel_case2 = \MyJsonEncoder::encode(new A(4));
  var_dump($json_public_fields1);
  var_dump($json_public_fields2);
  var_dump($json_camel_case1);
  var_dump($json_camel_case2);
  var_dump(to_array_debug(VK\Utils\MyJsonEncoder::decode($json_public_fields1, A::class)));
  var_dump(to_array_debug(Utils\MyJsonEncoder::decode($json_public_fields2, A::class)));
  var_dump(to_array_debug(MyJsonEncoder::decode($json_camel_case1, A::class)));
  var_dump(to_array_debug(\MyJsonEncoder::decode($json_camel_case2, A::class)));
}

test_encoder_namespaces();

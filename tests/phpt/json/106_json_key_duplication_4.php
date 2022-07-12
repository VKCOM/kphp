@kphp_should_fail
/Json key "a" appears twice for class A encoded with JsonEncoder/
<?php

class C {
  public string $c = "c_value";
}

class B extends C {
  public string $b = "b_value";
}

class A extends B {
  public string $a = "a_value";
  /** @kphp-json rename=a */
  public string $d = "d_value";
}

function test_json_key_duplication() {
  $json = JsonEncoder::encode(new A);
}

test_json_key_duplication();

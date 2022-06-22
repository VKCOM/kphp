@kphp_should_fail
/Json key "a" appears twice for class A encoded with JsonEncoder/
<?php

class C {
  /** @kphp-json rename=a */
  public string $c = "c_value";
}

class B extends C {
  public string $b = "b_value";
}

class A extends B {
  public string $a = "a_value";
}

function test_json_key_duplication_base_class() {
  $json = JsonEncoder::encode(new A);
}

test_json_key_duplication_base_class();

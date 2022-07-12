@kphp_should_fail
/Json decoding for Demo::\$arr is unavailable, because A1 has derived classes/
<?php

class A1 { public int $a1 = 1; }
class A2 extends A1 { public int $a2 = 2; }

/** @kphp-json visibility_policy = public */
class Demo {
  /**
   * @kphp-json skip = false
   * @var A1[]
   */
  private $arr;
}


JsonEncoder::decode('', Demo::class);

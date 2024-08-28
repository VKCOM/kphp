@ok k2_skip
<?php

class MyException1 extends \Exception {
  public $field1 = 'a';
}

class MyException2 extends \Exception {
  public $field2 = 'b';
}

function test(Exception $e) {
  try {
    throw $e;
  } catch (MyException1 $e1) {
    var_dump([__LINE__ => $e1->field1]);
  } catch (MyException2 $e2) {
    var_dump([__LINE__ => $e2->field2]);
  }
  var_dump(__LINE__);
}

test(new MyException1());
test(new MyException2());
try {
  test(new Exception());
} catch (Exception $e) {
  var_dump([__LINE__ => 'caught']);
}

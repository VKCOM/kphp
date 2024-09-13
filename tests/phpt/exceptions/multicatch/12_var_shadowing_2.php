@ok k2_skip
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function test($x) {
  /** @var Exception $caught_e */
  $caught_e = null;

  try {
    if ($x) {
      throw new MyException1('a');
    } else {
      throw new MyException2('b');
    }
  } catch (MyException1 $e) {
    var_dump($e->getLine());
    $caught_e = $e;
  } catch (MyException2 $e) {
    var_dump($e->getLine());
    $caught_e = $e;
  } catch (Exception $e) {
    var_dump($e->getLine());
    $caught_e = $e;
  }

  // unlike 11_var_shadowing, we use $caught_e here
  var_dump([__LINE__ => $caught_e->getMessage()]);
}

test(true);
test(false);

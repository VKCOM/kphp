@kphp_should_fail
/\$e is not an instance or it can't be detected/
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function test($x) {
  try {
    if ($x) {
      throw new MyException1('a');
    } else {
      throw new MyException2('b');
    }
  } catch (MyException1 $e) {
    var_dump($e->getLine());
  } catch (MyException2 $e) {
    var_dump($e->getLine());
  } catch (Exception $e) {
    var_dump($e->getLine());
  }

  // this does not compile, as $e exists only in scope of every catch and isn't visible outside
  // (btw, every $e in every catch clause is renamed to a unique name, so they don't intersect in assumptions)
  // see the test 12_var_shadowing_2 of a possible solution
  var_dump([__LINE__ => $e->getMessage()]);
}

test(true);
test(false);

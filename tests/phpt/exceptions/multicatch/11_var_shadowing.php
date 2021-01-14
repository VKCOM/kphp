@ok
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
  var_dump([__LINE__ => $e->getMessage()]);
}

test(true);
test(false);

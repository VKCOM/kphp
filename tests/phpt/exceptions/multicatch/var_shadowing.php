@ok
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function test($x) {
  try {
    if ($x) {
      throw new MyException1();
    } else {
      throw new MyException2();
    }
  } catch (MyException1 $e) {
    var_dump($e->getLine());
  } catch (MyException2 $e) {
    var_dump($e->getLine());
  } catch (Exception $e) {
    var_dump($e->getLine());
  }
}

test(true);
test(false);

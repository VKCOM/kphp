@ok
<?php

class MyException1 extends Exception {}
class MyException2 extends Exception {}

function test() {
  try {
    throw new MyException1();
  } catch (MyException1 $e) {
    var_dump($e->getLine());
  } catch (MyException2 $e) {
    var_dump($e->getLine());
  } catch (Exception $e) {
    var_dump($e->getLine());
  }
}

test();

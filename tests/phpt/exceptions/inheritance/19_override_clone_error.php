@kphp_should_fail
/You cannot override final method: MyException::__clone/
/Can not change access type for method: MyException::__clone/
<?php

class MyException extends Exception {
  public function __clone() {}
}

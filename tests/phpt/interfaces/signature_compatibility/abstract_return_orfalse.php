@ok
<?php

class BaseClass {
  /** @return string|false */
  public function getGroupName() {
    return false;
  }
}

class DerivedClass extends BaseClass {
  /** @return string */
  public function getGroupName() { return 'foo'; }
}

$v = new DerivedClass();
var_dump($v->getGroupName());

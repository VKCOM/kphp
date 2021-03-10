@ok
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

interface Iface {}

abstract class BaseBase implements Iface {}

abstract class Base extends BaseBase {
  /**
   * @param int $id
   * @param array $array
   * @return bool
   */
  abstract public function f($id, $array);
}

class Derived extends Base {
  /**
   * @param int $id
   * @param array $array
   * @return bool
   */
  public function f($id, $array) {
    return false;
  }
}

$d = new Derived();
$d->f(1, []);

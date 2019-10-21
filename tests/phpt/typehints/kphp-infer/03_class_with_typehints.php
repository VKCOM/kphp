@ok
<?php
/**
 * @kphp-infer
 */
class Base {
  /**
   * @param int $a
   */
  public function nohints($a) {
  }

  public function withhints(int $b, int $c) {
  }
}

new Base();


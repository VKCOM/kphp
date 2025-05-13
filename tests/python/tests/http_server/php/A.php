<?php

/** @kphp-immutable-class
 *  @kphp-serializable
*/
class A {
  /** @var int
   *  @kphp-serialized-field 0
  */
  public $a = 42;
  /** @var string
   *  @kphp-serialized-field 1
  */
  public $b = "hello";
}

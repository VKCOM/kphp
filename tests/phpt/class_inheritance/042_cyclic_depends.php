@ok
<?php

require_once 'kphp_tester_include.php';


function test_var() {
  class A1 {
      /** @var B1 */
      public $x = null;
  }

  class B1 extends A1 {
  }

  /** @var A1 */
  $b = new B1();
  var_dump(get_class($b));
}

function test_array() {
  class A2 {
      /** @var B2[] */
      public $x = [];
  }

  class B2 extends A2 {
  }

  /** @var A2 */
  $b = new B2();
  var_dump(get_class($b));
}

function test_tuple() {
  class A3 {
    function __construct() {
      $this->$x = tuple(null, 1);
    }

    /** @var tuple(B3,int) */
    public $x;
  }

  class B3 extends A3 {
    function __construct() {
      $this->$x = tuple(null, 1);
    }
  }

  /** @var A3 */
  $b = new B3();
  var_dump(get_class($b));
}

test_var();
test_array();
test_tuple();

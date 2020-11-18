@ok
<?php
require_once 'kphp_tester_include.php';

class AA {
  function bar(int $x) {
    var_dump($x);
    return $x;
  }

  /**
   * @return (callable():int)[]
   **/
  function foo(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo2(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo22(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo222(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo2222(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  function foo3() {
    $u = 12;
    $z = 87;
    /** callable(int):int */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo4() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo5() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo66() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
        $this->foo6($z);
        $this->foo5($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }
  function foo6() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
        $this->foo6($z);
        $this->foo5($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }
}
class A {
  function bar(int $x) {
    var_dump($x);
    return $x;
  }

  /**
   * @return (callable():int)[]
   **/
  function foo(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo2(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo22(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo222(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  /**
   * @return (callable():int)[]
   **/
  function foo2222(int $x) {
    return [
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() {
        return $this->bar(10);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      },
      function() use ($x) {
        return $this->bar($x);
      }
    ];
  }

  function foo3() {
    $u = 12;
    $z = 87;
    /** callable(int):int */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo4() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo5() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }

  function foo66() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
        $this->foo6($z);
        $this->foo5($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }
  function foo6() {
    $u = 12;
    $z = 87;
    /** @var callable():void */
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
        $this->foo6($z);
        $this->foo5($z);
    };

    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
    $x = function() use($u, $z) {
        $this->foo($u);
        $this->foo2($z);
    };
  }
}

function run() {
    $a = new A();
    $a->foo(777);
    $a->foo2(777);
    $a->foo22(777);
    $a->foo222(777);
    $a->foo2222(777);
    $a->foo3();
    $a->foo4();
    $a->foo5();
    $a->foo66();
}

function run2() {
    $a = new AA();
    $a->foo(777);
    $a->foo2(777);
    $a->foo22(777);
    $a->foo222(777);
    $a->foo2222(777);
    $a->foo3();
    $a->foo4();
    $a->foo5();
    $a->foo66();
}

run();
run2();

@kphp_should_fail
/lambda can't implement interface: IntCheckerInterface/
<?php

interface IntCheckerInterface {
  public function __invoke(int $x) : bool;
}

class ZeroChecker implements IntCheckerInterface {
  public function __invoke(int $x) : bool { return $x === 0; }
}

class OneChecker implements IntCheckerInterface {
  public function __invoke(int $x) : bool { return $x === 1; }
}

class SmartIntChecker implements IntCheckerInterface {
  public function __construct(int $v) {
    $this->v = $v;
  }

  public function __invoke(int $x) : bool { return $x === $this->v; }

  private $v = 0;
}

function demo(int $x) {
  /**@var (callable|IntCheckerInterface)[]*/
  $checkers = [
    new ZeroChecker(),
    new OneChecker(),
    new SmartIntChecker(2),
    function (int $x) : bool { return $x % 2 === 0; }
  ];

  foreach($checkers as $checker) {
    var_dump($checker($x));
  }

}

demo(123);

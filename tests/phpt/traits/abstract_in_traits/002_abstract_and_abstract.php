@ok
<?php

trait AT {
  abstract public function run(int $x): void;
}

trait BT {
  abstract public function run(int $x): void;
}

abstract class CT {
  use AT;
  use BT;
}

class CT2 {
    use AT;
    use BT;
    public function run(int $x): void { var_dump($x); }
}

class CT_D extends CT {
    public function run(int $x): void { var_dump($x); }
}

$ct = new CT_D();
$ct->run(10);

$ct2 = new CT2();
$ct2->run(10);

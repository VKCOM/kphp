@ok
<?php

trait AT {
  public function run(int $x): void { var_dump('123'); }
}

trait BT {
  abstract public function run(int $x): void;
}

class CT {
  use AT;
  use BT;
}

$ct = new CT();
$ct->run(10);

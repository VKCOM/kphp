@ok
<?php

trait AT {
  abstract protected function func(): void;
}

trait BT {
  protected function func(): void {
    var_dump('123');
  }

  public function func_dec(): void {
    $this->func();
  }
}

class CT {
  use BT;
  use AT;
}


$ct = new CT();
$ct->func_dec();

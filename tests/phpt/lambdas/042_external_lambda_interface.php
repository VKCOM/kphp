@ok
<?php
require_once 'kphp_tester_include.php';

class A {
  function bar(int $x) {
    var_dump($x);
  }

  /**
   * @kphp-infer
   * @return (callable():void)[]
   **/
  function foo(int $x) {
    return [
      function() {
        $this->bar(10);
      },
      function() use ($x) {
        $this->bar($x);
      },
    ];
  }
}

$a = new A();
$a->foo(777);

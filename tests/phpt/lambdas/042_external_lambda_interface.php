@ok
<?php
require_once 'polyfills.php';

function unused(Classes\LambdaInterface $i) {
}

if (0) {
  unused(null);
}

class A {
  function bar(int $x) {
    var_dump($x);
  }

  /**
   * @kphp-infer
   * @param $x int
   * @return (Classes\LambdaInterface|callable)[]
   **/
  function foo($x) {
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

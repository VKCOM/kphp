@ok
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
}

/**
 * @param int $y
 */
function test_primitive_capturing($y) {
    $f = function ($x) use ($y) {
        return $x + $y;
    };

    var_dump($f(100));
}

/**
 * @param int $x
 */
function test_returning_lambda_with_captured_values($x): callable {
    return function ($y) use ($x) {
        return $y + $x;
    };
}

function test_capturing_classes() {
    $x = new Classes\IntHolder(10);
    $f = function ($y) use ($x) {
        return $y + $x->a;
    };

    var_dump($f(100));
}

$x = 200;
test_primitive_capturing($x);
test_capturing_classes();
$callback = test_returning_lambda_with_captured_values(100);
var_dump($callback(20));

function test_capturing_already_captured_variables() {
    $x = 10;
    $f = function () use ($x) {
        $f2 = function () use ($x) {
            return $x;
        };

        return $f2();
    };

    var_dump($f());
}
test_capturing_already_captured_variables();


class WithFUse {
    function f() {
        $xx = 123;
        $f = function() use($xx) { echo $xx; };
        $f();
    }
    function f2() {
        $xx = 456;
        $f = fn() => $xx;
        echo $f(), "\n";
    }
    function f3(A $a) {
        $f1 = fn() => $a->fa();
        $f2 = function() use($a) { $a->fa(); };
        $f1();
        $f2();
    }
}
(new WithFUse)->f();
(new WithFUse)->f2();
(new WithFUse)->f3(new A);


class WithFCapturingDeep {
    function l2() {
        $xx = 111;
        $f = function() use($xx) {
            $g = function() use($xx) {
                echo $xx, "\n";
            };
            $g();
        };
        $f();
    }
    function l2_modif() {
        $xx = 123;
        $f = function() use($xx) {
            $xx++;
            $g = function() use($xx) {
                $xx++;
                echo $xx, "\n";
            };
            $g();
            $g();
            echo $xx, "\n";
        };
        $f();
        $f();
        echo $xx, "\n";
    }
}
(new WithFCapturingDeep)->l2();
(new WithFCapturingDeep)->l2_modif();

class Example020 {
  static private function takeInt(int $i) { echo $i; }

  public function unused_function(): void {
    $tmp_var = "";
    $this->call_function(function() use ($tmp_var): void {});
    $int = 10;
    $this->call_function(function() use ($int): void { self::takeInt($int); });
  }

  /** @param callable():void $fn */
  public function call_function(callable $fn): void {
    $fn();
  }
}

(new Example020())->call_function(function(): void {});

class Bxample020 {
  /**
   * @param callable(int):int $fn
   */
  public function unused_function(callable $fn): int {
    return $this->test2(function () use ($fn) {
      return $fn(1);
    });
  }

  /**
   * @param callable():int $fn
   */
  public function test2(callable $fn): int {
    return $fn();
  }
}

$bxample = new Bxample020();
$bxample->test2(function() { return 0; });


class Cxample {
  public function unused_function(): void {
    $tmp_var = "";
    $this->call_function_first(function() use ($tmp_var): void {
      $this->call_function_second(function() use ($tmp_var) {});
    });
  }

  /** @param callable():void $fn */
  public function call_function_first(callable $fn): void {
    $fn();
  }

  /** @param callable():void $fn */
  public function call_function_second(callable $fn): void {
    $fn();
  }
}

(new Cxample)->call_function_first(function():void{} );

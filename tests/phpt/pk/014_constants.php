@ok
<?php


class A {
  public function g() {
    echo "At method\n";
    echo "__FILE__: |", __FILE__, "|\n";
    echo "__DIR__: |", __DIR__, "|\n";
    echo "__FUNCTION__: |", __FUNCTION__, "|\n";
    echo "__CLASS__: |", __CLASS__, "|\n";
    echo "__METHOD__: |", __METHOD__, "|\n";
    echo "__LINE__: |", __LINE__, "|\n";
    echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
    $f = function() {
      echo "At lambda inside method\n";
      echo "__FILE__: |", __FILE__, "|\n";
      echo "__DIR__: |", __DIR__, "|\n";
      echo "__FUNCTION__: |", __FUNCTION__, "|\n";
      echo "__CLASS__: |", __CLASS__, "|\n";
      echo "__METHOD__: |", __METHOD__, "|\n";
      echo "__LINE__: |", __LINE__, "|\n";
      echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
    };
    $f();
  }
}

class B {
  public static function g() {
    echo "At static method\n";
    echo "__FILE__: |", __FILE__, "|\n";
    echo "__DIR__: |", __DIR__, "|\n";
    echo "__FUNCTION__: |", __FUNCTION__, "|\n";
    echo "__CLASS__: |", __CLASS__, "|\n";
    echo "__METHOD__: |", __METHOD__, "|\n";
    echo "__LINE__: |", __LINE__, "|\n";
    echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
  }
}
class C extends B {}

function f() {
  echo "At function\n";
  echo "__FILE__: |", __FILE__, "|\n";
  echo "__DIR__: |", __DIR__, "|\n";
  echo "__FUNCTION__: |", __FUNCTION__, "|\n";
  echo "__CLASS__: |", __CLASS__, "|\n";
  echo "__METHOD__: |", __METHOD__, "|\n";
  echo "__LINE__: |", __LINE__, "|\n";
  echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
}

echo "At global space\n";
echo "__FILE__: |", __FILE__, "|\n";
echo "__DIR__: |", __DIR__, "|\n";
echo "__FUNCTION__: |", __FUNCTION__, "|\n";
echo "__CLASS__: |", __CLASS__, "|\n";
echo "__METHOD__: |", __METHOD__, "|\n";
echo "__LINE__: |", __LINE__, "|\n";
echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";

f();
(new A())->g();
B::g();
C::g();

$f = function() {
  echo "At lambda on top level\n";
  echo "__FILE__: |", __FILE__, "|\n";
  echo "__DIR__: |", __DIR__, "|\n";
  echo "__FUNCTION__: |", __FUNCTION__, "|\n";
  echo "__CLASS__: |", __CLASS__, "|\n";
  echo "__METHOD__: |", __METHOD__, "|\n";
  echo "__LINE__: |", __LINE__, "|\n";
  echo "__NAMESPACE__: |", __NAMESPACE__, "|\n";
};
$f();

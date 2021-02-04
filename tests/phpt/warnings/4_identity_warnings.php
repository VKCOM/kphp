@kphp_should_warn
/Types A and int\[\] can.t be identical/
/Types A and int can.t be identical/
/Types A and bool can.t be identical/
/Types A and string can.t be identical/
/Identity operator between A and float types may give an unexpected result/
/Types A and int\[\] can.t be identical/
/Types A and int can.t be identical/
/Types A and bool can.t be identical/
/Types A and string can.t be identical/
/Identity operator between A and float types may give an unexpected result/
/Types int\[\] and A can.t be identical/
/Types int\[\] and int can.t be identical/
/Types int\[\] and bool can.t be identical/
/Types int\[\] and string can.t be identical/
/Identity operator between int\[\] and float types may give an unexpected result/
/Types int\[\] and A can.t be identical/
/Types int\[\] and int can.t be identical/
/Types int\[\] and bool can.t be identical/
/Types int\[\] and string can.t be identical/
/Identity operator between int\[\] and float types may give an unexpected result/
/Types int and A can.t be identical/
/Types int and int\[\] can.t be identical/
/Types int and bool can.t be identical/
/Types int and string can.t be identical/
/Identity operator between int and float types may give an unexpected result/
/Types int and A can.t be identical/
/Types int and int\[\] can.t be identical/
/Types int and bool can.t be identical/
/Types int and string can.t be identical/
/Identity operator between int and float types may give an unexpected result/
/Types bool and A can.t be identical/
/Types bool and int\[\] can.t be identical/
/Types bool and int can.t be identical/
/Types bool and string can.t be identical/
/Identity operator between bool and float types may give an unexpected result/
/Types bool and A can.t be identical/
/Types bool and int\[\] can.t be identical/
/Types bool and int can.t be identical/
/Types bool and string can.t be identical/
/Identity operator between bool and float types may give an unexpected result/
/Types string and A can.t be identical/
/Types string and int\[\] can.t be identical/
/Types string and int can.t be identical/
/Types string and bool can.t be identical/
/Identity operator between string and float types may give an unexpected result/
/Types string and A can.t be identical/
/Types string and int\[\] can.t be identical/
/Types string and int can.t be identical/
/Types string and bool can.t be identical/
/Identity operator between string and float types may give an unexpected result/
/Identity operator between float and A types may give an unexpected result/
/Identity operator between float and int\[\] types may give an unexpected result/
/Identity operator between float and int types may give an unexpected result/
/Identity operator between float and bool types may give an unexpected result/
/Identity operator between float and string types may give an unexpected result/
/Identity operator between float and A types may give an unexpected result/
/Identity operator between float and int\[\] types may give an unexpected result/
/Identity operator between float and int types may give an unexpected result/
/Identity operator between float and bool types may give an unexpected result/
/Identity operator between float and string types may give an unexpected result/
/Identity operator between float and float types may give an unexpected result/
/Identity operator between float and float types may give an unexpected result/

<?php

function test() {
  class A {
    public $x = 1;
  }

  $instance = new A;
  $array = [1, 2, 3];
  $integer = 1;
  $boolean = true;
  $string = "hello";
  $float = 1.2;

  var_dump($instance === $array);
  var_dump($instance === $integer);
  var_dump($instance === $boolean);
  var_dump($instance === $string);
  var_dump($instance === $float);

  var_dump($instance !== $array);
  var_dump($instance !== $integer);
  var_dump($instance !== $boolean);
  var_dump($instance !== $string);
  var_dump($instance !== $float);

  var_dump($array === $instance);
  var_dump($array === $integer);
  var_dump($array === $boolean);
  var_dump($array === $string);
  var_dump($array === $float);

  var_dump($array !== $instance);
  var_dump($array !== $integer);
  var_dump($array !== $boolean);
  var_dump($array !== $string);
  var_dump($array !== $float);

  var_dump($integer === $instance);
  var_dump($integer === $array);
  var_dump($integer === $boolean);
  var_dump($integer === $string);
  var_dump($integer === $float);

  var_dump($integer !== $instance);
  var_dump($integer !== $array);
  var_dump($integer !== $boolean);
  var_dump($integer !== $string);
  var_dump($integer !== $float);

  var_dump($boolean === $instance);
  var_dump($boolean === $array);
  var_dump($boolean === $integer);
  var_dump($boolean === $string);
  var_dump($boolean === $float);

  var_dump($boolean !== $instance);
  var_dump($boolean !== $array);
  var_dump($boolean !== $integer);
  var_dump($boolean !== $string);
  var_dump($boolean !== $float);

  var_dump($string === $instance);
  var_dump($string === $array);
  var_dump($string === $integer);
  var_dump($string === $boolean);
  var_dump($string === $float);

  var_dump($string !== $instance);
  var_dump($string !== $array);
  var_dump($string !== $integer);
  var_dump($string !== $boolean);
  var_dump($string !== $float);

  var_dump($float === $instance);
  var_dump($float === $array);
  var_dump($float === $integer);
  var_dump($float === $boolean);
  var_dump($float === $string);

  var_dump($float !== $instance);
  var_dump($float !== $array);
  var_dump($float !== $integer);
  var_dump($float !== $boolean);
  var_dump($float !== $string);

  var_dump($float === $float);
  var_dump($float !== $float);
}

test();

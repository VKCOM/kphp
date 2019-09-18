@kphp_should_fail
/Can't find definition for 'U'/
<?php

class Constants {
  const C = U;
  const B = "Hello";
}

class MyClass {
  const A = [Constants::B => Constants::C];
}

var_dump(MyClass::A);

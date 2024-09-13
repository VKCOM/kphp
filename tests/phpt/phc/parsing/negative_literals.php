@ok k2_skip
<?php
  var_dump(- - 0x7ffffffd);
  var_dump(- -0x7ffffffe);
  var_dump(- -0x7fffffff);

  var_dump(- -2147483645);
  var_dump(- -2147483646);
  var_dump(- -2147483647);

  var_dump(- - -0x7ffffffd);
  var_dump(- - -0x7ffffffe);
  var_dump(- - -0x7fffffff);
  var_dump(- - -0x80000000);

  var_dump(- - -2147483645);
  var_dump(- - -2147483646);
  var_dump(- - -2147483647);
  var_dump(- - -2147483648);

  // octal from 41118
  var_dump(- -017777777777);
  var_dump(- - -017777777777);
  var_dump(- - -020000000000);

  var_dump(-9223372036854775807);
  var_dump(- -9223372036854775807);
  var_dump(- - -9223372036854775807);

  var_dump(PHP_INT_MIN);
  var_dump(abs(PHP_INT_MIN + 1));

#ifndef KPHP
  var_dump(-PHP_INT_MAX - 1);
  var_dump(-PHP_INT_MAX - 1);
  var_dump(-PHP_INT_MAX - 1);
  return;
#endif
  // это все одно и тоже, так как переполнение
  var_dump(-9223372036854775808);
  var_dump(- -9223372036854775808);
  var_dump(- - -9223372036854775808);
?>

@ok
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
?>

@ok
<?php
  var_dump($x = base_convert("adj7sdf7d", 35, 9));
  var_dump(base_convert($x, 11, 23));
  var_dump(base_convert($x, 16, 10));

  $x = substr ($x, 1);

  var_dump(base_convert($x, 35, 9));
  var_dump(base_convert($x, 11, 23));
  var_dump(base_convert($x, 16, 10));

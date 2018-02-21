@ok_old_php
<?php
  $s = "  12312312";

  var_dump (isset($s[-10]));
  var_dump ($s);

  var_dump (isset($s[9]));
  var_dump ($s);

  unset ($t);
  var_dump (isset($s[$t]));
  var_dump ($s);

  var_dump (isset($s[false]));
  var_dump ($s);

  var_dump (isset($s[3.5]));
  var_dump ($s);

  @var_dump (isset($s["2asdasd"]));
  var_dump ($s);



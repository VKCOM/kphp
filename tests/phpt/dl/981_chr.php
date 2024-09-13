@ok k2_skip
<?php
  for ($i = -300; $i <= 300; $i++) {
    if ($i) {
      echo chr ($i);
      echo ord (chr ($i));
    }
  }
  var_dump (chr(0) == '');
  var_dump (chr(10));
  var_dump (chr(50));
  var_dump (chr(60));
  var_dump (chr(90));

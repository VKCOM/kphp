@ok benchmark
<?php
  for ($i = 0; $i < 256; $i++)
    $s_full .= chr ($i);
  var_dump (addcslashes ($s_full, "\0..\xff"));
  var_dump (addcslashes ($s_full, " \n\r\t\v\0"));
  var_dump (addcslashes ($s_full, ""));
  var_dump (addslashes ($s_full));
  var_dump (stripslashes (addslashes ($s_full)));
  var_dump (stripslashes (" \n\r\t\v\0"));
  
  $s = str_repeat ($s_full, 100);

//  for ($i = 0; $i < 10000; $i++) {
//    $res = addcslashes ($s, " \n\r\t\v\0");
//    $res = addcslashes ($s, "\0..\xff");
//  }

//  $s = str_repeat ("'\"\\a", 6400);
  $s = addslashes ($s).'\n\r\b\a\f\q\1\012\0\\\\\\';

  for ($i = 0; $i < 10000; $i++) {
    $res = stripslashes ($s);
  }

  var_dump ($res);

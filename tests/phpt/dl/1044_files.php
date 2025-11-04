@ok
<?php
  var_dump (basename ('..'));
  var_dump (basename ('phpt/'));
  var_dump (basename ('phpt'));
  var_dump (basename ('abacaba'));
  var_dump (basename ('.'));
  var_dump (basename ('/etc/sudoers.d', '.d'));

  $filename = "12341234124123";

  $large_text = str_repeat("s", 1000);
  $large_text2 = str_repeat("t", 1000);
  var_dump (file_put_contents($filename, $large_text));
  var_dump (file_put_contents($filename, $large_text2, 1)); // append
  $file = fopen ($filename, 'r');
  var_dump (fread($file, 2000));
  fclose ($file);


  $file = fopen ($filename, 'w');
  var_dump (fwrite($file, $large_text));
  fclose ($file);
  $file = fopen ($filename, 'a');
  var_dump (fwrite($file, $large_text2));
  fclose ($file);
  $file = fopen ($filename, 'r');
  var_dump (fread($file, 2000));
  fclose ($file);
// TODO    @var_dump (unlink ($filename));

  $file = fopen ($filename, 'r');
  var_dump (fread($file, 1));
  fclose ($file);


  var_dump (realpath ("../dl/473-file.php"));
  var_dump (realpath ("../dl/473_file.php"));
  var_dump (realpath ("phpt/dl/473-file.php"));
  var_dump (realpath ("phpt/dl/473_file.php"));


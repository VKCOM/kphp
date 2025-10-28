@ok
<?php
  var_dump (basename ('..'));
  var_dump (basename ('phpt/'));
  var_dump (basename ('phpt'));
  var_dump (basename ('abacaba'));
  var_dump (basename ('.'));
  var_dump (basename ('/etc/sudoers.d', '.d'));

  $filename = "12341234124123";
/*
  var_dump ($filename);
  $file = fopen ($filename, "w");
  fwrite ($file, "1232225\n");
  if (fwrite ($file, "qwreqer") != 7) {
    echo "error1\n";
  }
  if (fwrite ($file, "") !== 0) {
    echo "error2\n";
  }
  if (@(fwrite ("asdasdasd", "") !== false)) {
    echo "error3\n";
  }
  fflush ($file);
  fwrite ($file, "asdfdaf\n");
  fwrite ($file, "asdfdaf");
  fflush ($file);
  var_dump (fread ($file, 100));
  fclose ($file);
  if (@(fwrite ($file, "1231") !== false)) {
    echo "error4\n";
  }

  fwrite (STDERR, "Not a part of the answer\n");

  $file = fopen ($filename, "rb");

  var_dump (file_get_contents ($filename));
  @var_dump (file_put_contents ('tmp', 'test'));
  var_dump (file_put_contents ($filename, "test-test-test"));
  var_dump (file_get_contents ($filename));
  var_dump (file_put_contents ($filename, ''));
  fclose($file);
 */


  $large_text = str_repeat("s", 3000);
  var_dump (file_put_contents($filename, $large_text));
  $file = fopen ($filename, 'r');
  fclose ($file);
  @var_dump (file_get_contents ($filename));
  @var_dump ($filename);
  @var_dump (unlink ($filename));
  @var_dump (file_get_contents ($filename));

  $append_text = str_repeat("t", 30);
  var_dump (file_put_contents($filename, $append_text, 1));
  @var_dump (file_get_contents ($filename));

  @var_dump (realpath ("../dl/473-file.php"));
  @var_dump (realpath ("../dl/473_file.php"));
  @var_dump (realpath ("phpt/dl/473-file.php"));
  @var_dump (realpath ("phpt/dl/473_file.php"));


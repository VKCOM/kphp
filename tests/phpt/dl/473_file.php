@ok k2_skip
<?php
  var_dump (dirname ('..'));
  var_dump (dirname ('phpt/'));
  var_dump (dirname ('phpt'));
  var_dump (dirname ('abacaba'));
  var_dump (dirname ('.'));
  var_dump (dirname ('/etc/sudoers.d'));

  var_dump (basename ('..'));
  var_dump (basename ('phpt/'));
  var_dump (basename ('phpt'));
  var_dump (basename ('abacaba'));
  var_dump (basename ('.'));
  var_dump (basename ('/etc/sudoers.d', '.d'));

  $filename = tempnam ("", "rt");
  var_dump (is_file ($filename));
//  var_dump ($filename);
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
//  var_dump (fread ($file, 100));
  fclose ($file);
  if (@(fwrite ($file, "1231") !== false)) {
    echo "error4\n";
  }

  clearstatcache();
  var_dump (filesize ($filename));
  var_dump (filectime ($filename) > 0);
  var_dump (filemtime ($filename) > 0);

  $new_filename = tempnam ("", "rt");
  rename ($filename, $new_filename);
  $filename = $new_filename;

  fwrite (STDERR, "Not a part of the answer\n");
  
  var_dump (filesize ($filename));
  var_dump (filectime ($filename) > 0);
  var_dump (filemtime ($filename) > 0);
  $file = fopen ($filename, "rb");
  var_dump (fread ($file, 2));
  var_dump (ftell ($file));
  var_dump (fpassthru ($file));
  var_dump (fread ($file, 100));
  var_dump (rewind ($file));
  var_dump (fseek ($file, 5, SEEK_SET));
  var_dump (fpassthru ($file));
  fclose ($file);
  var_dump (filesize ($filename));
  var_dump (filectime ($filename) > 0);
  var_dump (filemtime ($filename) > 0);

  $file = fopen ($filename, "rb");
  var_dump (fgets ($file, 100));
  var_dump (fgets ($file, 2));
  var_dump (fgets ($file));
  var_dump (rewind ($file));
  var_dump (fgets ($file));
  var_dump (fgetc ($file));
  var_dump (fgetc ($file));
  var_dump (fgets ($file));
  var_dump (fgetc ($file));
  var_dump (fgetc ($file));
  var_dump (fgets ($file));
  var_dump (fgetc ($file));
  var_dump (fgetc ($file));
  var_dump (fgets ($file));
  var_dump (fgetc ($file));

  var_dump (file_get_contents ($filename));
  var_dump (file ($filename));
  @var_dump (file_put_contents ('tmp', 'test'));
  var_dump (file_put_contents ($filename, "test-test-test"));
  var_dump (file_get_contents ($filename));
  var_dump (file_put_contents ($filename, ''));
  fclose($file);

  $large_text = str_repeat("s", 3000);
  var_dump (file_put_contents($filename, $large_text));
  $file = fopen ($filename, 'r');
  $large_line = fgets ($file);
  var_dump (strlen ($large_line));
  fclose ($file);
  unlink ($filename);

  @var_dump (realpath ("../dl/473-file.php"));
  @var_dump (realpath ("../dl/473_file.php"));
  @var_dump (realpath ("phpt/dl/473-file.php"));
  @var_dump (realpath ("phpt/dl/473_file.php"));


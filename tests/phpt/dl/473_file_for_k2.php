@ok
<?php
  var_dump (basename ('..'));
  var_dump (basename ('phpt/'));
  var_dump (basename ('phpt'));
  var_dump (basename ('abacaba'));
  var_dump (basename ('.'));
  var_dump (basename ('/etc/sudoers.d', '.d'));

  $filename = '/tmp/' . rand(1, 2e9);
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


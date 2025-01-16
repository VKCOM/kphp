@ok benchmark k2_skip
<?php
#ifndef KPHP
  function crc32_file ($x) {
    return crc32 (file_get_contents ($x));
  }
#endif

function test_hash_files() {
  var_dump (md5_file ("/bin/ls"));
  var_dump (md5_file ("/bin/ls", true));

  var_dump (dechex (crc32_file ("/bin/ls")));
}

test_hash_files();

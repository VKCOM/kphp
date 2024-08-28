@ok benchmark k2_skip
<?php
  $strs = array (" ", "\n", "", " \n", "\r ", "    aasddsd", "asdasdasd     ", "\n\n\n\n\r  asdas  d s   d  \0  fasd    \0\0\0\0\0", "asdasdasdasdasdasdasdas", "A",
                 "%aba", "%%%", "%xyz", "%", "%+asa", "+%aba", "++++");
  foreach ($strs as $s) {
    var_dump ($s);
    var_dump (rawurlencode ($s));
    var_dump (rawurldecode ($s));
    var_dump (urlencode ($s));
    var_dump (urldecode ($s));
    var_dump (rawurldecode (rawurlencode ($s)));
    var_dump (rawurldecode (urlencode ($s)));
    var_dump (urldecode (rawurlencode ($s)));
    var_dump (urldecode (urlencode ($s)));
  }



  $s = str_repeat ("T%h  i\ts\r\ni!@s\n\ra\nstring\r\r#$%^&*()", 100);
//  $s = rawurlencode ($s);
  for ($i = 0; $i < 10000; $i++) {
    $res = urlencode ($s);
  }

  var_dump ($res);
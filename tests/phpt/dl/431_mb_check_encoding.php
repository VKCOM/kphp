@ok
<?php
  $a = array('<foo>',"'bar'",'"baz"','&blong&', "\xc3\xa9", "\xF4\x8F\xBF\xBF", "\xF4\x90\x80\x80", "\xc3\xa9\xF4\x8F\xBF\xBF\xF4\x90\x80\x80", 
             "\xD0\x9A", "\xE0\x90\x9A", "\xF0\x80\x90\x9A", "\xF8\x80\x80\x90\x9A", "\xFC\x80\x80\x80\x90\x9A",//character and its overlong encodings
             "\xE0\xAF\xB5", "\xED\x9F\xBF", "\xED\xA0\x80", "\xED\xBF\xBF", "\xEE\x80\x80", "\xF0\xA6\x88\x98", "\xF4\x8F\xBF\xBF",
             "\xF4\x90\x80\x80", "\xF8\xAA\xBC\xB7\xAF", "\xFD\xBF\xBF\xBF\xBF\xBF");

  foreach ($a as $s) {
    echo $s.": ";
    var_dump (mb_check_encoding ($s, "UTF-8"));
    if (mb_check_encoding ($s, "UTF-8")) {
      $t .= $s;
    } else {
      $t2 .= $s;
    }
  }
  var_dump (mb_check_encoding ($t, "UTF-8"));
  var_dump (mb_check_encoding ($t.$t2, "UTF-8"));

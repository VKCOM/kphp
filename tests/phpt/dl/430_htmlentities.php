@ok benchmark k2_skip
<?php
  for ($i = 32; $i < 128; $i++)
    $s .= chr ($i);

  $s = str_repeat ($s, 2);

  var_dump (htmlentities ($s));
  var_dump (html_entity_decode (htmlentities ($s)));
  var_dump (html_entity_decode ('&zz;'));

  var_dump (htmlspecialchars ($s));
  var_dump (htmlspecialchars ($s, ENT_NOQUOTES));
  var_dump (htmlspecialchars ($s, ENT_QUOTES));
  var_dump (htmlspecialchars ($s, ENT_COMPAT));
  var_dump (htmlspecialchars_decode (htmlspecialchars ($s)));

  var_dump (htmlspecialchars_decode (htmlspecialchars ($s, ENT_QUOTES)));
  var_dump (htmlspecialchars_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_NOQUOTES));
  var_dump (htmlspecialchars_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_QUOTES));
  var_dump (htmlspecialchars_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_COMPAT));

  var_dump (html_entity_decode (htmlspecialchars ($s, ENT_QUOTES)));
  var_dump (html_entity_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_NOQUOTES));
  var_dump (html_entity_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_QUOTES));
  var_dump (html_entity_decode (htmlspecialchars ($s, ENT_QUOTES), ENT_COMPAT));

  $s = str_repeat ($s, 10);
//  $s = str_repeat ("'\"", 1000);

  $s = "&q&quot&ampquot;".htmlspecialchars ($s, ENT_QUOTES)."&";

  for ($i = 0; $i < 10000; $i++) {
    $res = html_entity_decode ($s, ENT_QUOTES);
  }

  var_dump ($res);

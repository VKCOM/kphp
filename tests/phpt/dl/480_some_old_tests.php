@ok benchmark
<?php

// ---------------------

  var_dump (inet_pton('127.0.0.1'));
  var_dump (inet_pton('::1'));
  var_dump (inet_pton('172.27.1.04'));
  var_dump (inet_pton('172.27.1.4'));

// ---------------------
  var_dump (fmod (4.0, -1.5));
  var_dump (fmod (372, 360));
  var_dump (fmod (-372, 360));
  var_dump (fmod (-372, -360));

  var_dump (atan2 (0.0, 0.0));
  var_dump (sqrt (4.0));
  var_dump (log (1.0));
  var_dump (log (1.0, 2));
  var_dump (round (log(2.0), 7));
  var_dump (log (2.0, 2));
  var_dump (log (exp (3.5)));

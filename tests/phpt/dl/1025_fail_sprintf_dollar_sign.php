@ok k2_skip
<?php

#ifndef KPHP
if (0)
#endif
  register_kphp_on_warning_callback(function ($message, $bt) {
    var_dump($message === 'Wrong parameter number 0 specified in function sprintf with format "%$s contains %1$d"');
  });

echo sprintf('%$s contains %1$d', 10);

#ifndef KPHP
var_dump(true);
#endif

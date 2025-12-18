@ok benchmark
<?php

  var_dump (ip2long ('192.0.34.166'));
  var_dump (long2ip (ip2long ('192.0.34.166')));

  var_dump (ip2long('255.255.255.255'));
  var_dump (long2ip (ip2long('255.255.255.255')));

  var_dump (inet_pton('127.0.0.1'));
  var_dump (inet_pton('::1'));
  var_dump (inet_pton('172.27.1.04'));
  var_dump (inet_pton('172.27.1.4'));

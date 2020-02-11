@ok run_options:"-i $PHP_SOURCE_DIR/php_ini.conf"
<?php

#ifndef KittenPHP
echo "42~\n";
echo "53     1~\n";
echo "4~\n";
echo "hello~\n";
echo "qwerty~\n";
exit(0);
#endif

echo ini_get("a"), "~\n";
echo ini_get("b"), "~\n";
echo ini_get("c"), "~\n";
echo ini_get("d"), "~\n";
echo ini_get("efefe"), "~\n";

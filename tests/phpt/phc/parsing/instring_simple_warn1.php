@kphp_should_warn
/Simple string expressions with \[\] can work wrong\. Use more \{\}/
<?php

$arr = [1, 2, 3];

echo "$arr[0][1]\n";

@kphp_should_fail
/Bad numeric constant, '_' cannot go after '0x'/
<?php

echo 0x_FFF_AAA;

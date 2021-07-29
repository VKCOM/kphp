@kphp_should_fail
/Bad numeric constant, '_' cannot go before 'e' or 'E'/
<?php

echo 10_e10;

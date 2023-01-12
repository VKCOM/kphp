@kphp_should_fail php8
/Expected expression before token '=>'/
<?php

match(10) {
   => "ok",
};

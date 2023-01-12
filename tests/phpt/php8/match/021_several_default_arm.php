@kphp_should_fail php8
/Match expressions may only contain one default arm/
<?php

match(10) {
   default => "default",
   default => "default",
};

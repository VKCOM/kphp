@kphp_should_fail php8
Match: several `default` cases found"
<?php

match(10) {
   default => "default",
   default => "default",
};

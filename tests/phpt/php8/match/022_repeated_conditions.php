@kphp_should_fail php8
/Repeated case \[10\] in switch\/match/
<?php

match(10) {
   10, 20 => "1",
   10 => "2",
   default => "default",
};

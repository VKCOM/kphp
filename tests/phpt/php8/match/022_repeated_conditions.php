@kphp_should_fail php8
/Switch: repeated cases found \[10\]/
<?php

match(10) {
   10, 20 => "1",
   10 => "2",
   default => "default",
};

@kphp_runtime_should_warn php8
Error: unhandled value in match!
<?php

match(100) {
   10, 20 => "1",
   30 => "2",
};

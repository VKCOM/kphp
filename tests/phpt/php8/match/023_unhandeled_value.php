@kphp_runtime_should_warn php8
/Unhandled match value '100'/
<?php

match(100) {
   10, 20 => "1",
   30 => "2",
};

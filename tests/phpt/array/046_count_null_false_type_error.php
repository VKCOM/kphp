@kphp_should_fail
/Cannot call count\(\) with non-countable type null/
/Cannot call count\(\) with non-countable type false/
<?php

var_dump(count(null));
var_dump(count(false));

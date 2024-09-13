@kphp_should_fail k2_skip
/pass int\[]\ to argument \$value of instance_cache_store/
<?php

instance_cache_store("key", [1, 2]);

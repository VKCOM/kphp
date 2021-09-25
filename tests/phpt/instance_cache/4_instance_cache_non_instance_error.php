@kphp_should_fail
/Called instance_cache_store\(\) with a non-instance argument/
<?php

instance_cache_store("key", [1, 2]);

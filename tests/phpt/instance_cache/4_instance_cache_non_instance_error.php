@kphp_should_fail
/Can not store non\-instance var with instance_cache_store call/
<?php

instance_cache_store("key", [1, 2]);

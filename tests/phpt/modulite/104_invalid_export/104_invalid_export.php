@kphp_should_fail
KPHP_ENABLE_MODULITE=1
/'require' contains a member of @utils; you should require @utils directly, not its members/
/"\\\\A"/
/'export' contains a symbol that does not belong to @algo/
/Failed loading Utils104\/.modulite.yaml:/
/'force-internal' contains a symbol which is exported/
/'force-internal' can contain only class members/
/'force-internal' contains a symbol that does not belong to @utils/
/'export' of @utils lists a non-child @algo/
<?php

class A {
    const ONE = 1;
}

Utils104\Strings104::doSmth();
Algo104\Sort104::doSmth();

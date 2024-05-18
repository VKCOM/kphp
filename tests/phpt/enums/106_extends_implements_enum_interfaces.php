@kphp_should_fail
/Interface E cannot extend UnitEnum or BackedEnum/
/Class A cannot implement UnitEnum or BackedEnum/
<?php

interface E implements \UnitEnum {
}

class A extends BackedEnum {
}

$v = new A();

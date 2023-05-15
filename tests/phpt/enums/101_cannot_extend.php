@kphp_should_fail
/Enums cannot extend/
<?php

class S {};

enum MyBool extends S {
    case T;
    case F;
}

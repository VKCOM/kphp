@kphp_should_fail
<?php

enum MyBool {
    case T;
    case F;
}

class A extends MyBool {}

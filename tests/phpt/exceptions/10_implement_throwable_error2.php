@kphp_should_fail
/Interface MyThrowable cannot extend Throwable/
<?php

interface MyThrowable extends Throwable {}

class MyException implements MyThrowable {}

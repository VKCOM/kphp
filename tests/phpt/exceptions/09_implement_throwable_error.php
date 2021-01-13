@kphp_should_fail
/Class MyException cannot implement Throwable, extend Exception instead/
<?php

class MyException implements Throwable {}

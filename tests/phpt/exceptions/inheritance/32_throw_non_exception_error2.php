@kphp_should_fail
/Throw not Throwable, but NonException/
<?php

class NonException {}

throw new NonException();

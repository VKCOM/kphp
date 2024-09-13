@kphp_should_fail k2_skip
/Throw not Throwable, but NonException/
<?php

class NonException {}

throw new NonException();

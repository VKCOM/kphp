@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php

class NonException {}

throw new NonException();

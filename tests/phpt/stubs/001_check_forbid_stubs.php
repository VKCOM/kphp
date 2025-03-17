@kphp_should_fail
/try call generated_stub stub that is prohibited by option/
KPHP_FUNCTIONS=001_functions.txt
KPHP_FORBID_STUBS_USING=1
<?php

generated_stub();

@kphp_should_fail
/generation required can be used only for stubs/
KPHP_FUNCTIONS=002_functions.txt
KPHP_FORBID_STUBS_USING=1
<?php

// Tag generation-required is available only after indication stub
// See difference between generated_stub declaration in 001_functions.txt and 002_functions.txt

generated_stub();

@kphp_should_fail
/Throw expression is not an instance or it can't be detected/
<?php

function get_exception() {
  return new Exception();
}

throw get_exception();

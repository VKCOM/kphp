@kphp_should_fail
/Expected 'tok_func_name', found 'fn'/
<?php

// This test should be @ok, but we don't handle semireserved keywords
// in class member names correctly yet
//
// TODO: remove this test when this issue is resolved
// and uncomment tests in arrowfunc_semireserved.php

class Example {
  public function fn() {}
}

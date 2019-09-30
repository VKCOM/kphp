@ok
<?php
  function f() {
    func_get_args();
  }

  f (new Exception());
//  var_dump (new Exception());

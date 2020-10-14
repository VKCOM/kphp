@ok
<?php

  function regGenerateUniqueId($flag) {
    for ($i = 1; $i < 2; $i++) {
      $member_id = 1;

      if ($flag) {
        $member_id = false;
        continue;
      }
      break;
    }
    return $member_id;
  }

  var_dump (regGenerateUniqueId (true));
  var_dump (regGenerateUniqueId (false));

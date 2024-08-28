@ok k2_skip
<?php

$s = str_repeat(".", (1 << 24) * 10); 
try {
#ifndef KPHP
      throw new Exception();
#endif
    vk_json_encode_safe($s);
} catch (Exception $e) {
    var_dump("catch Exception");
}

@ok benchmark k2_skip
<?php
  $s = "This\r\nis\n\ra\nstring\radasdasdasdasdasdasjgjhsdgf4 6r34 t5 t54 t 54g 4g54 t 54 t 54 t 54  5534344r 34f34 f34fg 34tg35t5 t5tjsdgfhasdgasdgfjhgryt4512f14t4g5r3465f675s67f5sd675fas685gf78gb6df79sb6sdg96hsdg796hg6hfg876hfg7h6fg78h6fg78h6fg78h6fg78h6gf786hfg786hfg7h6fg786hg78h67h6fg786\n";
  $s = str_repeat ("\r\r", 100);
  $s = str_repeat ("This\r\nis\n\ra\nstring\r\r", 20);
  for ($i = 0; $i < 100000; $i++) {
    $res = nl2br ($s, false);
  }

  var_dump ($res);

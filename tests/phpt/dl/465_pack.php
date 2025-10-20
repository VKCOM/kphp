@ok
<?php
  var_dump (pack ("A5a4Ah2H5H2H3h", "h h", 'q q', 't t', 345, "ABCDE", "ABCDE", "ABCDEF", "A1"));
  var_dump (pack ("A5a4Ah3H5H4H6", "h h", 'q q', 't t', 345, "ABCDE", "ABCDE", "ABCDEF"));
  var_dump (pack ("A*a*A*h*H*H*H*", "h h", 'q q', 't t', 345, "ABCDE", "ABCDE", "ABCDEF"));

  var_dump (pack ("cCsSnv", -1, -2, -3, -4, -5, -6));
  var_dump (pack ("cCsSnv", 1, 2, 3, 4, 5, 6));
  var_dump (pack ("cCsSnv", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("c2C4", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("s2S4", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("nN5", -123456, -234567, -345678, -456789, -567890, -678901));

  $nan = acos(8);
  $floats = [0.0, 0.1, -0.1, 0.4932813, 1.0, -1.0, 1391212.2421, 9320.1828284300211, -8183.88129305222, $nan];
  foreach ($floats as $x) {
    $packed_e = pack('e', $x);
    $packed_E = pack('E', $x);
    $packed_d = pack('d', $x);
    var_dump(explode($packed_e, ''), explode($packed_E, ''), explode($packed_d, ''));
    var_dump(unpack('e', $packed_e));
    var_dump(unpack('E', $packed_E));
    var_dump(unpack('d', $packed_d));
  }

  var_dump (pack ("iIlLNV", -1, -2, -3, -4, -5, -6));
  var_dump (pack ("iIlLNV", 1, 2, 3, 4, 5, 6));
  var_dump (pack ("iIlLNV", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("i2L4", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("I2N4", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("lV5", -123456, -234567, -345678, -456789, -567890, -678901));

  var_dump (pack ("c*", -1, -2, -3, -4, -5, -6));
  var_dump (pack ("cC*", 1, 2, 3, 4, 5, 6));
  var_dump (pack ("cCs*", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("c2C*", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("s10c*", -123456, -234567, -345678, -456789, -567890, -678901, 1, 2, 3, 4, 5, 6));
  var_dump (pack ("n*", -123456, -234567, -345678, -456789, -567890, -678901));
  var_dump (pack ("nnN*", -123456, -234567, -345678, -456789, -567890, -678901));

  var_dump (pack ("VVVV3", -1, -2, -3, -4, -5, -6));
  var_dump (pack ("NNNN3", 1, 2, 3, 4, 5, 6));

  var_dump (pack ("f3d3", -1, -2, -3, -4, -5, -6));
  var_dump (pack ("f3d3", 1, 2, 3, 4, 5, 6));

  var_dump (unpack ("h1q/h1w", "AA"));
  var_dump (unpack ("hq/hw", "AA"));
  var_dump (unpack ("hq/h*", "AABCDE"));
  var_dump (unpack ("hq/A*", "AABCDE"));

  var_dump (unpack ("cq/Cw/se/Se/nt/vy", pack ("cCsSnv", -1, -2, -3, -4, -5, -6)));
  var_dump (unpack ("cq/Cw/se/Sr/nt/vy", pack ("cCsSnv", 1, 2, 3, 4, 5, 6)));
  var_dump (unpack ("cq/Cw/se/Sr/nt/vy", pack ("cCsSnv", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("c3q/C3y", pack ("c2C4", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("s2a/S4s", pack ("s2S4", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("nn/N5N", pack ("nN5", -123456, 234567, 345678, 456789, 567890, 678901)));

  var_dump (unpack ("iqwe/Iq22w2w22w2w2sd/lz123123/Lx11/Nqwe/V", pack ("iIlLNV", 1, 2, 3, 4, 5, 6)));
  var_dump (unpack ("i2q/L4w", pack ("i2L4", -123456, -234567, 345678, 456789, 567890, 678901)));
  var_dump (unpack ("lq/V5w", pack ("lV5", -123456, 234567, 345678, 456789, 567890, 678901)));

  var_dump (unpack ("c*", pack ("c*", -1, -2, -3, -4, -5, -6)));
  var_dump (unpack ("c/C*", pack ("cC*", 1, 2, 3, 4, 5, 6)));
  var_dump (unpack ("cq/Cqwe/s*", pack ("cCs*", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("c2q/C*w", pack ("c2C*", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("s10qwe/c*", pack ("s10c*", -123456, -234567, -345678, -456789, -567890, -678901, 1, 2, 3, 4, 5, 6)));
  var_dump (unpack ("n*", pack ("n*", -123456, -234567, -345678, -456789, -567890, -678901)));
  var_dump (unpack ("nq/nn/N*N", pack ("nnN*", -123456, -234567, 345678, 456789, 567890, 678901)));

  var_dump (unpack ("V6", pack ("VVVV3", 1, 2, 3, 4, 5, 6)));
  var_dump (unpack ("N6", pack ("NNNN3", 1, 2, 3, 4, 5, 6)));

  var_dump (unpack ("f3q/d3w", pack ("f3d3", -1, -2, -3, -4, -5, -6)));
  var_dump (unpack ("d2q/f4w", pack ("d2f4", 1, 2, 3, 4, 5, 6)));

  var_dump (unpack("cchars/nint", "\x04\x00\xa0\x00"));
  var_dump (unpack("c2chars/nint", "\x04\x00\xa0\x00"));
  var_dump (unpack("c2/n", "\x32\x42\x00\xa0"));

  var_dump(unpack("00V6", "\x04\x00\xa0\x00"));
  var_dump(unpack("H", ""));
  var_dump(unpack("c*2/n", "\x04\x00\xa0\x00"));
  var_dump(unpack("\e", "\x04\x00\xa0\x00"));

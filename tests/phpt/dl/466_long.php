@ok
<?php
#ifndef KittenPHP
  function ladd ($x, $y) {
    return longval (bcadd ($x, $y));
  }

  function lmul ($x, $y) {
    return longval (bcmul ($x, $y));
  }

  function ldiv ($x, $y) {
    return longval (bcdiv ($x, $y, 0));
  }

  function lmod ($x, $y) {
    return longval (bcmod ($x, $y));
  }

  function lsub ($x, $y) {
    return longval (bcsub ($x, $y));
  }

  function labs ($x) {
    return abs ($x);
  }

  function lpow ($x, $y) {
    $res = 1;
    while ($y--) {
      $res = lmul ($res, $x);
    }
    return $res;
  }

  function lshl ($x, $y) {
    return lmul ($x, lpow (2, $y));
  }

  function lshr ($x, $y) {
    return ldiv ($x, lpow (2, $y));
  }

  function lnot ($x) {
    return ~intval ($x);
  }

  function lxor ($x, $y) {
    return intval ($x) ^ intval ($y);
  }

  function lor ($x, $y) {
    return intval ($x) | intval ($y);
  }

  function land ($x, $y) {
    return intval ($x) & intval ($y);
  }

  function lcomp ($x, $y) {
    return intval ($x) < intval ($y) ? -1 : (intval ($x) > intval ($y) ? 1 : 0);
  }

  function longval ($x) {
    if ($x[0] == '0' && $x[1] == 'x') {
      $x = base_convert (substr ($x, 2), 16, 10);
    }
    $x = bcmod ($x, "18446744073709551616");
    if (bccomp ($x, "9223372036854775807") == 1) {
      $x = bcsub ($x, "18446744073709551616");
    } else if (bccomp ($x, "-9223372036854775808") == -1) {
      $x = bcadd ($x, "18446744073709551616");
    }
    return $x;
  }


  function uladd ($x, $y) {
    return ulongval (bcadd (ulongval ($x), ulongval ($y)));
  }

  function ulmul ($x, $y) {
    return ulongval (bcmul (ulongval ($x), ulongval ($y)));
  }

  function uldiv ($x, $y) {
    return ulongval (bcdiv (ulongval ($x), ulongval ($y), 0));
  }

  function ulmod ($x, $y) {
    return ulongval (bcmod (ulongval ($x), ulongval ($y)));
  }

  function ulsub ($x, $y) {
    return ulongval (bcsub (ulongval ($x), ulongval ($y)));
  }

  function ulpow ($x, $y) {
    $res = 1;
    while ($y--) {
      $res = ulmul ($res, $x);
    }
    return $res;
  }

  function ulshl ($x, $y) {
    return ulmul ($x, ulpow (2, $y));
  }

  function ulshr ($x, $y) {
    return uldiv ($x, ulpow (2, $y));
  }

  function ulnot ($x) {
    return ulongval (~intval (longval ($x)));
  }

  function ulxor ($x, $y) {
    return ulongval (intval (longval ($x)) ^ intval (longval ($y)));
  }

  function ulor ($x, $y) {
    return ulongval (intval (longval ($x)) | intval (longval ($y)));
  }

  function uland ($x, $y) {
    return ulongval (intval (longval ($x)) & intval (longval ($y)));
  }

  function ulcomp ($x, $y) {
    return bccomp (ulongval ($x), ulongval ($y));
  }

  function ulongval ($x) {
    if ($x[0] == '0' && $x[1] == 'x') {
      $x = base_convert (substr ($x, 2), 16, 10);
    }
    $x = bcmod ($x, "18446744073709551616");
    if (bccomp ($x, "0") == -1) {
      $x = bcadd ($x, "18446744073709551616");
    }
    return $x;
  }


  function uiadd ($x, $y) {
    return uintval (bcadd (uintval ($x), uintval ($y)));
  }

  function uimul ($x, $y) {
    return uintval (bcmul (uintval ($x), uintval ($y)));
  }

  function uidiv ($x, $y) {
    return uintval (bcdiv (uintval ($x), uintval ($y), 0));
  }

  function uimod ($x, $y) {
    return uintval (bcmod (uintval ($x), uintval ($y)));
  }

  function uisub ($x, $y) {
    return uintval (bcsub (uintval ($x), uintval ($y)));
  }

  function uipow ($x, $y) {
    $res = 1;
    while ($y--) {
      $res = uimul ($res, $x);
    }
    return $res;
  }

  function uishl ($x, $y) {
    return uimul ($x, uipow (2, $y));
  }

  function uishr ($x, $y) {
    return uidiv ($x, uipow (2, $y));
  }

  function uinot ($x) {
    return uintval (~intval (longval (uintval ($x))));
  }

  function uixor ($x, $y) {
    return uintval (intval (longval (uintval ($x))) ^ intval (longval (uintval ($y))));
  }

  function uior ($x, $y) {
    return uintval (intval (longval (uintval ($x))) | intval (longval (uintval ($y))));
  }

  function uiand ($x, $y) {
    return uintval (intval (longval (uintval ($x))) & intval (longval (uintval ($y))));
  }

  function uicomp ($x, $y) {
    return bccomp (uintval ($x), uintval ($y));
  }

  function uintval ($x) {
    if ($x[0] == '0' && $x[1] == 'x') {
      $x = base_convert (substr ($x, 2), 16, 10);
    }
    $x = bcmod ($x, "4294967296");
    if (bccomp ($x, "0") == -1) {
      $x = bcadd ($x, "4294967296");
    }
    return $x;
  }

  function base64url_encode($data) {
    return rtrim(strtr(base64_encode($data), '+/', '-_'), '=');
  }

  function base64url_decode($data) {
    return base64_decode(str_pad(strtr($data, '-_', '+/'), ceil(strlen($data) / 4) * 4, '=', STR_PAD_RIGHT));
  }

  function base64url_decode_ulong ($secret_encoded) {
    $secret_packed = base64url_decode($secret_encoded);
    $secret_unpacked = unpack('Vlow/Vhigh', $secret_packed);
    $secret = uladd(ulongval($secret_unpacked['low']), ulshl(ulongval($secret_unpacked['high']), 32));
    return $secret;
  }

  function base64url_encode_ulong ($secret_long) {
    $secret_long = ulongval(longval($secret_long));
    $secret_encoded = pack('VV', intval(uland($secret_long, 0xFFFFFFFF)), intval(ulshr($secret_long, 32)));
    return base64url_encode($secret_encoded);
  }

  function base64url_decode_ulong_NN ($secret_encoded) {
    $secret_packed = base64url_decode($secret_encoded);
    $secret_unpacked = unpack('Nlow/Nhigh', $secret_packed);
    $secret = uladd(ulongval($secret_unpacked['low']), ulshl(ulongval($secret_unpacked['high']), 32));
    return $secret;
  }

  function base64url_encode_ulong_NN ($secret_long) {
    $secret_long = ulongval(longval($secret_long));
    $secret_encoded = pack('NN', intval(uland($secret_long, 0xFFFFFFFF)), intval(ulshr($secret_long, 32)));
    return base64url_encode($secret_encoded);
  }

#endif

  $a = array (1, -123, longval (12345678901), -456789, -1, longval ("1234567890121"), longval (2131312321323), longval (-3843848384838483843), 2323223, 1, @longval ("-100000000000000000000000000000000123123123123"), longval ("0x12345abcde"), longval ("0xfffffffffffffff"));
  $b = $a;

  foreach ($a as $va) {
    var_dump (strval (lpow ($va, 11)));
    var_dump (strval (labs ($va)));
    var_dump (strval (lnot ($va)));
    foreach ($b as $vb) {
      var_dump (strval ("Long ".longval ($va))." vs ".longval ($vb));
      var_dump (strval (ladd ($va, $vb)));
      var_dump (strval (ldiv ($va, $vb)));
      var_dump (strval (lmod ($va, $vb)));
      var_dump (strval (lsub ($va, $vb)));
      var_dump (strval (lor ($va, $vb)));
      var_dump (strval (land ($va, $vb)));
      var_dump (strval (lxor ($va, $vb)));
      var_dump (strval (lcomp ($va, $vb)));
    }
  }

  var_dump (strval (lmul (123123123123, 456)));
  var_dump (strval (lshl (123, 32)));
  var_dump (strval (lshr (123123123123, 12)));
  var_dump (strval (lpow (-2, 5)));


  $ula = array (1, 123, ulongval (12345678901), -456789, 1, ulongval ("1234567890121"), ulongval (2131312321323), ulongval (3843848384838483843), 2323223, 1, @ulongval ("-100000000000000000000000000000000123123123123"), ulongval ("0x12345abcde"), ulongval ("0xfffffffffffffff"));
  $ulb = $ula;

  foreach ($ula as $ulva) {
    var_dump (base64url_encode_ulong ($ulva));
    var_dump (strval (base64url_decode_ulong (base64url_encode_ulong ($ulva))));
    var_dump (base64url_encode_ulong_NN ($ulva));
    var_dump (strval (base64url_decode_ulong_NN (base64url_encode_ulong_NN ($ulva))));
    var_dump (strval (ulpow ($ulva, 9)));
    var_dump (strval (ulnot ($ulva)));
    foreach ($ulb as $ulvb) {
      var_dump (strval ("ULong ".ulongval ($ulva))." vs ".ulongval ($ulvb));
      var_dump (strval (uladd ($ulva, $ulvb)));
      var_dump (strval (uldiv ($ulva, $ulvb)));
      var_dump (strval (ulmod ($ulva, $ulvb)));
      var_dump (uicomp ($uiva, $uivb));
      var_dump (strval (uisub ($uiva, $uivb)));
      var_dump (strval (ulor ($ulva, $ulvb)));
      var_dump (strval (uland ($ulva, $ulvb)));
      var_dump (strval (ulxor ($ulva, $ulvb)));
      var_dump (strval (ulcomp ($ulva, $ulvb)));
    }
  }

  var_dump (strval (ulmul (123123123123, 456)));
  var_dump (strval (ulshl (123, 42)));
  var_dump (strval (ulshr (123123123123, 12)));
  var_dump (strval (ulpow (2, 62)));


  $uia = array (1, -123, uintval (123456789), 456789, 1, uintval ("1137890121"), uintval (2331312321), uintval (-3843), 2323223, 1, -112312, @uintval ("100000000000000000000000000000000123123123123"), uintval ("0xabcde"), uintval ("0xfffffffe"), uintval ("0xa"));
  $uib = $uia;

  foreach ($uia as $uiva) {
    var_dump (strval (uipow ($uiva, 12)));
    var_dump (strval (uinot ($uiva)));
    foreach ($uib as $uivb) {
      var_dump (strval ("UInt ".uintval ($uiva))." vs ".uintval ($uivb));
      var_dump (strval (uiadd ($uiva, $uivb)));
      var_dump (strval (uidiv ($uiva, $uivb)));
      var_dump (strval (uimod ($uiva, $uivb)));
      var_dump (uicomp ($uiva, $uivb));
      var_dump (strval (uisub ($uiva, $uivb)));
      var_dump (strval (uior ($uiva, $uivb)));
      var_dump (strval (uiand ($uiva, $uivb)));
      var_dump (strval (uixor ($uiva, $uivb)));
      var_dump (strval (uicomp ($uiva, $uivb)));
    }
  }

  var_dump (strval (uimul (1000456, 4001)));
  var_dump (strval (uishr (4123123123, 12)));
  var_dump (strval (uishl (100123, 12)));
  var_dump (strval (uipow (2, 31)));

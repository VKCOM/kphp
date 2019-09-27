@kphp_vs_php_diff
<?php

function test_pack_unpack() {
  var_dump(unpack("a5q/a4w/Ae/h2r/H5t/H2y/H3u/h", pack ("A5a4Ah2H5H2H3h", "h h", 'q q', 't t', 345, "ABCDE", "ABCDE", "ABCDEF", "A1")));
  var_dump(unpack("I2q/N4w", pack ("I2N4", -123456, -234567, 345678, 456789, 567890, 678901)));
  var_dump(unpack("iqwe/Iq22w2w22w2w2sd/lz123123/Lx11/Nqwe/V", pack ("iIlLNV", -1, -2, -3, 4, 5, 6)));
  var_dump(unpack("iqwe/Iq22w2w22w2w2sd/lz123123/Lx11/Nqwe/V", pack ("iIlLNV", -123456, -234567, -345678, 456789, 567890, 678901)));
}

function test_file_put_contents_into_stdout() {
  file_put_contents(STDOUT, array ("Part", " of the answer\n"));
}

function test_strip_tags_br() {
  var_dump(strip_tags("<br/>", "<br>"));
}

function test_bin_to_hex() {
  $str = "A 'цитата' ЕСТЬ \"<b>bold</b>!@#$%^&*(){}[]=-:\";'/.,<>?`~\n";

  for ($i = 0; $i <= 255; $i++) {
    if ($i != 0x98) {
      $s .= chr ($i);
    }
  }

#ifndef KittenPHP
  var_dump (bin2hex (htmlentities($str, ENT_COMPAT | ENT_HTML401, 'cp1251')));
  var_dump (bin2hex (htmlspecialchars($str, ENT_COMPAT | ENT_HTML401, 'cp1251')));
  var_dump (bin2hex (htmlentities($s, ENT_COMPAT | ENT_HTML401, 'cp1251')));

  if (0) {
#endif
  var_dump (bin2hex (htmlentities($str)));
  var_dump (bin2hex (htmlspecialchars($str)));
  var_dump (bin2hex (htmlentities($s)));
#ifndef KittenPHP
  }
#endif
}

function test_var_dump_escape_sequences() {
  $x = ["\a", "\cx", "\e", "\f", "\n", "\r", "\t", "\xhh", "\ddd", "\v"];
  var_dump($x);
}

function test_array_in_string() {
  $juices = array ("apple", "orange");
  echo "$juices[0]s\n";

  $b["foo"] = 1;
  echo "a $b[foo] c\n";

  $i = 5;
  $c[$i] = 1;
  echo "a $c[$i] c\n";
}

function test_unset_in_foreach_with_ref() {
  $a = array ("a" => "A", "b" => "B", 0 => 7, "a" => "Z");
  foreach ($a as $key => &$val) {
    var_dump ($key);
    var_dump ($val);
    unset ($a[$key]);
    var_dump ($a);
  }
}

function test_string_incr() {
  // and post-op
  $str = "asdasd";
  var_dump ($str++);
  var_dump ($str++);
  var_dump ($str++);
  var_dump ($str++);

  $str = "0as0dasd0";
  var_dump ($str++);
  var_dump ($str++);
  var_dump ($str++);
  var_dump ($str++);

  // and pre-op
  $str = "asdasd";
  var_dump (++$str);
  var_dump (++$str);
  var_dump (++$str);
  var_dump (++$str);

  $str = "0as0dasd0";
  var_dump (++$str);
  var_dump (++$str);
  var_dump (++$str);
  var_dump (++$str);
}

function test_list_assignment() {
  // note, weak references
  list ($a, $b) = array ("x" => "x", "y" => "y");
  var_dump ($a);
  var_dump ($b);

  // the php site makes an interesting point, and sullies it with an awful example
  $a = array();
  $info = array('coffee', 'brown', 'caffeine');
  list($a[0], $a[1], $a[2]) = $info;
  var_dump ($a);
  echo "-------------9----------------\n";

  // how about:
  list($info[2], $info[1], $info[0]) = $info;
  // it looks like it reverses the order, but it doesnt. It first overwrites
  // $info[0], then later overwrites $info[2] with the value already copied
  // from $info[2]
  var_dump ($info);
}

function test_integer_overflow() {
  echo "0x80000000\t= "; var_dump(0x80000000);
  echo "0x80000001\t= "; var_dump(0x80000001);
  echo "0x80000002\t= "; var_dump(0x80000002);

  echo "2147483648\t= "; var_dump(2147483648);
  echo "2147483649\t= "; var_dump(2147483649);
  echo "2147483650\t= "; var_dump(2147483650);

  echo "0xfffffffd\t= "; var_dump(0xfffffffd);
  echo "0xfffffffe\t= "; var_dump(0xfffffffe);
  echo "0xffffffff\t= "; var_dump(0xffffffff);
  echo "0x100000000\t= "; var_dump(0x100000000);
  echo "0x100000001\t= "; var_dump(0x100000001);
  echo "0x100000002\t= "; var_dump(0x100000002);

  echo "4294967293\t= "; var_dump(4294967293);
  echo "4294967294\t= "; var_dump(4294967294);
  echo "4294967295\t= "; var_dump(4294967295);
  echo "4294967296\t= "; var_dump(4294967296);
  echo "4294967297\t= "; var_dump(4294967297);
  echo "4294967298\t= "; var_dump(4294967298);

  echo "-0x80000001\t= "; var_dump(-0x80000001);
  echo "-0x80000002\t= "; var_dump(-0x80000002);

  echo "-2147483649\t= "; var_dump(-2147483649);
  echo "-2147483650\t= "; var_dump(-2147483650);

  echo "-0xfffffffd\t= "; var_dump(-0xfffffffd);
  echo "-0xfffffffe\t= "; var_dump(-0xfffffffe);
  echo "-0xffffffff\t= "; var_dump(-0xffffffff);
  echo "-0x100000000\t= "; var_dump(-0x100000000);
  echo "-0x100000001\t= "; var_dump(-0x100000001);
  echo "-0x100000002\t= "; var_dump(-0x100000002);

  echo "-4294967293\t= "; var_dump(-4294967293);
  echo "-4294967294\t= "; var_dump(-4294967294);
  echo "-4294967295\t= "; var_dump(-4294967295);
  echo "-4294967296\t= "; var_dump(-4294967296);
  echo "-4294967297\t= "; var_dump(-4294967297);
  echo "-4294967298\t= "; var_dump(-4294967298);
}

function test_negative_literals_overflow() {
  var_dump(- -0x80000000);
  var_dump(- -0x80000001);
  var_dump(- -0x80000002);

  var_dump(- -2147483648);
  var_dump(- -2147483649);
  var_dump(- -2147483650);

  var_dump(- -0xfffffffd);
  var_dump(- -0xfffffffe);
  var_dump(- -0xffffffff);
  var_dump(- -0x100000000);
  var_dump(- -0x100000001);
  var_dump(- -0x100000002);

  var_dump(- -4294967293);
  var_dump(- -4294967294);
  var_dump(- -4294967295);
  var_dump(- -4294967296);
  var_dump(- -4294967297);
  var_dump(- -4294967298);

  var_dump(- - -0x80000001);
  var_dump(- - -0x80000002);

  var_dump(- - -2147483649);
  var_dump(- - -2147483650);

  var_dump(- - -0xfffffffd);
  var_dump(- - -0xfffffffe);
  var_dump(- - -0xffffffff);
  var_dump(- - -0x100000000);
  var_dump(- - -0x100000001);
  var_dump(- - -0x100000002);

  var_dump(- - -4294967293);
  var_dump(- - -4294967294);
  var_dump(- - -4294967295);
  var_dump(- - -4294967296);
  var_dump(- - -4294967297);
  var_dump(- - -4294967298);

  var_dump(- -020000000000);
  var_dump(- -020000000007);
  var_dump(- - -020000000007);
}

function test_naming_change() {
  function foobar(){
    echo "foobar()\n";
  }

  class Foo {
    function foobar()	{
      echo "Foo::foobar()\n";
    }

    function Foo(){
      $this->foobar();
      foobar();
    }
  }

  $x = new Foo();
  foobar();
}

function test_nested_statement() {
  var_dump(($x = 5) + ($x = 7) * ($x = 12));
}

test_pack_unpack();
test_file_put_contents_into_stdout();
test_strip_tags_br();
test_bin_to_hex();
test_var_dump_escape_sequences();
test_array_in_string();
test_unset_in_foreach_with_ref();
test_string_incr();
test_list_assignment();
test_integer_overflow();
test_negative_literals_overflow();
test_naming_change();
test_nested_statement();

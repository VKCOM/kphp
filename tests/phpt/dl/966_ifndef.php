@ok
<?

echo "A";

#ifndef KPHP

class X { }

$x = new X;
// kphp does not support such thing
$x->unknown_field = "123";
if ($x->unknown_field != 123) {
  echo "Wow";
}

#endif

echo "D";

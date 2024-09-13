@ok k2_skip
<?php
  $is_kphp = true;
#ifndef KPHP
  $is_kphp = false;
#endif

  $a = 1 ? null : 2;
  $b = true;
  $c = 13;
  $d = 17.0;
  $e = "7 abacaba";
  $f = array (45);
  $g = array ("adasd");
  $h = array (45);//TODO object
  $i = array ("adasd");//TODO object
  $j = "5";
  if (0) {
    $j = 7;
  }

  @var_dump ($a / $a);
  if($is_kphp) @var_dump ($a / $b); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $c); else var_dump (0.0);
  @var_dump ($a / $d);
  if($is_kphp) @var_dump ($a / $e); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $i); else var_dump (0.0);
  if($is_kphp) @var_dump ($a / $j); else var_dump (0.0);

  echo "\n";

  @var_dump ($b / $a);
  if($is_kphp) @var_dump ($b / $b); else var_dump ((double) $b / $b);
  @var_dump ($b / $c);
  @var_dump ($b / $d);
  @var_dump ($b / $e);
  if($is_kphp) @var_dump ($b / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($b / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($b / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($b / $i); else var_dump (0.0);
  @var_dump ($b / $j);

  echo "\n";

  @var_dump ($c / $a);
  if($is_kphp) @var_dump ($c / $b); else var_dump ((double) $c / $b);
  if($is_kphp) @var_dump ($c / $c); else var_dump ((double) $c / $c);
  @var_dump ($c / $d);
  @var_dump ($c / $e);
  if($is_kphp) @var_dump ($c / $f); else var_dump ($c / 1.0);
  if($is_kphp) @var_dump ($c / $g); else var_dump ($c / 1.0);
  if($is_kphp) @var_dump ($c / $h); else var_dump ($c / 1.0);
  if($is_kphp) @var_dump ($c / $i); else var_dump ($c / 1.0);
  @var_dump ($c / $j);

  echo "\n";

  @var_dump ($d / $a);
  @var_dump ($d / $b);
  @var_dump ($d / $c);
  @var_dump ($d / $d);
  @var_dump ($d / $e);
  if($is_kphp) @var_dump ($d / $f); else var_dump ($d / 1.0);
  if($is_kphp) @var_dump ($d / $g); else var_dump ($d / 1.0);
  if($is_kphp) @var_dump ($d / $h); else var_dump ($d / 1.0);
  if($is_kphp) @var_dump ($d / $i); else var_dump ($d / 1.0);
  @var_dump ($d / $j);

  echo "\n";

  @var_dump ($e / $a);
  if($is_kphp) @var_dump ($e / $b); else @var_dump ((double) $e / $b);
  @var_dump ($e / $c);
  @var_dump ($e / $d);
  if($is_kphp) @var_dump ($e / $e); else @var_dump ((double) $e / $e);
  if($is_kphp) @var_dump ($e / $f); else @var_dump ($e / 1.0);
  if($is_kphp) @var_dump ($e / $g); else @var_dump ($e / 1.0);
  if($is_kphp) @var_dump ($e / $h); else @var_dump ($e / 1.0);
  if($is_kphp) @var_dump ($e / $i); else @var_dump ($e / 1.0);
  @var_dump ($e / $j);

  echo "\n";

  if($is_kphp) @var_dump ($f / $a); else var_dump (1 / 0.0);
  if($is_kphp) @var_dump ($f / $b); else var_dump (0.0);
  if($is_kphp) @var_dump ($f / $c); else var_dump (1.0 / $c);
  if($is_kphp) @var_dump ($f / $d); else var_dump (1.0 / $d);
  if($is_kphp) @var_dump ($f / $e); else @var_dump (1.0 / $e);
  if($is_kphp) @var_dump ($f / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($f / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($f / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($f / $i); else var_dump (0.0);
  if($is_kphp) @var_dump ($f / $j); else var_dump (1.0 / $j);

  echo "\n";

  if($is_kphp) @var_dump ($g / $a); else var_dump (1 / 0.0);
  if($is_kphp) @var_dump ($g / $b); else var_dump (0.0);
  if($is_kphp) @var_dump ($g / $c); else var_dump (1.0 / $c);
  if($is_kphp) @var_dump ($g / $d); else var_dump (1.0 / $d);
  if($is_kphp) @var_dump ($g / $e); else @var_dump (1.0 / $e);
  if($is_kphp) @var_dump ($g / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($g / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($g / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($g / $i); else var_dump (0.0);
  if($is_kphp) @var_dump ($g / $j); else var_dump (1.0 / $j);

  echo "\n";

  if($is_kphp) @var_dump ($h / $a); else var_dump (1 / 0.0);
  if($is_kphp) @var_dump ($h / $b); else var_dump (0.0);
  if($is_kphp) @var_dump ($h / $c); else var_dump (1.0 / $c);
  if($is_kphp) @var_dump ($h / $d); else var_dump (1.0 / $d);
  if($is_kphp) @var_dump ($h / $e); else @var_dump (1.0 / $e);
  if($is_kphp) @var_dump ($h / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($h / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($h / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($h / $i); else var_dump (0.0);
  if($is_kphp) @var_dump ($h / $j); else var_dump (1.0 / $j);

  echo "\n";

  if($is_kphp) @var_dump ($i / $a); else var_dump (1 / 0.0);
  if($is_kphp) @var_dump ($i / $b); else var_dump (0.0);
  if($is_kphp) @var_dump ($i / $c); else var_dump (1.0 / $c);
  if($is_kphp) @var_dump ($i / $d); else var_dump (1.0 / $d);
  if($is_kphp) @var_dump ($i / $e); else @var_dump (1.0 / $e);
  if($is_kphp) @var_dump ($i / $f); else var_dump (0.0);
  if($is_kphp) @var_dump ($i / $g); else var_dump (0.0);
  if($is_kphp) @var_dump ($i / $h); else var_dump (0.0);
  if($is_kphp) @var_dump ($i / $i); else var_dump (0.0);
  if($is_kphp) @var_dump ($i / $j); else var_dump (1.0 / $j);

  echo "\n";

  @var_dump ($j / $a);
  if($is_kphp) @var_dump ($j / $b); else var_dump ((double) $j / $b);
  @var_dump ($j / $c);
  @var_dump ($j / $d);
  @var_dump ($j / $e);
  if($is_kphp) @var_dump ($j / $f); else var_dump ($j / 1.0);
  if($is_kphp) @var_dump ($j / $g); else var_dump ($j / 1.0);
  if($is_kphp) @var_dump ($j / $h); else var_dump ($j / 1.0);
  if($is_kphp) @var_dump ($j / $i); else var_dump ($j / 1.0);
  if($is_kphp) @var_dump ($j / $j); else var_dump ((double) $j / $j);

  echo "\n";

  if($is_kphp) @var_dump ($a % $a); else var_dump (0);
  @var_dump ($a % $b);
  @var_dump ($a % $c);
  @var_dump ($a % $d);
  @var_dump ($a % $e);
  @var_dump ($a % $f);
  @var_dump ($a % $g);
  @var_dump ($a % $h);
  @var_dump ($a % $i);
  @var_dump ($a % $j);

  echo "\n";

  if($is_kphp) @var_dump ($b % $a); else var_dump (0);
  @var_dump ($b % $b);
  @var_dump ($b % $c);
  @var_dump ($b % $d);
  @var_dump ($b % $e);
  @var_dump ($b % $f);
  @var_dump ($b % $g);
  @var_dump ($b % $h);
  @var_dump ($b % $i);
  @var_dump ($b % $j);

  echo "\n";

  if($is_kphp) @var_dump ($c % $a); else var_dump (0);
  @var_dump ($c % $b);
  @var_dump ($c % $c);
  @var_dump ($c % $d);
  @var_dump ($c % $e);
  @var_dump ($c % $f);
  @var_dump ($c % $g);
  @var_dump ($c % $h);
  @var_dump ($c % $i);
  @var_dump ($c % $j);

  echo "\n";

  if($is_kphp) @var_dump ($d % $a); else var_dump (0);
  @var_dump ($d % $b);
  @var_dump ($d % $c);
  @var_dump ($d % $d);
  @var_dump ($d % $e);
  @var_dump ($d % $f);
  @var_dump ($d % $g);
  @var_dump ($d % $h);
  @var_dump ($d % $i);
  @var_dump ($d % $j);

  echo "\n";

  if($is_kphp) @var_dump ($e % $a); else var_dump (0);
  @var_dump ($e % $b);
  @var_dump ($e % $c);
  @var_dump ($e % $d);
  @var_dump ($e % $e);
  @var_dump ($e % $f);
  @var_dump ($e % $g);
  @var_dump ($e % $h);
  @var_dump ($e % $i);
  @var_dump ($e % $j);

  echo "\n";

  if($is_kphp) @var_dump ($f % $a); else var_dump (0);
  @var_dump ($f % $b);
  @var_dump ($f % $c);
  @var_dump ($f % $d);
  @var_dump ($f % $e);
  @var_dump ($f % $f);
  @var_dump ($f % $g);
  @var_dump ($f % $h);
  @var_dump ($f % $i);
  @var_dump ($f % $j);

  echo "\n";

  if($is_kphp) @var_dump ($g % $a); else var_dump (0);
  @var_dump ($g % $b);
  @var_dump ($g % $c);
  @var_dump ($g % $d);
  @var_dump ($g % $e);
  @var_dump ($g % $f);
  @var_dump ($g % $g);
  @var_dump ($g % $h);
  @var_dump ($g % $i);
  @var_dump ($g % $j);

  echo "\n";

  if($is_kphp) @var_dump ($h % $a); else var_dump (0);
  @var_dump ($h % $b);
  @var_dump ($h % $c);
  @var_dump ($h % $d);
  @var_dump ($h % $e);
  @var_dump ($h % $f);
  @var_dump ($h % $g);
  @var_dump ($h % $h);
  @var_dump ($h % $i);
  @var_dump ($h % $j);

  echo "\n";

  if($is_kphp) @var_dump ($i % $a); else var_dump (0);
  @var_dump ($i % $b);
  @var_dump ($i % $c);
  @var_dump ($i % $d);
  @var_dump ($i % $e);
  @var_dump ($i % $f);
  @var_dump ($i % $g);
  @var_dump ($i % $h);
  @var_dump ($i % $i);
  @var_dump ($i % $j);

  echo "\n";

  if($is_kphp) @var_dump ($j % $a); else var_dump (0);
  @var_dump ($j % $b);
  @var_dump ($j % $c);
  @var_dump ($j % $d);
  @var_dump ($j % $e);
  @var_dump ($j % $f);
  @var_dump ($j % $g);
  @var_dump ($j % $h);
  @var_dump ($j % $i);
  @var_dump ($j % $j);

  @var_dump( 0.0 / 0.0);
  @var_dump( 1.0 / 0.0);
  @var_dump(-1.0 / 0.0);

  @var_dump( 0 / 0);
  @var_dump( 1 / 0);
  @var_dump(-1 / 0);

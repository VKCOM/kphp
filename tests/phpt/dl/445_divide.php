@ok no_php
<?php
  $a = null;
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
  @var_dump ($a / $b);
  @var_dump ($a / $c);
  @var_dump ($a / $d);
  @var_dump ($a / $e);
  @var_dump ($a / $f);
  @var_dump ($a / $g);
  @var_dump ($a / $h);
  @var_dump ($a / $i);
  @var_dump ($a / $j);

  @var_dump ($b / $a);
  @var_dump ($b / $b);
  @var_dump ($b / $c);
  @var_dump ($b / $d);
  @var_dump ($b / $e);
  @var_dump ($b / $f);
  @var_dump ($b / $g);
  @var_dump ($b / $h);
  @var_dump ($b / $i);
  @var_dump ($b / $j);

  @var_dump ($c / $a);
  @var_dump ($c / $b);
  @var_dump ($c / $c);
  @var_dump ($c / $d);
  @var_dump ($c / $e);
  @var_dump ($c / $f);
  @var_dump ($c / $g);
  @var_dump ($c / $h);
  @var_dump ($c / $i);
  @var_dump ($c / $j);

  @var_dump ($d / $a);
  @var_dump ($d / $b);
  @var_dump ($d / $c);
  @var_dump ($d / $d);
  @var_dump ($d / $e);
  @var_dump ($d / $f);
  @var_dump ($d / $g);
  @var_dump ($d / $h);
  @var_dump ($d / $i);
  @var_dump ($d / $j);

  @var_dump ($e / $a);
  @var_dump ($e / $b);
  @var_dump ($e / $c);
  @var_dump ($e / $d);
  @var_dump ($e / $e);
  @var_dump ($e / $f);
  @var_dump ($e / $g);
  @var_dump ($e / $h);
  @var_dump ($e / $i);
  @var_dump ($e / $j);

  @var_dump ($f / $a);
  @var_dump ($f / $b);
  @var_dump ($f / $c);
  @var_dump ($f / $d);
  @var_dump ($f / $e);
  @var_dump ($f / $f);
  @var_dump ($f / $g);
  @var_dump ($f / $h);
  @var_dump ($f / $i);
  @var_dump ($f / $j);

  @var_dump ($g / $a);
  @var_dump ($g / $b);
  @var_dump ($g / $c);
  @var_dump ($g / $d);
  @var_dump ($g / $e);
  @var_dump ($g / $f);
  @var_dump ($g / $g);
  @var_dump ($g / $h);
  @var_dump ($g / $i);
  @var_dump ($g / $j);

  @var_dump ($h / $a);
  @var_dump ($h / $b);
  @var_dump ($h / $c);
  @var_dump ($h / $d);
  @var_dump ($h / $e);
  @var_dump ($h / $f);
  @var_dump ($h / $g);
  @var_dump ($h / $h);
  @var_dump ($h / $i);
  @var_dump ($h / $j);

  @var_dump ($i / $a);
  @var_dump ($i / $b);
  @var_dump ($i / $c);
  @var_dump ($i / $d);
  @var_dump ($i / $e);
  @var_dump ($i / $f);
  @var_dump ($i / $g);
  @var_dump ($i / $h);
  @var_dump ($i / $i);
  @var_dump ($i / $j);

  @var_dump ($j / $a);
  @var_dump ($j / $b);
  @var_dump ($j / $c);
  @var_dump ($j / $d);
  @var_dump ($j / $e);
  @var_dump ($j / $f);
  @var_dump ($j / $g);
  @var_dump ($j / $h);
  @var_dump ($j / $i);
  @var_dump ($j / $j);

  @var_dump ($a % $a);
  @var_dump ($a % $b);
  @var_dump ($a % $c);
  @var_dump ($a % $d);
  @var_dump ($a % $e);
  @var_dump ($a % $f);
  @var_dump ($a % $g);
  @var_dump ($a % $h);
  @var_dump ($a % $i);
  @var_dump ($a % $j);

  @var_dump ($b % $a);
  @var_dump ($b % $b);
  @var_dump ($b % $c);
  @var_dump ($b % $d);
  @var_dump ($b % $e);
  @var_dump ($b % $f);
  @var_dump ($b % $g);
  @var_dump ($b % $h);
  @var_dump ($b % $i);
  @var_dump ($b % $j);

  @var_dump ($c % $a);
  @var_dump ($c % $b);
  @var_dump ($c % $c);
  @var_dump ($c % $d);
  @var_dump ($c % $e);
  @var_dump ($c % $f);
  @var_dump ($c % $g);
  @var_dump ($c % $h);
  @var_dump ($c % $i);
  @var_dump ($c % $j);

  @var_dump ($d % $a);
  @var_dump ($d % $b);
  @var_dump ($d % $c);
  @var_dump ($d % $d);
  @var_dump ($d % $e);
  @var_dump ($d % $f);
  @var_dump ($d % $g);
  @var_dump ($d % $h);
  @var_dump ($d % $i);
  @var_dump ($d % $j);

  @var_dump ($e % $a);
  @var_dump ($e % $b);
  @var_dump ($e % $c);
  @var_dump ($e % $d);
  @var_dump ($e % $e);
  @var_dump ($e % $f);
  @var_dump ($e % $g);
  @var_dump ($e % $h);
  @var_dump ($e % $i);
  @var_dump ($e % $j);

  @var_dump ($f % $a);
  @var_dump ($f % $b);
  @var_dump ($f % $c);
  @var_dump ($f % $d);
  @var_dump ($f % $e);
  @var_dump ($f % $f);
  @var_dump ($f % $g);
  @var_dump ($f % $h);
  @var_dump ($f % $i);
  @var_dump ($f % $j);

  @var_dump ($g % $a);
  @var_dump ($g % $b);
  @var_dump ($g % $c);
  @var_dump ($g % $d);
  @var_dump ($g % $e);
  @var_dump ($g % $f);
  @var_dump ($g % $g);
  @var_dump ($g % $h);
  @var_dump ($g % $i);
  @var_dump ($g % $j);

  @var_dump ($h % $a);
  @var_dump ($h % $b);
  @var_dump ($h % $c);
  @var_dump ($h % $d);
  @var_dump ($h % $e);
  @var_dump ($h % $f);
  @var_dump ($h % $g);
  @var_dump ($h % $h);
  @var_dump ($h % $i);
  @var_dump ($h % $j);

  @var_dump ($i % $a);
  @var_dump ($i % $b);
  @var_dump ($i % $c);
  @var_dump ($i % $d);
  @var_dump ($i % $e);
  @var_dump ($i % $f);
  @var_dump ($i % $g);
  @var_dump ($i % $h);
  @var_dump ($i % $i);
  @var_dump ($i % $j);

  @var_dump ($j % $a);
  @var_dump ($j % $b);
  @var_dump ($j % $c);
  @var_dump ($j % $d);
  @var_dump ($j % $e);
  @var_dump ($j % $f);
  @var_dump ($j % $g);
  @var_dump ($j % $h);
  @var_dump ($j % $i);
  @var_dump ($j % $j);

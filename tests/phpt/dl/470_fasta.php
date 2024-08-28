@ok benchmark k2_skip
<?php
/* The Computer Language Benchmarks Game
   http://shootout.alioth.debian.org/

   contributed by Wing-Chung Leung
   modified by Isaac Gouy
   modified by anon
 */

$last = 42;
function gen_random(&$last, &$randoms, $max = 1, $ia = 3877, $ic = 29573, $im = 139968) {
   foreach($randoms as &$r) {
      $r = $max * ($last = ($last * $ia + $ic) % $im) / $im;
   }
}

/* Weighted selection from alphabet */

function makeCumulative(&$genelist) {
   $cumul = 0.0;
   foreach($genelist as $k=>&$v) {
      $cumul = $v += $cumul;
   }
}


/* Generate and write FASTA format */

function makeRandomFasta(&$genelist, $n) {
   $width = 60;
   $lines = (int) ($n / $width);
   $pick = str_repeat('?', $width)."\n";
   $randoms = array_fill(0, $width, 0.0);
   global $last;

   // full lines
   for ($i = 0; $i < $lines; ++$i) {
      gen_random($last, $randoms);
      $j = 0;
      foreach ($randoms as $r) {
         foreach($genelist as $k=>$v) {
            if ($r < $v) {
               break;
            }
         }
         $pick[$j++] = $k;
      }
      echo $pick;
   }

   // last, partial line
   $w = $n % $width;
   if ($w !== 0) {
      $randoms = array_fill(0, $w, 0.0);
      gen_random($last, $randoms);
      $j = 0;
      foreach ($randoms as $r) {
         foreach($genelist as $k=>$v) {
            if ($r < $v) {
               break;
            }
         }
         $pick[$j++] = $k;
      }
      $pick[$w] = "\n";
      echo substr($pick, 0, $w+1);
   }

}


function makeRepeatFasta($s, $n) {
   $i = 0; $sLength = strlen($s); $lineLength = 60;
   while ($n > 0) {
      if ($n < $lineLength) $lineLength = $n;
      if ($i + $lineLength < $sLength){
         print(substr($s,$i,$lineLength)); print("\n");
         $i += $lineLength;
      } else {
         print(substr($s,$i));
         $i = $lineLength - ($sLength - $i);
         print(substr($s,0,$i)); print("\n");
      }
      $n -= $lineLength;
   }
}


/* Main -- define alphabets, make 3 fragments */

$iub=array(
   'a' => 0.27,
   'c' => 0.12,
   'g' => 0.12,
   't' => 0.27,

   'B' => 0.02,
   'D' => 0.02,
   'H' => 0.02,
   'K' => 0.02,
   'M' => 0.02,
   'N' => 0.02,
   'R' => 0.02,
   'S' => 0.02,
   'V' => 0.02,
   'W' => 0.02,
   'Y' => 0.02
);

$homosapiens = array(
   'a' => 0.3029549426680,
   'c' => 0.1979883004921,
   'g' => 0.1975473066391,
   't' => 0.3015094502008
);

$alu =
   'GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG' .
   'GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA' .
   'CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT' .
   'ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA' .
   'GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG' .
   'AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC' .
   'AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA';

$n = 100000;

if ($_SERVER['argc'] > 1) $n = $_SERVER['argv'][1];

makeCumulative($iub);
makeCumulative($homosapiens);

echo ">ONE Homo sapiens alu\n";
makeRepeatFasta($alu, $n*2);

echo ">TWO IUB ambiguity codes\n";
makeRandomFasta($iub, $n*3);

echo ">THREE Homo sapiens frequency\n";
makeRandomFasta($homosapiens, $n*5);

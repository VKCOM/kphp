@ok k2_skip
<?php

fputcsv(STDOUT, array(0, 1, 2, 3, "a", '"b"', "x\tz"));//, ":q", "y");
fputcsv(STDOUT, array(0, 1, 2, 3, "a", '"b"', "x\tz"), ":", "y");
fputcsv(STDOUT, array(0, 1, 2, 3, "a", '"b"', "x\tz"), "q", "y");
fputcsv(STDOUT, array(0, 1, 2, 3, "a", '"b"', "x\tz"), ":", "0");
fputcsv(STDOUT, array(0, 1, 2, 3, "a", '"b"', "xx\tz"), ":", "0", "x");
fputcsv(STDOUT, array(0, 1, 2, 3, "a\nd", '"b"', "xx\tz"), ":", "0", "x");
fputcsv(STDOUT, array(0, 1, 2, 3, "asd asd qwe asd zxc asd    qw e as d zxc  asd", '"b"', "xx\tz"), ":", "0");
fputcsv(STDOUT, array(0, 1, 2, 3, "\\\"", '"b"', "xx\tz"), ":", "0");
fputcsv(STDOUT, array(" привет", " как дела? ", "мы тут"));

$fout = fopen("tmpfile", "w");
fputcsv($fout, array(0, 1, 2, 3, "a", '"b"', "x\tz"));//, ":q", "y");
fputcsv($fout, array(0, 1, 2, 3, "a", '"b"', "x\tz"), ":", "y");
fputcsv($fout, array(0, 1, 2, 3, "a", '"b"', "x\tz"), "q", "y");
fputcsv($fout, array(0, 1, 2, 3, "a", '"b"', "x\tz"), ":", "0");
fputcsv($fout, array(0, 1, 2, 3, "a", '"b"', "xx\tz"), ":", "0", "x");
fputcsv($fout, array(0, 1, 2, 3, "a\nd", '"b"', "xx\tz"), ":", "0", "x");
fputcsv($fout, array(0, 1, 2, 3, "asd asd qwe asd zxc asd    qw e as d zxc  asd", '"b"', "xx\tz"), ":", "0");
fputcsv($fout, array(0, 1, 2, 3, "\\\"", '"b"', "xx\tz"), ":", "0");
fputcsv($fout, array(" привет", " как дела? ", "мы тут"));
fclose($fout);

$fin = fopen("tmpfile", "r");
while (true) {
  $s = fgetcsv($fin);
  if (!$s) break;
  var_dump($s);
}
fclose($fin);


@ok benchmark_but_too_long k2_skip
<?php
#
# The Computer Language Shootout
# http://shootout.alioth.debian.org/
#
# contributed by Danny Sauer
# modified by Josh Goldfoot

# regexp matches

#ini_set("memory_limit","40M");

$variants = array(
    'agggtaaa|tttaccct',
    '[cgt]gggtaaa|tttaccc[acg]',
    'a[act]ggtaaa|tttacc[agt]t',
    'ag[act]gtaaa|tttac[agt]ct',
    'agg[act]taaa|ttta[agt]cct',
    'aggg[acg]aaa|ttt[cgt]ccct',
    'agggt[cgt]aa|tt[acg]accct',
    'agggta[cgt]a|t[acg]taccct',
    'agggtaa[cgt]|[acg]ttaccct',
);

# IUB replacement parallel arrays
$IUB = array(); $IUBnew = array();
$IUB[]='/B/';     $IUBnew[]='(c|g|t)';
$IUB[]='/D/';     $IUBnew[]='(a|g|t)';
$IUB[]='/H/';     $IUBnew[]='(a|c|t)';
$IUB[]='/K/';     $IUBnew[]='(g|t)';
$IUB[]='/M/';     $IUBnew[]='(a|c)';
$IUB[]='/N/';     $IUBnew[]='(a|c|g|t)';
$IUB[]='/R/';     $IUBnew[]='(a|g)';
$IUB[]='/S/';     $IUBnew[]='(c|g)';
$IUB[]='/V/';     $IUBnew[]='(a|c|g)';
$IUB[]='/W/';     $IUBnew[]='(a|t)';
$IUB[]='/Y/';     $IUBnew[]='(c|t)';

# sequence descriptions start with > and comments start with ;
#my $stuffToRemove = '^[>;].*$|[\r\n]';
$stuffToRemove = '^>.*$|\n'; # no comments, *nix-format test file...

# read in file
$contents = file_get_contents('/unexisting/tests/phpt/dl/regexdna-input.txt');
$initialLength = strlen($contents);

# remove things
$contents = preg_replace("/$stuffToRemove/m", '', $contents);
$codeLength = strlen($contents);

# do regexp counts
foreach ($variants as $regex){
    print $regex . ' ' . preg_match_all("/$regex/i", $contents, $discard). "\n";
}

# do replacements
$contents = preg_replace($IUB, $IUBnew, $contents);

print "\n" .
      $initialLength . "\n" .
      $codeLength . "\n" .
      strlen($contents) . "\n" ;
?>

@ok k2_skip
<?php

error_reporting(0);
/**
 * @param string $s1
 * @param string $s2
 * @param int $o
 * @param int $l
 * @param bool $c
 */
function substr_compare_helper($s1, $s2, $o = 0, $l = 1000, $c = false) {
    $x = substr_compare($s1, $s2, $o, $l, $c);

    if ($x === false) echo "false";
    elseif ($x < 0)   echo -1;
    elseif ($x > 0)   echo 1;
    elseif ($x == 0)  echo 0;

    echo "\n";
}

function substr_compare_test_less() {
    substr_compare_helper("abc", "", 3); 
    substr_compare_helper("", "b", 0); 
    substr_compare_helper("a", "b"); 
    substr_compare_helper("aabcde", "bcde", 1); 
}

function substr_compare_test_eq() {
    substr_compare_helper("abc", "", 3); 
    substr_compare_helper("", "", 0);
    substr_compare_helper("a", "a");
    substr_compare_helper("abcde", "abcde"); 

    substr_compare_helper("aaaa", "aa", 2); 
    substr_compare_helper("aaaa", "aa", -2); 
    substr_compare_helper("aaaa", "aa", -10); 
    substr_compare_helper("aaaa", "aa", -1, 1); 

    substr_compare_helper("AAA", "aaa", 0, 3, true); 
    substr_compare_helper("aABbCcDOp", "AabBcCdoP", 0, 5, true); 

    substr_compare_helper("zzzzz aABbCcDOp", "AabBcCdoP", 6, 9, true); 
}

function substr_compare_test_greater() {
    substr_compare_helper("b", ""); 
    substr_compare_helper("b", "a"); 
    substr_compare_helper("aabcde", "bcde", 3); 
    substr_compare_helper("bcde", "bcde", -1); 
}

function substr_compare_test_false() {
    substr_compare_helper("b", "", 2); 
    substr_compare_helper("b", "a", 0, 2); 
}

substr_compare_test_less(); 
substr_compare_test_eq(); 
substr_compare_test_greater(); 
substr_compare_test_false(); 


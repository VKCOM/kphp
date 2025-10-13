@ok k2_skip
<?php
$subject = "abcdef";
$pattern = '/^def/';
preg_match($pattern, $subject, $matches, PREG_OFFSET_CAPTURE, 3);
print_r($matches);


$subject = "abcdef";
$pattern = '/^def/';
preg_match($pattern, substr($subject,3), $matches, PREG_OFFSET_CAPTURE);
print_r($matches);

if (preg_match("/php/i", "PHP is the web scripting language of choice.")) {
    echo "A match was found.\n";
} else {
    echo "A match was not found.\n";
}

/* The \b in the pattern indicates a word boundary, so only the distinct
 * word "web" is matched, and not a word partial like "webbing" or "cobweb" */
if (preg_match("/\bweb\b/i", "PHP is the web scripting language of choice.")) {
    echo "A match was found.\n";
} else {
    echo "A match was not found.\n";
}

if (preg_match("/\bweb\b/i", "PHP is the website scripting language of choice.")) {
    echo "A match was found.\n";
} else {
    echo "A match was not found.\n";
}

// get host name from URL
preg_match('@^(?:http://)?([^/]+)@i',
    "http://www.php.net/index.html", $matches);
$host = $matches[1];

// get last two segments of host name
preg_match('/[^.]+\.[^.]+$/', $host, $matches);
echo "domain name is: {$matches[0]}\n";


$str = 'foobar: 2008';

preg_match('/(?P<name>\w+): (?P<digit>\d+)/', $str, $matches);

/* This also works in PHP 5.2.2 (PCRE 7.0) and later, however 
 * the above form is recommended for backwards compatibility */
// preg_match('/(?<name>\w+): (?<digit>\d+)/', $str, $matches);

print_r($matches);


var_dump(preg_match('/#/u', 'a#',$matches,0,2));
var_dump(preg_match('/#/u', "\xc3\xa4#",$matches,0,2));
var_dump(preg_match('/#/u', "\xc3\xa4#",$matches,0,3));


$text = 'foo bar';
print (int) preg_match('/^bar/',$text,$a,null,4); // prints 0
print (int) preg_match('/\Abar/',$text,$a,null,4); // prints 0
print (int) preg_match('/bar/A',$text,$a,null,4); // prints 1
echo "\n";

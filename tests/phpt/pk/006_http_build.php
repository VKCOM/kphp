@ok
<?php


echo http_build_query(array("v"=>1)) . "\n";
echo http_build_query(array("v"=>"1")) . "\n";
echo http_build_query(array("w"=>array("v"=>1))) . "\n";
echo http_build_query(array("v"=>1, "w"=>"1", "u"=>array(1))) . "\n";

$data = array(
    'foo' => 'bar',
    'baz' => 'boom',
    'cow' => 'milk',
    'php' => 'hypertext processor'
);

echo http_build_query($data) . "\n";
echo http_build_query($data, '', '&amp;');

$data = array('foo', 'bar', 'baz', 'boom', 'cow' => 'milk', 'php' => 'hypertext processor');

echo http_build_query($data) . "\n";
echo http_build_query($data, 'myvar_') . "\n";

$data = array(
    'user' => array(
        'name' => 'Bob Smith',
        'age'  => 47,
        'sex'  => 'M',
        'dob'  => '5/12/1956'
    ),
    'pastimes' => array('golf', 'opera', 'poker', 'rap'),
    'children' => array(
        'bobby' => array('age'=>12, 'sex'=>'M'),
        'sally' => array('age'=>8, 'sex'=>'F')
    ),
    'CEO'
);

echo http_build_query($data, 'flags_', 'separator') . "\n";
echo http_build_query($data, 'flags_', 'separator', PHP_QUERY_RFC1738) . "\n";
echo http_build_query($data, 'flags_', 'separator', PHP_QUERY_RFC3986) . "\n";

echo http_build_query(['test' => 1, 'empty' => null]) . "\n";
echo http_build_query(['test' => 1, 'empty' => null, 'key' => 42]). "\n";
echo http_build_query(['empty' => null, 'test' => 1, 'key' => 42]). "\n";
echo http_build_query(['empty' => null]). "\n";

$data_with_nulls = array(
    'user' => array(
        'name' => null,
        'age'  => 47,
        'sex'  => 'M',
        'dob'  => '5/12/1956'
    ),
    'pastimes' => array('golf', null, 'poker', 'rap'),
    'children' => array(
        'bobby' => array('age'=>12, 'sex'=>null),
        'sally' => array(null)
    ),
    'nulls' => array(null=>12, 'hello'=>null, 'nulls' => array(null=>12, 'hello'=>'world')),
    'CEO'
);

echo http_build_query($data_with_nulls, 'flags_', 'separator') . "\n";
echo http_build_query($data_with_nulls, 'flags_', 'separator', PHP_QUERY_RFC1738) . "\n";
echo http_build_query($data_with_nulls, 'flags_', 'separator', PHP_QUERY_RFC3986) . "\n";

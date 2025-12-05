@ok callback benchmark
<?php

define('RE_URL_PATTERN', '(?<![A-Za-z\$0-9À-ßà-ÿ¸¨\-\_])(https?:\/\/)?((?:[A-Za-z\$0-9À-ßà-ÿ¸¨](?:[A-Za-z\$0-9\-\_À-ßà-ÿ¸¨]*[A-Za-z\$0-9À-ßà-ÿ¸¨])?\.){1,5}[A-Za-z\$ðôóêÐÔÓÊ\-\d]{2,22}(?::\d{2,5})?)((?:\/(?:(?:\&amp;|\&#33;|,[_%]|[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.~=;:]+|\[[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\]|\([A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\))*(?:,[_%]|[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿\-\_#%?+\/\$.~=;:]*[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\_#%?+\/\$~=]|\[[A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\]|\([A-Za-z0-9\xa8\xb8\xc0-\xffºª¥´¯¿²³\-\_#%?+\/\$.,~=;:]*\)))?)?)');

$text = 'ß ñëûøàë, ÷òî â iOS 7 ïîÿâèëèñü ëîêàëüíûå ïóø-óâåäîìëåíèÿ. Íî òóò http://blog.derand.net/2010/08/local-notifications-ios-40.html óòâåðæäàåòñÿ, ÷òî åùå â ÷åòâåðòîé.';
$text = preg_replace_callback('/'.RE_URL_PATTERN.'/', 'prcConvertHyperref', $text);

/**
 * @kphp-required
 * @param string[] $matches
 * @return string
 */
function prcConvertHyperref($matches) {
  return (string)preg_match('/\.([a-zA-ZðôóêÐÔÓÊ\-0-9]+)$/', $matches[2], $match);
}


/**
 * @kphp-required
 * @param string[] $param
 * @return string
 */
function cb($param) {
  var_dump($param);
  return "yes!";
}


$input = "plain [indent] deep [indent] [abcd]deeper[/abcd] [/indent] deep [/indent] plain";

/**
 * @param mixed $input
 * @return string
 */
function parseTagsRecursive($input)
{
	global $count;
    $regex = '#\[indent]((?:[^[]|\[(?!/?indent])|(?R))+)\[/indent]#';
    var_dump ($input);

    if (is_array($input)) {
        $input = '<div style="margin-left: 10px">'.$input[1].'</div>';
    }


    $res = preg_replace_callback($regex, 'parseTagsRecursive', $input, -1, $count);
    var_dump ($count);
    return (string)$res;

}

$output = parseTagsRecursive($input);

echo $output, "\n";


/**
 * @kphp-required
 * @param string[] $x
 * @return string
 */
function g($x) {
	return "'{$x[0]}'";
}

var_dump(preg_replace_callback('@\b\w{1,2}\b@', 'g', array('a b3 bcd', 'v' => 'aksfjk', 12 => 'aa bb')));

@var_dump(preg_replace_callback('~\A.~', 'g', array(array('xyz'))));

/**
 * @kphp-required
 * @param string[] $m
 * @return string
 */
function tmp($m) {
  return strtolower($m[0]);
}

var_dump(preg_replace_callback('~\A.~', 'tmp', 'ABC'));

var_dump(preg_replace_callback("/(ab)(cd)(e)/", "cb", 'abcde'));

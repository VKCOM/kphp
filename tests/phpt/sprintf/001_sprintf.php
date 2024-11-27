@ok
<?php

function nullableString(): ?string {
    return null;
}

function getString(): string {
    return "";
}

function getInt(): string {
    return "";
}

$int = 1;
$string = "Hello";

echo sprintf("%%");
echo sprintf("%% %% %%");

// simple
echo sprintf("%d", $int);
echo sprintf("%d", getInt());
echo sprintf("%s", $string);
echo sprintf("%s", getString());
echo sprintf("%s %d", getString(), $int);
echo sprintf("%s %s", getString(), $string);

// simple with text
echo sprintf("Hello %d", $int);
echo sprintf("%d world", getInt());
echo sprintf("%s world", $string);
echo sprintf("Hello %s world %d", getString(), getInt());

// with non-const format
$format = "%s";
echo sprintf($format, $int);
echo sprintf($format, $int, getString());

// with constant
const FORMAT = "Hello %d%s";
echo sprintf(FORMAT, $int, getString());

// complex example
$html = "";
$html .= sprintf('<td colspan="%s"  category="%s" path="%s"
      onclick="new(\'%s\')">
      %s <a>%s</a>
      <span style="color: #000000; font-size: 8pt;">%s</span>
      <span style="color: #ffffff">%s</span>
      </td></tr>',
    $string, $string, $string, $string, $string, $string, $string, true ? 'no audience' : '');

echo $html;

// with not simple format
echo sprintf("%b", (bool)$int);
echo sprintf("%f", (float)$int);
echo sprintf("%10d", $int);
echo sprintf("%10s", $string);

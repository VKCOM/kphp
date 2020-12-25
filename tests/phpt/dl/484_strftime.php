@ok fixme
<?php

function test() {
    $time = 1234567890;//time();

    $strftimeFormats = array(
        'A' => 'A full textual representation of the day',
        'B' => 'Full month name, based on the locale',
        'C' => 'Two digit representation of the century (year divided by 100, truncated to an integer)',
        'D' => 'Same as "%m/%d/%y"',
        'E' => '',
        'F' => 'Same as "%Y-%m-%d"',
        'G' => 'The full four-digit version of %g',
        'H' => 'Two digit representation of the hour in 24-hour format',
        'I' => 'Two digit representation of the hour in 12-hour format',
        'J' => '',
        'K' => '',
        'L' => '',
        'M' => 'Two digit representation of the minute',
        'N' => '',
        'O' => '',
        'P' => 'lower-case "am" or "pm" based on the given time',
        'Q' => '',
        'R' => 'Same as "%H:%M"',
        'S' => 'Two digit representation of the second',
        'T' => 'Same as "%H:%M:%S"',
        'U' => 'Week number of the given year, starting with the first Sunday as the first week',
        'V' => 'ISO-8601:1988 week number of the given year, starting with the first week of the year with at least 4 weekdays, with Monday being the start of the week',
        'W' => 'A numeric representation of the week of the year, starting with the first Monday as the first week',
        'X' => 'Preferred time representation based on locale, without the date',
        'Y' => 'Four digit representation for the year',
        'Z' => 'The time zone offset/abbreviation option NOT given by %z (depends on operating system)',
        'a' => 'An abbreviated textual representation of the day',
        'b' => 'Abbreviated month name, based on the locale',
        'c' => 'Preferred date and time stamp based on local',
        'd' => 'Two-digit day of the month (with leading zeros)',
        'e' => 'Day of the month, with a space preceding single digits',
        'f' => '',
        'g' => 'Two digit representation of the year going by ISO-8601:1988 standards (see %V)',
        'h' => 'Abbreviated month name, based on the locale (an alias of %b)',
        'i' => '',
        'j' => 'Day of the year, 3 digits with leading zeros',
        'k' => '',
        'l' => 'Hour in 12-hour format, with a space preceding single digits',
        'm' => 'Two digit representation of the month',
        'n' => 'A newline character ("\n")',
        'o' => '',
        'p' => 'UPPER-CASE "AM" or "PM" based on the given time',
        'q' => '',
        'r' => 'Same as "%I:%M:%S %p"',
        's' => 'Unix Epoch Time timestamp',
        't' => 'A Tab character ("\t")',
        'u' => 'ISO-8601 numeric representation of the day of the week',
        'v' => '',
        'w' => 'Numeric representation of the day of the week',
        'x' => 'Preferred date representation based on locale, without the time',
        'y' => 'Two digit representation of the year',
        'z' => 'Either the time zone offset from UTC or the abbreviation (depends on operating system)',
        '%' => 'A literal percentage character ("%")',
    );

    // Results.
    $strftimeValues = array();

    // Evaluate the formats whilst suppressing any errors.
    foreach($strftimeFormats as $format => $description){
      if (false !== ($value = strftime(strval ("%{$format}"), $time))){
        $strftimeValues[$format] = $value;
      }
    }

    // Find the longest value.
    $maxValueLength = 12;

    // Report known formats.
    foreach($strftimeValues as $format => $value){
        echo "Known format   : '{$format}' = ", str_pad("'{$value}'", $maxValueLength), " ( {$strftimeFormats[$format]} )\n";
    }

    // Report unknown formats.
    foreach(array_diff_key($strftimeFormats, $strftimeValues) as $format => $description){
        echo "Unknown format : '{$format}'   ", str_pad(' ', $maxValueLength), ($description ? " ( {$description} )" : ''), "\n";
    }

    var_dump (mktime(14, 59, 38, 2, 29, 2012));
    var_dump (gmmktime(14, 59, 38, 2, 29, 2012));

    var_dump (mktime(0, 0, 0, 1, 1, 1970));
    var_dump (gmmktime(0, 0, 0, 1, 1, 1970));

    var_dump (mktime(0, 0, 0, 1, 1));
    var_dump (gmmktime(0, 0, 0, 1, 1));

    var_dump (mktime(0, 0, 0, 8, 8));
    var_dump (gmmktime(0, 0, 0, 8, 8));

    var_dump (date('D, d M Y H:i:s', $time));
    var_dump (gmdate('D, d M Y H:i:s', $time));

    #var_dump (date('D, d M Y H:i:s', time()));
    #var_dump (gmdate('D, d M Y H:i:s', time()));

    var_dump (checkdate(2, 29, 2000));
    var_dump (checkdate(2, 29, 2100));
    var_dump (checkdate(2, 29, 2200));
    var_dump (checkdate(2, 29, 2004));

    #var_dump (strftime('%r'));
    var_dump (strftime('%r', $time));

    #var_dump (getdate());
    var_dump (getdate($time));

    #var_dump (localtime());
    var_dump (localtime($time));

    #var_dump (localtime(time(), true));
    var_dump (localtime($time, true));
}

date_default_timezone_set ("Etc/GMT-3");
test();
date_default_timezone_set ("Europe/Moscow");
test();

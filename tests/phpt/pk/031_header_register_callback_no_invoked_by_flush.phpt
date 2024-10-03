@ok
<?php

echo "Zero";
header_register_callback(function () {
    echo "In CLI is unreachable, even via flush";
});
flush();
echo "One";
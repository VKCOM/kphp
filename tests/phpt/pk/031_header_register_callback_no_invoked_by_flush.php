@ok k2_skip
k2_skip: call to unsupported function : flush
<?php

echo "Zero";
header_register_callback(function () {
    echo "In CLI is unreachable, even via flush";
});
flush();
echo "One";

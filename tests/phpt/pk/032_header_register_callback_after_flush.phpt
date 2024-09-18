@ok
<?php

echo "Zero ";
header_register_callback(function () {
    echo "Unreachable ";
});

flush();
echo "One ";
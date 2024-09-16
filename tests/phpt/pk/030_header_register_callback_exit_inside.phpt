@ok
<?php

echo "Zero ";
header_register_callback(function () {
	exit("Two ");
    echo("Unreachable ");
});
echo "One ";
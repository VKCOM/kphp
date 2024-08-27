<?php

$query = component_server_accept_query();

while(true) {
// clang++ remove empty loop and it leads to sigsegv. For this reason add echo
    echo(1);
}

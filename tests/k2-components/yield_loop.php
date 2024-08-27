<?php

$query = component_server_accept_query();

while(true) {
    sched_yield();
}

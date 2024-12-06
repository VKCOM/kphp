<?php

namespace Classes;

interface KeysableArrayAccess extends \ArrayAccess {
    /**
     * @return mixed[]
     */
    public function keys();
}

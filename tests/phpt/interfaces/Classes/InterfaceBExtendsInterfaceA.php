<?php

namespace Classes;

interface InterfaceBExtendsInterfaceA extends InterfaceA {

  /**
   * @return InterfaceB
   */
  function getSelfAsB();
}

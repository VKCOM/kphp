<?php

namespace Classes;

interface InterfaceBExtendsInterfaceA extends InterfaceA {

  /**
   * @return InterfaceBExtendsInterfaceA
   */
  function getSelfAsB();
}

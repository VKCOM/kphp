// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <stdint.h>

int parse_ipv4(const char *str, uint32_t *ip, uint32_t *mask);
unsigned get_my_ipv4();
int get_my_ipv6(unsigned char ipv6[16]);

constexpr uint32_t ip2uint(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | d;
}

/**
 * A small wrapper to reduce confusion between IP and mask naming
 */
constexpr uint32_t mask2uint(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return ip2uint(a, b, c, d);
}

constexpr uint32_t LOCALHOST_NETWORK = ip2uint(127, 0, 0, 1);
constexpr uint32_t LOCALHOST_MASK = mask2uint(255, 255, 255, 0); // equivalent CIDR /24

constexpr uint32_t PRIVATE_A_NETWORK = ip2uint(10, 0, 0, 0);
constexpr uint32_t PRIVATE_A_MASK = mask2uint(255, 0, 0, 0); // equivalent CIDR /8

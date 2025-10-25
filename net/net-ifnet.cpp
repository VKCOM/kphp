// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "net/net-ifnet.h"

#include <ifaddrs.h>
#include <memory.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common/kprintf.h"
#include "common/options.h"

static uint32_t force_ipv4_mask = PRIVATE_A_MASK;
static uint32_t force_ipv4_ip = PRIVATE_A_NETWORK;
static char *force_ipv4_interface = NULL;
bool allow_loopback = false;

int parse_ipv4(const char *str, uint32_t *ip, uint32_t *mask) {
  int ip4[4], subnet;
  int x = sscanf(str, "%d.%d.%d.%d/%d", &ip4[0], &ip4[1], &ip4[2], &ip4[3], &subnet);
  if (x < 4) {
    kprintf("Failed to parse ip in %s\n", str);
    return -1;
  }
  if (x == 4) {
    subnet = 32;
  }
  for (int i = 0; i < 4; i++) {
    if (ip4[i] < 0 || ip4[i] >= 256) {
      kprintf("Failed to parse ip in %s\n", str);
      return -1;
    }
  }
  if (subnet <= 0 || subnet > 32) {
    kprintf("Failed to parse subnet in %s\n", str);
    return -1;
  }
  *mask = ~((1 << (32 - subnet)) - 1);
  *ip = (ip4[0] << 24) | (ip4[1] << 16) | (ip4[2] << 8) | ip4[3];
  if (((*ip) & ~(*mask)) != 0) {
    kprintf("Bad ip/subnet pair %s\n", str);
    return -1;
  }
  return 0;
}

OPTION_PARSER(OPT_NETWORK, "force-ipv4", required_argument, "in form [iface:]ip[/subnet]. Forces engine to choose interface and ip in given subnet") {
  char *colon = strchr(optarg, ':');
  char *str = optarg;
  if (colon != NULL) {
    force_ipv4_interface = strndup(str, colon - str);
    str = colon + 1;
  }
  return parse_ipv4(str, &force_ipv4_ip, &force_ipv4_mask);
}

OPTION_PARSER(OPT_NETWORK, "allow-loopback", no_argument, "allow use loopback with address 127.0.0.1") {
  allow_loopback = true;
  return 0;
}

unsigned get_my_ipv4() {
  struct ifaddrs *ifa_first, *ifa;
  unsigned my_ip = 0, my_netmask = -1;
  const char *my_iface = NULL;
  if (getifaddrs(&ifa_first) < 0) {
    perror("getifaddrs()");
    return 0;
  }
  for (ifa = ifa_first; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
      continue;
    }
    if (!allow_loopback && strcmp(ifa->ifa_name, "lo") == 0) {
      continue;
    }
    if (force_ipv4_interface != NULL && strcmp(ifa->ifa_name, force_ipv4_interface) != 0) {
      continue;
    }
    unsigned ip = ntohl(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr);
    unsigned mask = ntohl(((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr.s_addr);
    vkprintf (2, "%08x %08x (%08x == %08x)\t%s\t-> %d,%d\n", ip, mask, (ip & force_ipv4_mask), force_ipv4_ip, ifa->ifa_name, (ip & force_ipv4_mask) == force_ipv4_ip, mask < my_netmask);
    if ((ip & force_ipv4_mask) == force_ipv4_ip && (!my_ip || mask < my_netmask)) {
      my_ip = ip;
      my_netmask = mask;
      my_iface = ifa->ifa_name;
    }
  }

  if (allow_loopback && !my_ip) {
    my_ip = LOCALHOST_NETWORK;
    my_netmask = LOCALHOST_MASK;
    my_iface = "lo";
  }

  // if removed, it can break local development, since not everyone has the `10.0.0.0/8` network on their computer
  if (force_ipv4_mask != PRIVATE_A_MASK || force_ipv4_ip != PRIVATE_A_NETWORK) {
    assert(my_ip != 0 && "can't choose ip in given subnet");
  }
  vkprintf (2, "using main IP %d.%d.%d.%d/%d at interface %s\n", (my_ip >> 24), (my_ip >> 16) & 255, (my_ip >> 8) & 255, my_ip & 255,
            __builtin_clz(~my_netmask), my_iface ?: "(none)");
  freeifaddrs(ifa_first);
  return my_ip;
}

int get_my_ipv6(unsigned char ipv6[16]) {
  struct ifaddrs *ifa_first, *ifa;
  char *my_iface = 0;
  static unsigned char ip[16];
  static unsigned char mask[16];
  if (getifaddrs(&ifa_first) < 0) {
    perror("getifaddrs()");
    return 0;
  }
  int found_auto = 0;
  for (ifa = ifa_first; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET6) {
      continue;
    }
    memcpy(ip, &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, 16);
    vkprintf (4, "test IP %s at interface %s\n", ipv6_to_print(ip), ifa->ifa_name);

    if ((ip[0] & 0xf0) != 0x30 && (ip[0] & 0xf0) != 0x20) {
      vkprintf (4, "not a global ipv6 address\n");
      continue;
    }

    if (ip[11] == 0xff && ip[12] == 0xfe && (ip[8] & 2)) {
      if (found_auto) {
        continue;
      }
      my_iface = ifa->ifa_name;
      memcpy(ipv6, ip, 16);
      memcpy(mask, &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, 16);
      found_auto = 1;
    } else {
      my_iface = ifa->ifa_name;
      memcpy(ipv6, ip, 16);
      memcpy(mask, &((struct sockaddr_in6 *)ifa->ifa_netmask)->sin6_addr, 16);
      break;
    }
  }
  int m = 0;
  while (m < 128 && mask[m / 8] == 0xff) {
    m += 8;
  }
  if (m < 128) {
    unsigned char c = mask[m / 8];
    while (c & 1) {
      c /= 2;
      m++;
    }
  }
  vkprintf (2, "using main IP %s/%d at interface %s\n", ipv6_to_print(ipv6), m, my_iface);
  freeifaddrs(ifa_first);
  return 1;
}

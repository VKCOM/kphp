---
sort: 1
---

# Compiling KPHP from sources

KPHP can be compiled for Debian systems. Public builds are available for Buster and Ubuntu. For Jessie and Stretch, this can also be done with lots of backports.

We have not tested if KPHP can be compiled for a non-Debian system. If you have luck, please let us know.


## Where all sources are located

All KPHP sources are [on Github]({{site.url_github_kphp}}), distributed under the GPLv3 license.  
These are sources of KPHP, vkext, flex data (for Russian declensions), and all TL tools.

KPHP linkage depends on some custom packages, that are also compiled from source:
* patched curl build [on Github]({{site.url_package_curl}}) (branch *dpkg-build-7.60.0*)
* custom uber-h3 build [on Github]({{site.url_package_h3}}) (branch *dpkg-build*)

KPHP compilation depends on some third-party libraries, which are just copied to the `third_party/` folder as is, containing all references to originals. 

KPHP is compiled with CMake and packed with CPack.


## Prerequisites

KPHP compilation, runtime, and linkage depend on the following packages:
```bash
apt install \
        build-essential ccache cmake g++ git gperf \
        libfmt-dev libmsgpack-dev libnghttp2-dev libpcre3-dev libre2-dev \
        libssl-dev libyaml-cpp-dev libzstd-dev lld make \
        php7.4-dev python3-jsonschema python3-minimal unzip zlib1g-dev
```

It's also recommended having a `libfmt >= 7` installed (if not, pass `-DDOWNLOAD_MISSING_LIBRARIES=On` to CMake). 


## CMake configuration

Here are available variables, that can be modified during the `cmake` invocation:
```
DOWNLOAD_MISSING_LIBRARIES when libfmt / gtest / etc not found, downloads them to a local folder [Off]
ADDRESS_SANITIZER enables the address sanitizer [Off]
UNDEFINED_SANITIZER enables the undefined sanitizer [Off]
KPHP_TESTS include tests to default target [On]
```


## Compiling KPHP

```note
We use the `make` command here, but of course, parallel compilation means `make -j$(nproc)` or env settings.
```

```bash
mkdir build
cd build
cmake ..
make
```


## Compiling tl2php and other tools

This is achieved with the same command:
```bash 
# in build/ folder
make
```


## Compiling vkext

To build and install vkext locally, you need to have `php` and `php-dev` packages installed for PHP 7.2 or above.  
vkext is also compiled with CMake.

```
# in build/ folder
make
```

**Install for the default PHP version (recommended)**

```
sudo cp ../objs/vkext/modules/vkext.so $(php-config --extension-dir)
```

If you want to use it everywhere, update the global *php.ini* file by adding the following lines:
```
extension=vkext.so
tl.conffile="/path/to/scheme.tlo"
```

If you don't want to install it globally, pass it as the `-d` option to *php* instead of updating *php.ini*:
```bash
php -d extension=vkext.so -d tl.conffile=/path/to/scheme.tlo ...
```  

**Build for a specific PHP version (optional)**

Run the following script with the desirable *PHP_VERSION* (only "7.2" and "7.4" are supported so far):
```
# in build/ folder
make vkext${PHP_VERSION}
sudo cp ../objs/vkext/modules${PHP_VERSION}/vkext.so $(php-config${PHP_VERSION} --extension-dir)
```

**Compiling .tlo for php.ini**

Use the `tl-compiler` executable from `vk-tl-tools` package:
```bash
tl-compiler -e /path/to/output.tlo input1.tl input2.tl ...
```


## Building a .deb package

```bash
# in build/ folder
cpack -D CPACK_COMPONENTS_ALL="KPHP;FLEX" .
```


## Compiling for Mac OS

We believe that KPHP will be compiled on Mac OS soon (just for local testing without Docker, not for production).

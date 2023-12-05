---
sort: 1
---

# Installation

The easiest way to try KPHP is using Docker, since KPHP is available in the official Docker registry.

```note
Docker is the preferred way to **test out** KPHP. For production, we recommend installing deb packages.
```


## Install KPHP from the Docker registry

Execute the following command:
```bash
docker pull vkcom/kphp
```

Wait a couple of minutes for all dependencies to download and install, and you're done.

Now you are ready for the next step: [Compile a sample PHP script](./compile-sample-script.md).

```warning
Depending on your settings, some systems can require **sudo** for Docker commands (even for *docker ps*).  
If `docker ...` doesn't work, use `sudo docker ...`
```


## Install KPHP from the Dockerfile manually

If you don't want to use the Docker registry, take the latest *Dockerfile* from [here]({{site.url_dockerfile}}) (either *git clone* or just save it).  
In the same folder, execute the following:
```bash
docker build -t kphp .
```

And that's it, you can now proceed to [the next step](./compile-sample-script.md).


## Install KPHP from .deb packages

If you don't want to use Docker at all, feel free to use .deb packages available at the vk.com artifactory.  
This is the preferred way to set up KPHP on production servers.

**Add *artifactory-external.vkpartner.ru* to sources.list.d**

```bash
# trusted key
wget -qO /etc/apt/trusted.gpg.d/vkpartner.asc https://artifactory-external.vkpartner.ru/artifactory/api/gpg/key/public

# for Ubuntu 20.04 (focal)
echo "deb [arch=amd64] https://artifactory-external.vkpartner.ru/artifactory/kphp focal main" | sudo tee /etc/apt/sources.list.d/vkpartner.list

# for Ubuntu 22.04 (jammy)
echo "deb [arch=amd64] https://artifactory-external.vkpartner.ru/artifactory/kphp jammy main" | sudo tee /etc/apt/sources.list.d/vkpartner.list

# for Debian 10 (buster)
echo "deb [arch=amd64] https://artifactory-external.vkpartner.ru/artifactory/kphp buster main" | sudo tee /etc/apt/sources.list.d/vkpartner.list

# for Debian 11 (bullseye)
echo "deb [arch=amd64] https://artifactory-external.vkpartner.ru/artifactory/kphp bullseye main" | sudo tee /etc/apt/sources.list.d/vkpartner.list

```

**Install KPHP packages**

```bash
sudo apt update
sudo apt install kphp vk-tl-tools tlgen

# If you want to use php for development, you need to install vkext of the corresponding php version.
# The following versions are available: 7.4, 8.0, 8.1, 8.2.
sudo apt install php7.4-vkext

sudo mkdir -p /var/www/vkontakte/data/www/vkontakte.com/tl/
sudo tlgen -ignoreGeneratedCode -tloPath /var/www/vkontakte/data/www/vkontakte.com/tl/scheme.tlo /usr/share/vkontakte/examples/tl-files/common.tl /usr/share/vkontakte/examples/tl-files/tl.tl
```



Having done all these steps, you'll have the `kphp` command and related ones to use later on.

You can now proceed to [the next step](./compile-sample-script.md).


## Install KPHP for MacOS

KPHP isn't available as a Homebrew package, but can be [compiled from sources](../kphp-internals/developing-and-extending-kphp/compiling-kphp-from-sources.md) or run using Docker.


## Install KPHP for Windows

KPHP isn't available on Windows. Strange as it might seem, please use Docker instead.


## Install KPHP from source

Typically, the only time when you'd need to compile KPHP from scratch is when you're installing it on a Unix-based system without *apt* support.
  
This process is rather complicated and therefore described on [a separate page](../kphp-internals/developing-and-extending-kphp/compiling-kphp-from-sources.md).

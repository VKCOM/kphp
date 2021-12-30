FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-utils ca-certificates gnupg wget && \
    wget -qO - https://repo.vkpartner.ru/GPG-KEY.pub | apt-key add - && \
    echo "deb https://repo.vkpartner.ru/kphp-focal/ focal main" >> /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends kphp php7.4-vkext vk-flex-data vk-tl-tools && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/www/vkontakte/data/www/vkontakte.com/tl/ && \
    tl-compiler -e /var/www/vkontakte/data/www/vkontakte.com/tl/scheme.tlo -w 4 \
    /usr/share/vkontakte/examples/tl-files/common.tl /usr/share/vkontakte/examples/tl-files/tl.tl

RUN useradd -ms /bin/bash kitten

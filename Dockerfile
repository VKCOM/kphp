FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends apt-utils ca-certificates gnupg wget && \
    wget -qO /etc/apt/trusted.gpg.d/vkpartner.asc https://artifactory-external.vkpartner.ru/artifactory/api/gpg/key/public && \
    echo "deb https://artifactory-external.vkpartner.ru/artifactory/kphp focal main" >> /etc/apt/sources.list && \
    echo "deb https://artifactory.mvk.com/artifactory/debian bullseye-backports main" >> /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y --no-install-recommends kphp php7.4-vkext vk-flex-data vk-tl-tools tlgen && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /var/www/vkontakte/data/www/vkontakte.com/tl/ && \
    tlgen -ignoreGeneratedCode -tloPath /var/www/vkontakte/data/www/vkontakte.com/tl/scheme.tlo \
    /usr/share/vkontakte/examples/tl-files/common.tl /usr/share/vkontakte/examples/tl-files/tl.tl

RUN useradd -ms /bin/bash kitten

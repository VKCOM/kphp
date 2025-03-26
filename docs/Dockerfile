FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && \
    apt install -y --no-install-recommends \
      build-essential ruby-dev rubygems zlib1g-dev libssl-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /kphp-docs/
COPY Gemfile /kphp-docs/
RUN gem install ffi -v 1.16.3 && \
        gem install public_suffix -v 5.1.1 && \
        gem install jekyll -v 3.9.0 && \
        gem install bundler -v 2.4.22 && \
        bundle install
EXPOSE 4000 35729
ENTRYPOINT bundle exec jekyll server --livereload --incremental --host 0.0.0.0 \
            --config /kphp-docs/src/_config.yml \
            --source /kphp-docs/src/ \
            --destination /kphp-docs/site/

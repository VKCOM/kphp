FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update && \
    apt install -y --no-install-recommends \
      build-essential ruby-dev rubygems zlib1g-dev libssl-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /kphp-docs/
COPY Gemfile /kphp-docs/
RUN gem install jekyll bundler && bundle install
EXPOSE 4000 35729
ENTRYPOINT bundle exec jekyll server --livereload --incremental --host 0.0.0.0 \
            --config /kphp-docs/src/_config.yml \
            --source /kphp-docs/src/ \
            --destination /kphp-docs/site/

# KPHP website

This folder is a KPHP website, available at [https://vkcom.github.io/kphp/](https://vkcom.github.io/kphp/).

It is based on Jekyll and works on Github Pages.


## How to launch the site locally

1. Make sure **ruby** and **bundle** are installed
2. Install all dependencies in the site folder: `gem install jekyll bundler && bundle install`
3. Launch:
```bash
bundle exec jekyll server --livereload --incremental --config _config.yml
```
4. Wait until "Generating..." stage is done and proceed to *http://127.0.0.1:4000*

If an incremental build stops working, remove the file `.jekyll-metadata`.

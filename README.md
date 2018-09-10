# How to build

Get emscripten, and have em++ available in your PATH
```
git submodule update --init
make
```

# How to run

```
sudo nginx -g 'daemon off;' -p . -c nginx.conf
```

Access localhost from your webbrowser, and wala.

# TODO

- more features
- refactor build system mess
- desktop backend with glfw or something

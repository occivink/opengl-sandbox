Deps:

emscripten (em++)
glm
imgui (included)

Launch nginx:
sudo nginx -g 'daemon off;' -c "$PWD/nginx.conf"

Launch the server
shell2http -cgi GET:/RandomImage "cat $(find . -name '*png' | shuf -n1)" POST:/ProcessImage "if [ -f ~/bro.png ]; then ffmpeg -i pipe:0 -i ~/bro.png -filter_complex hstack -c:v png -f image2pipe pipe:1 && rm ~/bro.png; else cat > ~/bro.png; fi

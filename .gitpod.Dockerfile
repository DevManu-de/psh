FROM debian:latest

USER gitpod

RUN sudo apt-get -q update && sudo apt-get install -yq libreadline-dev \
libgoogle-perftools-dev google-perftools gcc make curl zsh
RUN ln -f /bin/zsh /bin/sh

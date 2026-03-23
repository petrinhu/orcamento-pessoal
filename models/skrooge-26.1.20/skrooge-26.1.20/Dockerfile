FROM ubuntu:impish 

MAINTAINER Stéphane MANKOWSKI <stephane@mankowski.fr> 

RUN apt-get update -y && \ 
apt-get install --fix-missing -y software-properties-common && \ 
add-apt-repository ppa:s-mankowski/ppa-kf5 && \
apt-get update -y && \ 
apt-get install --fix-missing -y skrooge-kf5 && \ 
apt-get clean -y 

ENV HOME /home/user-sk
RUN useradd --create-home --home-dir $HOME user-sk && chown -R user-sk:user-sk $HOME

RUN echo "QT_X11_NO_MITSHM=1" >> /etc/environment

USER user-sk

ENV QT_X11_NO_MITSHM=1
ENV DISPLAY=:1
ENV SHELL=/bin/bash

RUN mkdir -p $HOME/.kde/share
VOLUME $HOME/.kde/share
WORKDIR /home/user-sk

CMD export $(dbus-launch) && /usr/bin/skrooge

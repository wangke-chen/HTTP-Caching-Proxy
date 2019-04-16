FROM ubuntu:18.04
RUN mkdir /code
RUN mkdir /var/log/erss
RUN apt-get update && apt-get -v install build-essential g++ make net-tools
WORKDIR /code
ADD . /code/
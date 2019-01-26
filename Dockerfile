# This dockerfile can be used to both build a containerized version of the application
# and to self-test its build.
# * To self-test, simply `docker build -t opustags .`; `make check` is run as the final step anyway.
# * To use the dockerized version, `docker run -it opustags opustags -h` (etc.)

FROM ubuntu:18.04
RUN apt-get update
RUN apt-get install --no-install-recommends -y make cmake g++ libogg-dev pkg-config ffmpeg liblist-moreutils-perl locales
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
ADD . /src
WORKDIR /build
RUN env CXX=g++ cmake /src && make && make install
# We need to run tests as a regular user, since on Docker /dev is a writable directory
# for root (it's backed by a tmpfs).  This would make the "device as partial file" test fail.
RUN useradd ubuntu
RUN mkdir -p /build && chown -R ubuntu:ubuntu /build /src
USER ubuntu
RUN env CXX=g++ make check
# Copied from https://github.com/sleepyfox/janet-lang-docker/blob/master/Dockerfile
# This dockerfile is for running valgrind on our build output, since valgrind doesn't
# currently work on MacOS Mojave or later.
#
# Some reference material:
#  - https://medium.com/@yutafujii_59175/a-complete-one-by-one-guide-to-install-docker-on-your-mac-os-using-homebrew-e818eb4cfc3
#  - https://www.gungorbudak.com/blog/2018/06/13/memory-leak-testing-with-valgrind-on-macos-using-docker-containers/

FROM ubuntu:19.10

# Install make
RUN apt -q update \
  && DEBIAN_FRONTEND=noninteractive apt install -yq \
  make gcc git curl valgrind libpq-dev

# Install Janet
RUN cd /tmp && \
    git clone https://github.com/janet-lang/janet.git && \
    cd janet && \
    make all test install && \
    rm -rf /tmp/janet
RUN chmod 777 /usr/local/lib/janet

# Set group and user IDs for docker user
ARG GID=1000
ARG UID=1000
ARG USER=me

# Create the group and user
RUN groupadd -g $GID $USER
RUN useradd -g $GID -M -u $UID -d /var/app $USER

# Application setup
COPY ./ /var/app
WORKDIR /var/app
# RUN jpm deps
# RUN jpm build
# USER $USER

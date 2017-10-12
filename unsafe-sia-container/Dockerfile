# WARNING: This Dockerfile exposes your sia control port potentially without a
# password using socat and working around Sia standard security features.
# Be careful when using this Dockerfile.
FROM debian

ARG SIA_VERSION=1.3.0

RUN apt-get update && \
    apt-get install -y socat wget unzip && \
    mkdir -p /sia && \
    wget -O /tmp/sia.zip https://github.com/NebulousLabs/Sia/releases/download/v$SIA_VERSION/Sia-v$SIA_VERSION-linux-amd64.zip && \
    unzip -d /tmp/sia/ /tmp/sia.zip && \
    mv /tmp/sia/Sia-v$SIA_VERSION-linux-amd64/siac /usr/local/bin/ && \
    mv /tmp/sia/Sia-v$SIA_VERSION-linux-amd64/siad /usr/local/bin/ && \
    rm -rf /tmp/sia/ /tmp/sia.zip /var/lib/apt/lists

# for custom or locally compiled versions, uncomment the following lines
# ADD siac /usr/local/bin
# ADD siad /usr/local/bin

# WARNING:  See warning at top of file
WORKDIR /sia
EXPOSE 8000 9981 9982

# socat allows us to work around Sia security container incompatibility bug
ENTRYPOINT socat tcp-listen:8000,reuseaddr,fork tcp:localhost:9980 & /usr/local/bin/siad --modules gctewhr --sia-directory /sia

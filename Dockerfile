FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential gcc-multilib nasm python3 grub-pc-bin grub-common xorriso mtools
WORKDIR /src
CMD ["bash","-c","chmod +x build.sh && ./build.sh"]

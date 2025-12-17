FROM scratch
ARG TARGETARCH
COPY build/sayt-${TARGETARCH}.elf /sayt
CMD ["/sayt"]

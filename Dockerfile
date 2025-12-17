FROM scratch
ARG TARGETARCH
COPY pkg/bin/sayt-${TARGETARCH}.elf /sayt
CMD ["/sayt"]

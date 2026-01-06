FROM gcc:13

WORKDIR /app

COPY broker.c broker.h ../common/protocol.h ./

RUN gcc broker.c -o broker

EXPOSE 1883

CMD ["./broker"]

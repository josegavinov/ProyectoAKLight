FROM gcc:13

WORKDIR /app

COPY broker/broker.c broker/broker.h common/protocol.h ./

RUN gcc broker.c -o broker

EXPOSE 1883

CMD ["./broker"]

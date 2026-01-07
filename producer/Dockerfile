FROM gcc:13

WORKDIR /app

COPY producer/producer.c common/protocol.h ./

RUN gcc producer.c -o producer

CMD ["./producer"]

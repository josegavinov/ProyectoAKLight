FROM gcc:13
WORKDIR /app

# Broker
COPY broker/broker.c broker/broker.h common/protocol.h ./
RUN gcc broker.c -o broker

# Producer
COPY producer/producer.c common/protocol.h ./
RUN gcc producer.c -o producer

# Consumer
COPY consumer/consumer.c common/protocol.h ./
RUN gcc consumer.c -o consumer

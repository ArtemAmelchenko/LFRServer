FROM ubuntu:focal

COPY ./build/ /server

WORKDIR /server

RUN cp -rp /server/boost_1_79_0/lib/* /usr/lib/

EXPOSE 8080
EXPOSE 8081


CMD [ "./CentralServer" ]

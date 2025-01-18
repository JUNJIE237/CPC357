FROM quay.io/mongodb/charts:v1.20.2

LABEL maintainer="thepliable2@gmail.com"


ARG EMAIL
ARG PASSWORD

ENV EMAIL=${EMAIL} \
    PASSWORD=${PASSWORD}

COPY entrypoint.sh /usr/bin

ENTRYPOINT [ "entrypoint.sh" ]
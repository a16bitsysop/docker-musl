ARG DVER=edge
ARG NME=builder

##################################################################################################
FROM alpine:${DVER} AS buildbase
ARG NME

RUN apk update
# install abuild deps and add /tmp/pkg to repositories
RUN apk add --no-cache -u alpine-conf alpine-sdk atools findutils gdb git pax-utils sudo \
&&  echo /tmp/pkg >> /etc/apk/repositories

# setup build user
RUN adduser -D ${NME} && addgroup ${NME} abuild \
&&  echo "Defaults  lecture=\"never\"" > /etc/sudoers.d/${NME} \
&&  echo "${NME} ALL=NOPASSWD : ALL" >> /etc/sudoers.d/${NME} \
&&  sed "s/ERROR_CLEANUP.*/ERROR_CLEANUP=\"\"/" -i /etc/abuild.conf

# create keys and copy to global folder, switch to build user
RUN su ${NME} -c "abuild-keygen -a -i -n"
USER ${NME}
WORKDIR /tmp/pkg

##################################################################################################
FROM buildbase AS builddep
ARG NME
ENV APORT=musl
ENV REPO=main

# pull source on host with
# pull-apk-source.sh main/lttng-ust

# copy aport folder into container
WORKDIR /tmp/${APORT}
COPY ${APORT} ./
RUN sudo chown -R ${NME}:${NME} ../${APORT}

RUN pwd && ls -RC
RUN abuild checksum
RUN abuild deps
RUN echo "Arch is: $(abuild -A)" && abuild -K -P /tmp/pkg

##################################################################################################
FROM buildbase AS buildaport
ARG NME
ENV APORT=statx-test

# copy built packages from previous step
COPY --from=builddep /tmp/pkg/* /tmp/pkg/
RUN ls -RC /tmp/pkg

# pull source on host with
# pull-apk-source.sh community/lttng-tools

# copy aport folder into container
WORKDIR /tmp/${APORT}
COPY ${APORT} ./
RUN sudo chown -R ${NME}:${NME} ../${APORT}
RUN sudo apk -U upgrade
RUN sudo apk add musl-dev

RUN pwd && ls -RC
RUN make
RUN make test

COPY statx-test /usr/local/bin/

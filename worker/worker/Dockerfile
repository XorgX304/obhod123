FROM ubuntu:16.04

LABEL maintainer="Obhod123 <Obhod123@vpn.com>"

WORKDIR /root

######### install DEV TOOLS #################
RUN apt-get update
RUN apt-get install -yy gcc make libqt5core5a

######### install Open VPN #################
RUN apt-get install -yy \
	openvpn \
	iptables \
	openssl \
	libssl-dev \
	net-tools \
	libgnutls-dev \
	gnutls-bin

RUN apt-get clean

######### install Easy RSA #################
RUN apt-get install -yy unzip
COPY ./openvpn/easy-rsa-3.0.5.zip /root
RUN unzip /root/easy-rsa-3.0.5.zip -d /tmp
RUN mv /tmp/easy-rsa-3.0.5/easyrsa3  /usr/share/easy-rsa
RUN ls -l /usr/share/easy-rsa/
RUN ln -s /usr/share/easy-rsa/easyrsa /usr/local/bin

RUN rm -rf /tmp/* /var/tmp/* /var/cache/apk/* /var/cache/distfiles/*


# Needed by scripts
ENV OPENVPN /etc/openvpn
ENV EASYRSA /usr/share/easy-rsa
ENV EASYRSA_PKI $OPENVPN/pki
ENV EASYRSA_VARS_FILE $OPENVPN/vars

# Prevents refused client connection because of an expired CRL
ENV EASYRSA_CRL_DAYS 3650

#VOLUME ["/etc/openvpn"]

ADD ./openvpn/bin /usr/local/bin
#RUN chmod a+x /usr/share/easy-rsa/easyrsa/*

# Add support for OTP authentication using a PAM module
ADD ./openvpn/otp/openvpn /etc/pam.d/


###################################################################


######### install nxweb #################
COPY ./nxweb/nxweb.tar.gz /root

RUN tar -zxvf /root/nxweb.tar.gz

WORKDIR /root/nxweb-3.3.0-dev
RUN ls -al /root/
RUN ./configure --with-gnutls
RUN make
RUN make install

WORKDIR /root
RUN rm -rf /root/nxweb-3.3.0-dev
RUN rm -rf /root/nxweb.tar.gz

RUN nxweb -v

######### copy nxweb lib #################
COPY ./nxweb/nxweb_config.json /root/
COPY ./build-worker-Desktop_Qt_5_12_3_GCC_64bit4-Release/api/libapi_v_1.so /root/
COPY ./build-worker-Desktop_Qt_5_12_3_GCC_64bit4-Release/root/libroot_handler.so /root/



######### RUN #################
#CMD ["ovpn_run"]
CMD ["nxweb", "-H", "0.0.0.0:80", "-w", "/root/"]

# Internally uses port 1194/udp, remap using `docker run -p 443:1194/tcp`
EXPOSE 1194/udp
EXPOSE 80/tcp
EXPOSE 443/tcp

FROM debian:jessie

ADD main.py /

RUN apt-get -q update \
	&& apt-get -qy install \
		bluez-hcidump \
		libusb-dev \
		expect \
		curl \
		debootstrap \
		python \
		python-pip \
		python-pygame \
		xboxdrv \
		build-essential \
		bluez bluez-tools \
		python-dev python-pip \
		libglib2.0-dev libboost-python-dev libboost-thread-dev libbluetooth-dev && \
		pip install pybluez gattlib \
	&& rm -rf /var/lib/apt/lists/*

RUN pip install awscli

RUN gpg --recv-keys --keyserver ha.pool.sks-keyservers.net 0x9165938D90FDDD2E

COPY . /usr/src/mkimage

WORKDIR /usr/src/mkimage

#CMD bluetoothctl && trust 5C:BA:37:FE:E0:03 && pair 5C:BA:37:FE:E0:03 && connect 5C:BA:37:FE:E0:03 && python ./main.py

#CMD trust 5C:BA:37:FE:E0:03

#CMD pair 5C:BA:37:FE:E0:03

#CMD connect 5C:BA:37:FE:E0:03

CMD ./build.sh

#CMD [ "python", "./main.py" ]

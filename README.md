Replica Set Verifier
====================

RSV is a unix service that checks if your master slave replica is running, this chunk of code is based on C.

####SAMPLE BUILD:

    $ make
    $ sudo make install

####REQUIREMENTS:

LIBBSON 0.4.0

MONGO-C-DRIVER 0.8.1

####CONFIG FILE:

    $ /etc/master_slave_verifier.conf 

config file example:

    [NAME]
    name=rs
    number_of_servers=3
    [SERVERS]
    path=12.0.0.10:27017
    path=12.0.0.11:27017
    path=12.0.0.12:27017

####EXECUTE:

    $ su -c "cd /opt/master_slave_verifier; ./master_slave_verifier"

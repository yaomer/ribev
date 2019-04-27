# /bin/sh

cc serv.c -L ../../src -lribev-serv -o serv
cc cli.c -L ../../src -lribev-cli -o cli

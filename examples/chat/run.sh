# /bin/sh

cc serv.c coder.c -L ../../src -lribev-serv -o serv
cc cli.c coder.c -L ../../src -lribev-cli -o cli

#!/bin/bash

port=${RANDOM}
export LD_LIBRARY_PATH=lib

bin/chat_server_async ${port} &
pid=${!}

trap "kill ${pid}" SIGINT 

bin/chat_client_async 127.0.0.1 ${port}

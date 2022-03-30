#!/bin/bash

i=${1:-3}

if (( i > 3 || i < 0)); then
      exit 1
fi

export LD_LIBRARY_PATH=lib

pids=()
names=(net_async asio_async_ssl websocket_async websocket_async_ssl)
client=(net_client_async asio_client_async_ssl websocket_client_async websocket_client_async_ssl)
server=(net_server_async asio_server_async_ssl websocket_server_async websocket_server_async_ssl)
gateway=(net_gateway_async asio_gateway_async_ssl websocket_gateway_async websocket_gateway_async_ssl)
services=$(awk '/ .*\..*\..*\..* /{print $3}' bin/hosts.conf)

run()
{
    for port in ${services[@]}; do
        bin/${2} 0.0.0.0 ${port} &
        pids[${#pids[@]}]=${!}
    done

    sleep 2
    port=9090
    bin/${3} 0.0.0.0 ${port} bin/hosts.conf &
    pids[${#pids[@]}]=${!}

    for service in {1..3}; do
        bin/${1} 127.0.0.1 ${port} "[]<typename ...>(){}();" ${service}
    done 
}

echo ${names[i]}
run ${client[i]} ${server[i]} ${gateway[i]}

sleep 2
for ((i=${#pids[@]}-1; i >= 0; --i)); do
      kill ${pids[i]}
done

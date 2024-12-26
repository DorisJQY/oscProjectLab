make all
port=5678
clients=9
echo -e "starting gateway "
./sensor_gateway $port $clients &
sleep 3
echo -e 'starting 9 sensor nodes'
./sensor_node 15 1 127.0.0.1 $port &

./sensor_node 21 2 127.0.0.1 $port &

./sensor_node 37 3 127.0.0.1 $port &

./sensor_node 132 4 127.0.0.1 $port &

./sensor_node 142 4 127.0.0.1 $port &

./sensor_node 49 5 127.0.0.1 $port &

./sensor_node 112 6 127.0.0.1 $port &

./sensor_node 129 7 127.0.0.1 $port &

./sensor_node 250 3 127.0.0.1 $port &
sleep 11
killall sensor_node
sleep 30
killall sensor_gateway

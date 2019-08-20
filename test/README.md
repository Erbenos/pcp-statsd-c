Tests are written in *Ruby*, so make sure you have that on your machine.

Install test dependencies:
```
bundle install
```

Tests assume that *pmdastatsd* binary is built in agent folder, so if you didnt already, build it with:
```
make
```

Run tests:
```
./test.rb
```

Tests may take a while to finish as the agent is restarted many times.

Coverage:
- activating agent
- deactivating agent
- all hardcoded metrics are setup and update correctly
- checks that all ini options are editable by agent, fallbacks are used when needed and are actually impactful
    - max_udp_packet_size
    - port
    - max_unprocessed_packets
    - verbose
    - debug
    - debug_output_filename
    - duration_aggregation_type
    - parser_type
- metrics
    - creation of metrics
    - update of metrics
        - checks for overflow / underflow
- misc

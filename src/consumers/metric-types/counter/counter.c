#include <string.h>
#include "../../../config-reader/config-reader.h"
#include "../../../statsd-parsers/statsd-parsers.h"
#include "../../../utils/utils.h"
#include "../../consumers.h"

void record_counter(statsd_datagram* datagram, bucket_collection* buckets, agent_config* config) {
  print_out_datagram(datagram);
  /*
    int (*current_datagram_meta)(bucket_collection*);
    bucket* result = find_bucket(LAMBDA(int, (bucket* result), {
      if (strcmp(((bucket*)result)->metric, datagram->metric) == 0) {
        return 1;
      }
      return -1;
    }));
    if (result == NULL) {
      printf("BUCKET NOT FOUND\n");
      bucket* new_bucket = create_bucket_from_datagram(datagram);
      add_bucket(new_bucket);
    } else {
      printf("BUCKET_FOUND\n");
    }
  */
}

#include <cpp_redis/cpp_redis>
#include <cpp_redis/core/client.hpp>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>
#include <json/json.h>
#include <string>
#include <fstream>
#include <string.h>

std::string get_height_padded(int height) {
  char buf[16];
  sprintf(buf, "%09d", height);
  return std::string(buf);
}

std::string get_height_key(int height) {
  return std::string("h") + get_height_padded(height);
}

struct blockinfo {
  char id[32];
  uint32_t height;
  uint32_t size;
};

int main(int argc, char **argv) {
  cpp_redis::client redis_conn;
  std::string last_wrote_height = "";
  int last = -1;

  redis_conn.connect();
  redis_conn.get("const_lwh", [&last_wrote_height](cpp_redis::reply& reply) {
    if (!reply.is_null()) {
      last_wrote_height = reply.m_strval;
    } else {
      std::cerr << "Error: No blocks in Redis database\n";
      exit(1);
    }
  });
  redis_conn.sync_commit();
  int i;
  if (last_wrote_height.size() > 0) {
    last = atoi(last_wrote_height.c_str());
  }
  std::vector<std::string> hashkeys;
  for (i = 0; i < last; ++i) {
    redis_conn.get(get_height_key(i), [i,&hashkeys](cpp_redis::reply& reply) {
      if (!reply.is_null()) {
        hashkeys.push_back(reply.m_strval);
      } else {
        std::cerr << "Error: missing block at height " + i << "\n";
        exit(1);
      }
    });
  }
  redis_conn.sync_commit();
  std::vector<struct blockinfo> info;
  std::ofstream outfile_bin("blocks.rdb");
  std::ofstream outfile_meta("meta.rdb");
  for (i = 0; i < last; ++i) {
    redis_conn.get("H"+hashkeys[i], [i,&hashkeys,&outfile_bin,&outfile_meta](cpp_redis::reply& reply) {
      if (!reply.is_null()) {
        blockinfo bi;
        bi.height = i; //bi.binblock = reply.m_strval;
        memcpy(bi.id, hashkeys[i].c_str(), sizeof(bi.id));
        bi.size = reply.m_strval.size();
        outfile_bin.write(reply.m_strval.data(), bi.size);
        outfile_meta.write((const char *) &bi, sizeof(bi));
      } else {
        std::cerr << "Error: missing block binary at height " + i << "\n";
        exit(1);
      }
    });
  }
  std::cout << "Read " << hashkeys.size() << " blocks\n";
  redis_conn.sync_commit();
  return 0;
}

#include <cpp_redis/cpp_redis>
#include <cpp_redis/core/client.hpp>
#include <restclient-cpp/connection.h>
#include <restclient-cpp/restclient.h>
#include <json/json.h>
#include <string>
#include <sstream>

RestClient::Connection* sia_conn;
void restinit(void) {
  RestClient::init();

  sia_conn = new RestClient::Connection("http://localhost:9980");

  sia_conn->SetUserAgent("Sia-Agent");
}
std::string fetch_rest_string(std::string path) {
  RestClient::Response r = sia_conn->get(path);
  if (r.code / 100 != 2)
    return "";
  return r.body;
}

Json::Value fetch_rest(std::string path) {
  RestClient::Response r = sia_conn->get(path);
  if (r.code / 100 != 2)
    return "";
  Json::Value root;
  std::istringstream str(r.body);
  str >> root;
  return root;
}

std::string dehex(std::string input) {
  int len = input.size() / 2;
  int i;
  char s[3];
  s[2] = 0;
  std::ostringstream accum;
  for (i = 0; i < len; ++i) {
    s[0] = input[2*i+0];
    s[1] = input[2*i+1];
    unsigned char result = std::stoul(s, nullptr, 16);
    accum << result;
  }
  return accum.str();
}

std::string get_height_padded(int height) {
  char buf[16];
  sprintf(buf, "%09d", height);
  return std::string(buf);
}

std::string get_height_key(int height) {
  return std::string("h") + get_height_padded(height);
}

int main(int argc, char **argv) {
  restinit();
  cpp_redis::client redis_conn;
  Json::Value root = fetch_rest("/consensus");
  int height = root["height"].asInt64();
  std::cout << height << "\n";
  std::string last_wrote_height = "";
  int last = -1;

  redis_conn.connect();
//  redis_conn.flushall();
  //  int height = root["height"].asInt64();

  redis_conn.get("const_lwh", [&last_wrote_height](cpp_redis::reply& reply) {
    if (!reply.is_null()) {
      last_wrote_height = reply.m_strval;
    }
  });
  redis_conn.sync_commit();
  if (last_wrote_height.size() > 0) {
    std::istringstream ist(last_wrote_height);
    ist >> last;
    std::cout << "last " << last << "\n";
    redis_conn.get(get_height_key(last), [](cpp_redis::reply& reply) {
      std::cout << "points to " << reply.m_strval.size() << "\n";
    });
    redis_conn.sync_commit();
  }
  else {
    std::cout << "lwhis is empty\n";
  }

  last -= 5;
  if (last < -1)
    last = -1;
  for (last++; last <= height; ++last) {
    std::ostringstream path;
    path << "/explorer/blocks/" << last;
    Json::Value obj = fetch_rest(path.str()+"?hexblock=true");
    std::string result = obj["block"]["hexblock"].asString();
    if (result.size() < 20) {
      std::cerr << "Error, hexblock is too short for " << last << "\n";
      exit(1);
    }
    std::string blockid = dehex(obj["block"]["blockid"].asString());
    std::string hashkey = std::string("H") + blockid;
    std::string binblock = dehex(result);
//    std::cout << binblock;
    redis_conn.set(hashkey, binblock);
//    std::cout << "block id is " << blockid << "\n";
    redis_conn.set(get_height_key(last), blockid);
    redis_conn.set("const_lwh", get_height_padded(last));
    if (last%1000 == 0 || height - last < 5) {
      redis_conn.sync_commit();
      std::cout << last << "\n";
    }
  }
//! also support std::future
//! std::future<cpp_redis::reply> get_reply = client.get("hello");

//! or client.commit(); for synchronous call
return 0;
}

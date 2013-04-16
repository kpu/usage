#include <fstream>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>

#include <ctype.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace {
float FloatSec(const struct timeval &tv) {
  return static_cast<float>(tv.tv_sec) + (static_cast<float>(tv.tv_usec) / 1000000.0);
}
float FloatSec(const struct timespec &tv) {
  return static_cast<float>(tv.tv_sec) + (static_cast<float>(tv.tv_nsec) / 1000000000.0);
}

const char *SkipSpaces(const char *at) {
  for (; *at == ' ' || *at == '\t'; ++at) {}
  return at;
}

void PrintUsage(std::ostream &out) {
  // Linux doesn't set memory usage in getrusage :-(
  std::set<std::string> headers;
  headers.insert("VmPeak:");
  headers.insert("VmRSS:");
  headers.insert("Name:");

  std::ifstream status("/proc/self/status", std::ios::in);
  std::string header, value;
  while ((status >> header) && getline(status, value)) {
    if (headers.find(header) != headers.end()) {
      out << header << SkipSpaces(value.c_str()) << '\t';
    }
  }

  struct rusage usage;
  if (getrusage(RUSAGE_CHILDREN, &usage)) {
    perror("getrusage");
    return;
  }
  out << "RSSMax:" << usage.ru_maxrss << " kB" << '\t';
  out << "user:" << FloatSec(usage.ru_utime) << "\tsys:" << FloatSec(usage.ru_stime) << '\t';
  out << "CPU:" << (FloatSec(usage.ru_utime) + FloatSec(usage.ru_stime));
  // Note: caller is responsible for ending or continuing line.
}

void PrintUsage(std::ostream &out, const struct timespec &started) {
  PrintUsage(out);
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  out << "\treal:" << (FloatSec(current) - FloatSec(started)) << '\n';
}

void FDWrite(int fd, const std::string &str) {
  const char *start = str.data();
  const char *end = start + str.size();
  while (start != end) {
    ssize_t ret = write(fd, start, end - start);
    if (ret == -1) return;
    start += ret;
  }
}

class Global {
  public:
    Global() {
      clock_gettime(CLOCK_MONOTONIC, &started_);
      // Some processes close stderr.
      fd_ = dup(2);
    }
    ~Global() {
      std::stringstream buf;
      PrintUsage(buf, started_);
      if (fd_ != -1) {
        FDWrite(fd_, buf.str());
        // I'm supposed to check the return value, but there's no stderr to print to!
        (void)close(fd_);
      }
    }

  private:
    struct timespec started_;
    int fd_;
};
const Global kGlobal;
} // namespace

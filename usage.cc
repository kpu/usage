#include <fstream>
#include <iostream>
#include <ostream>
#include <set>
#include <sstream>
#include <string>

#include <ctype.h>
#include <err.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

namespace {

// gcc possible-unused function flags
double Subtract(time_t first, time_t second) __attribute__ ((unused));
double DoubleSec(time_t tv) __attribute__ ((unused));
double Subtract(const struct timeval &first, const struct timeval &second) __attribute__ ((unused));
double Subtract(const struct timespec &first, const struct timespec &second) __attribute__ ((unused));
double DoubleSec(const struct timeval &tv) __attribute__ ((unused));
double DoubleSec(const struct timespec &tv) __attribute__ ((unused));

// These all assume first > second
double Subtract(time_t first, time_t second) {
  return difftime(first, second);
}
double DoubleSec(time_t tv) {
  return static_cast<double>(tv);
}
double Subtract(const struct timeval &first, const struct timeval &second) {
  return static_cast<double>(first.tv_sec - second.tv_sec) + static_cast<double>(first.tv_usec - second.tv_usec) / 1000000.0;
}
double Subtract(const struct timespec &first, const struct timespec &second) {
  return static_cast<double>(first.tv_sec - second.tv_sec) + static_cast<double>(first.tv_nsec - second.tv_nsec) / 1000000000.0;
}
double DoubleSec(const struct timeval &tv) {
  return static_cast<double>(tv.tv_sec) + (static_cast<double>(tv.tv_usec) / 1000000.0);
}
double DoubleSec(const struct timespec &tv) {
  return static_cast<double>(tv.tv_sec) + (static_cast<double>(tv.tv_nsec) / 1000000000.0);
}

const char *SkipSpaces(const char *at) {
  for (; *at == ' ' || *at == '\t'; ++at) {}
  return at;
}

class RecordStart {
  public:
    RecordStart() {
      if (-1 == clock_gettime(CLOCK_MONOTONIC, &started_)) 
        err(1, "Failed to read CLOCK_MONOTONIC");
    }

    const struct timespec &Started() const {
      return started_;
    }

  private:
    struct timespec started_;
};

const RecordStart kRecordStart;

void PrintRUsage(std::ostream &out, const char *prefix, int who) {
  struct rusage usage;
  if (getrusage(who, &usage)) {
    perror("getrusage");
    return;
  }
  out << prefix << "RSSMax:" << usage.ru_maxrss << " kB" << '\t';
  out << prefix << "user:" << DoubleSec(usage.ru_utime) << '\t';
  out << prefix << "sys:" << DoubleSec(usage.ru_stime) << '\t';

  double cpu;
  if (who == RUSAGE_SELF) {
    struct timespec process_time;
    if (-1 == clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &process_time))
      err(1, "Failed to read CLOCK_PROCESS_CPUTIME_ID");
    cpu = DoubleSec(process_time);
  } else {
    cpu = DoubleSec(usage.ru_utime) + DoubleSec(usage.ru_stime);
  }
  out << prefix << "CPU:" << cpu << '\t';
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

  PrintRUsage(out, "", RUSAGE_SELF);
  PrintRUsage(out, "Child", RUSAGE_CHILDREN);

  struct timespec current;
  if (-1 == clock_gettime(CLOCK_MONOTONIC, &current))
    err(1, "Failed to read CLOCK_MONOTONIC");
  out << "real:" << Subtract(current, kRecordStart.Started()) << '\n';
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
      // Some processes close stderr.
      fd_ = dup(2);
    }

    ~Global() {
      std::stringstream buf;
      PrintUsage(buf);
      if (fd_ != -1) {
        FDWrite(fd_, buf.str());
        // I'm supposed to check the return value, but there's no stderr to print to!
        (void)close(fd_);
      }
    }

  private:
    int fd_;
};
const Global kGlobal;
} // namespace

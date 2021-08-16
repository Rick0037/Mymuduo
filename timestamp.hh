#ifndef MY_TIMESTAMP
#define MY_TIMESTAMP

#include <inttypes.h>

#include <chrono>
#include <cstdio>
#include <string>
class Timestamp {
  public:
    using clock = std::chrono::steady_clock;
    using timestamp = std::chrono::time_point<clock>;
    using duration = timestamp::duration;
    Timestamp() = default;
    ~Timestamp() = default;
    Timestamp(const timestamp& ts) : m_ts(ts) {}
    bool is_valid()
    {
        return m_ts.time_since_epoch() > duration::zero();
    }
    std::chrono::seconds seconds_since_epoch() const  
    {
        return std::chrono::duration_cast<std::chrono::seconds>(m_ts.time_since_epoch());
    }
    std::chrono::microseconds microseconds_since_epoch() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(m_ts.time_since_epoch());
    }
    bool operator<(const Timestamp& r) const 
    {
        return this->microseconds_since_epoch() < r.microseconds_since_epoch();
    }

    bool operator>(const Timestamp& r) const 
    {
        return this->microseconds_since_epoch() > r.microseconds_since_epoch();
    }

    bool operator==(const Timestamp& r) const 
    {
        return this->microseconds_since_epoch() == r.microseconds_since_epoch();
    }

    bool operator!=(const Timestamp& r) const
    {
        return this->microseconds_since_epoch() != r.microseconds_since_epoch();
    }
    std::string to_string() const  
    {
        char buf[32] = {0};
        int64_t seconds = seconds_since_epoch().count();
        int64_t microseconds = microseconds_since_epoch().count();
        snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
        return std::string(buf);
    }

    static Timestamp now()
    {
        return clock::now();
    }
    static Timestamp now_after(double seconds)
    {
        return clock::now() + std::chrono::duration_cast<duration>(
                                  std::chrono::duration<double, std::chrono::seconds::period>(seconds));
    }

    static auto microseconds_from_now_to(const Timestamp& ts) -> std::chrono::microseconds {
        return ts.microseconds_since_epoch() - now().microseconds_since_epoch();
    }
  private:
    timestamp m_ts;

};

#endif
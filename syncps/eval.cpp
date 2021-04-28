#define BOOST_LOG_DYN_LINK 1

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

#include "syncps.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <string>

#include "log.hpp"

int averageTimeBetweenPublishesInMilliseconds = 5000;
int varianceInTimeBetweenPublishesInMilliseconds = 1000;

using namespace ndn::time_literals;

class Producer
{
public:
  /**
   * @brief Initialize producer and schedule updates
   *
   * Set IBF size as 80 expecting 80 updates to IBF in a sync cycle
   * Set syncInterestLifetime and syncReplyFreshness to 1.6 seconds
   * userPrefix is the default user prefix, no updates are published on it in this example
   */
  Producer(const std::string& userPrefix)
    : m_userPrefix(userPrefix)
    , m_scheduler(m_face.getIoService())
    , m_sync(std::make_shared<syncps::SyncPubsub>(m_face, "/ndn/svs", isExpired, filterPubs))
    , m_rng(ndn::random::getRandomNumberEngine())
    , m_sleepTime(averageTimeBetweenPublishesInMilliseconds - varianceInTimeBetweenPublishesInMilliseconds, averageTimeBetweenPublishesInMilliseconds + varianceInTimeBetweenPublishesInMilliseconds)
  {
    m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                         [this] { runIter(); });
    m_sync->subscribeTo(
      ndn::Name("/ndn/svs"),
      std::bind(&Producer::processSyncUpdate, this, _1)
    );
  }

  static inline const syncps::FilterPubsCb filterPubs =
    [](auto& pOurs, auto& pOthers) mutable {
      // Only reply if at least one of the pubs is ours. Order the
      // reply by ours/others then most recent first (to minimize latency).
      // Respond with as many pubs will fit in one Data.
      if (pOurs.empty()) {
        return pOurs;
      }
      const auto cmp = [](const auto p1, const auto p2) {
        return p1->getName()[-1].toTimestamp() >
               p2->getName()[-1].toTimestamp();
      };
      if (pOurs.size() > 1) {
        std::sort(pOurs.begin(), pOurs.end(), cmp);
      }
      std::sort(pOthers.begin(), pOthers.end(), cmp);
      for (auto& p : pOthers) {
        pOurs.push_back(p);
      }
      return pOurs;
    };

  static inline const syncps::IsExpiredCb isExpired =
    [](auto p) {
      auto dt = ndn::time::system_clock::now() - p.getName()[-1].toTimestamp();
      return dt >= syncps::maxPubLifetime+syncps::maxClockSkew || dt <= -syncps::maxClockSkew;
    };

  void
  run()
  {
    BOOST_LOG_TRIVIAL(info) << "NODE_INIT::" << m_userPrefix;
    m_face.processEvents();
  }

  void
  runIter()
  {
    if (start_time == 0) {
      start_time = static_cast<long int> (time(NULL));
    }

    long int curr_time = static_cast<long int> (time(NULL));

    if (curr_time - start_time <= 120) {
      curr_i++;
      std::ostringstream ss = std::ostringstream();
      ss << m_userPrefix << "::" << curr_i;
      std::string message = ss.str();
      publishMsg(message);
      BOOST_LOG_TRIVIAL(info) << "PUBL_MSG::" << m_userPrefix << "::" << message;
    }

    if (curr_time - start_time <= 120 + 30) {
      m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                           [this] { runIter(); });
      return;
    }

    m_sync.reset();
    m_face.shutdown();
  }

private:
  syncps::Publication buildCmd(const std::string& s, const std::string& a = "")
  {
    ndn::Name cmd;
    cmd.append(s)
       .append(m_userPrefix)
       .appendTimestamp();
    return syncps::Publication(cmd);
  }

  void
  publishMsg(std::string msg)
  {
    auto cmd(buildCmd("/ndn/svs", "test2"));
    cmd.setContent(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.size());
    m_sync->publish(std::move(cmd));
  }

  void
  processSyncUpdate(const syncps::Publication& publication)
  {
    size_t data_size = publication.getContent().value_size();
    std::string content_str((char *)publication.getContent().value(), data_size);

    BOOST_LOG_TRIVIAL(info) << "RECV_STATE::" << content_str;
  }

private:
  std::string m_userPrefix;
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  long int start_time = 0;
  int curr_i = 0;

  std::shared_ptr<syncps::SyncPubsub> m_sync;

  ndn::random::RandomNumberEngine& m_rng;
  std::uniform_int_distribution<> m_sleepTime;
};

int
main(int argc, char* argv[])
{
  if (argc != 4) {
    BOOST_LOG_TRIVIAL(error) << "WRONG_ARGS";
    exit(1);
  }

  averageTimeBetweenPublishesInMilliseconds = strtol(argv[3], NULL, 10);
  varianceInTimeBetweenPublishesInMilliseconds = averageTimeBetweenPublishesInMilliseconds / 5;

  initlogger(std::string(argv[2]));

  try {
    Producer producer(argv[1]);
    producer.run();
  }
  catch (const std::exception& e) {}
}


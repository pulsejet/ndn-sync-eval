/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  The University of Memphis
 *
 * This file is part of PSync.
 * See AUTHORS.md for complete list of PSync authors and contributors.
 *
 * PSync is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * PSync is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * PSync, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <PSync/full-producer.hpp>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>

#include <iostream>
#include <string>
#include <thread>

#include "log.hpp"

int averageTimeBetweenPublishesInMilliseconds = 5000;
int varianceInTimeBetweenPublishesInMilliseconds = 1000;

NDN_LOG_INIT(examples.FullSyncApp);

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
    , m_fullProducer(std::make_shared<psync::FullProducer>(
                      6, m_face, "/ndn/svs", userPrefix,
                      std::bind(&Producer::processSyncUpdate, this, _1),
                      1000_ms, 1000_ms))
    , m_rng(ndn::random::getRandomNumberEngine())
    , m_sleepTime(averageTimeBetweenPublishesInMilliseconds - varianceInTimeBetweenPublishesInMilliseconds, averageTimeBetweenPublishesInMilliseconds + varianceInTimeBetweenPublishesInMilliseconds)
  {
    ndn::Name prefix(userPrefix);
    m_fullProducer->addUserNode(prefix);

    m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                         [this] { runIter(); });
  }

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
      ss << m_userPrefix << "=" << curr_i;
      std::string message = ss.str();
      publishMsg(message);
      BOOST_LOG_TRIVIAL(info) << "PUBL_MSG::" << m_userPrefix << "::" << message;
    }

    if (curr_time - start_time <= 120 + 30) {
      m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                           [this] { runIter(); });
      return;
    }

    m_fullProducer.reset();
    m_face.shutdown();
  }

private:
  void
  publishMsg(std::string msg)
  {
    ndn::Name prefix(m_userPrefix);
    m_fullProducer->publishName(prefix);
  }

  void
  processSyncUpdate(const std::vector<psync::MissingDataInfo>& updates)
  {
    for (const auto& update : updates) {
      for (uint64_t i = update.lowSeq; i <= update.highSeq; i++) {
        BOOST_LOG_TRIVIAL(info) << "RECV_STATE::" << update.prefix << "::" << i;
        // BOOST_LOG_TRIVIAL(info) << "RECV_MSG::" << m_userPrefix << "::" << update.prefix << "=" << i;
      }
    }
  }

private:
  std::string m_userPrefix;
  ndn::Face m_face;
  ndn::Scheduler m_scheduler;

  long int start_time = 0;
  int curr_i = 0;

  std::shared_ptr<psync::FullProducer> m_fullProducer;

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
  catch (const std::exception& e) {
    NDN_LOG_ERROR(e.what());
  }
}

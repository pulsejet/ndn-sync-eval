/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2021 University of California, Los Angeles
 *
 * This file is part of ndn-svs, synchronization library for distributed realtime
 * applications for NDN.
 *
 * ndn-svs library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, in version 2.1 of the License.
 *
 * ndn-svs library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 */
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <stdlib.h>

#include <ndn-svs/svsync-base.hpp>

#include "log.hpp"

int m_stateVectorLogIntervalInMilliseconds = 1000;

class Options
{
public:
  Options() {}

public:
  std::string prefix;
  std::string m_id;
};

class Program
{
public:
  Program(const Options &options)
    : m_options(options)
    , m_scheduler(face.getIoService())
    , m_rng(ndn::random::getRandomNumberEngine())
    , m_sleepTime(averageTimeBetweenPublishesInMilliseconds - varianceInTimeBetweenPublishesInMilliseconds, averageTimeBetweenPublishesInMilliseconds + varianceInTimeBetweenPublishesInMilliseconds)
  {
    m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                         [this] { runIter(); });
  }

  void
  run()
  {
    BOOST_LOG_TRIVIAL(info) << "NODE_INIT::" << m_options.m_id;
    face.processEvents();
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
      ss << m_options.m_id << "=" << curr_i;
      std::string message = ss.str();
      publishMsg(message);
      BOOST_LOG_TRIVIAL(info) << "PUBL_MSG::" << m_options.m_id << "::" << message;
    }

    if (curr_time - start_time <= 120 + 30) {
      m_scheduler.schedule(ndn::time::milliseconds(m_sleepTime(m_rng)),
                           [this] { runIter(); });
      return;
    }

    m_svs.reset();
    face.shutdown();
  }

protected:
  void
  onMissingData(const std::vector<ndn::svs::MissingDataInfo>& v)
  {
    for (size_t i = 0; i < v.size(); i++)
    {
      for (ndn::svs::SeqNo s = v[i].low; s <= v[i].high; ++s)
      {
        ndn::svs::NodeID nid = v[i].session;
        BOOST_LOG_TRIVIAL(info) << "RECV_STATE::" << nid << "::" << s;
        m_svs->fetchData(nid, s, [&] (const ndn::Data& data)
          {
            size_t data_size = data.getContent().value_size();
            std::string content_str((char *)data.getContent().value(), data_size);
            BOOST_LOG_TRIVIAL(info) << "RECV_MSG::" << m_options.m_id << "::" << content_str;
          }, 5);
      }
    }
  }

  void
  publishMsg(std::string msg)
  {
    m_svs->publishData(reinterpret_cast<const uint8_t*>(msg.c_str()),
                       msg.size(),
                       ndn::time::milliseconds(1000));
  }

public:
  const Options m_options;
  ndn::Face face;
  std::shared_ptr<ndn::svs::SVSyncBase> m_svs;
  ndn::Scheduler m_scheduler;

  ndn::random::RandomNumberEngine& m_rng;
  std::uniform_int_distribution<> m_sleepTime;

  long int start_time = 0;
  int curr_i = 0;
};

template <typename T>
int
callMain(int argc, char **argv) {
  if (argc != 4) {
    BOOST_LOG_TRIVIAL(error) << "WRONG_ARGS";
    exit(1);
  }

  averageTimeBetweenPublishesInMilliseconds = strtol(argv[3], NULL, 10);
  varianceInTimeBetweenPublishesInMilliseconds = averageTimeBetweenPublishesInMilliseconds / 5;

  Options opt;
  opt.prefix = "/ndn/svs";
  opt.m_id = argv[1];

  initlogger(std::string(argv[2]));

  T program(opt);
  program.run();
  return 0;
}

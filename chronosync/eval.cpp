#if 1
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

int averageTimeBetweenPublishesInMilliseconds = 5000;
int varianceInTimeBetweenPublishesInMilliseconds = 1000;

#include "chat.hpp"

using namespace ndn::time_literals;

class ProgramPrefix : public Program
{
public:
  ProgramPrefix(const Options &options) : Program(options)
  {
    m_cs = std::make_shared<chronosync::Socket>(
      ndn::Name(m_options.prefix),
      ndn::Name(m_options.m_id),
      face,
      std::bind(&ProgramPrefix::onMissingData, this, _1),
      chronosync::Socket::DEFAULT_NAME,
      chronosync::Socket::DEFAULT_VALIDATOR,
      1000_ms);
  }
};

int
main(int argc, char **argv)
{
  return callMain<ProgramPrefix>(argc, argv);
}
#else
int
main(int argc, char **argv)
{
  return 0;
}
#endif
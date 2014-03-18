/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "status-server.hpp"
#include "fw/forwarder.hpp"
#include "core/version.hpp"

namespace nfd {

const Name StatusServer::DATASET_PREFIX = "ndn:/localhost/nfd/status";
const time::milliseconds StatusServer::RESPONSE_FRESHNESS = time::milliseconds(5000);

StatusServer::StatusServer(shared_ptr<AppFace> face, Forwarder& forwarder)
  : m_face(face)
  , m_forwarder(forwarder)
  , m_startTimestamp(time::system_clock::now())
{
  m_face->setInterestFilter(DATASET_PREFIX, bind(&StatusServer::onInterest, this, _2));
}

void
StatusServer::onInterest(const Interest& interest) const
{
  Name name(DATASET_PREFIX);
  /// \todo use NumberComponent
  name.append(Name::Component::fromNumberWithMarker(
    time::toUnixTimestamp(time::system_clock::now()).count(), 0x00));
  name.append(Name::Component::fromNumberWithMarker(0, 0x00));

  shared_ptr<Data> data = make_shared<Data>(name);
  data->setFreshnessPeriod(RESPONSE_FRESHNESS);

  shared_ptr<ndn::nfd::Status> payload = this->collectStatus();
  ndn::EncodingBuffer payloadBuffer;
  payload->wireEncode(payloadBuffer);
  data->setContent(payloadBuffer.buf(), payloadBuffer.size());

  m_face->sign(*data);
  m_face->put(*data);
}

shared_ptr<ndn::nfd::Status>
StatusServer::collectStatus() const
{
  shared_ptr<ndn::nfd::Status> status = make_shared<ndn::nfd::Status>();

  status->setNfdVersion(NFD_VERSION);
  status->setStartTimestamp(m_startTimestamp);
  status->setCurrentTimestamp(time::system_clock::now());

  status->setNNameTreeEntries(m_forwarder.getNameTree().size());
  status->setNFibEntries(m_forwarder.getFib().size());
  status->setNPitEntries(m_forwarder.getPit().size());
  status->setNMeasurementsEntries(m_forwarder.getMeasurements().size());
  status->setNCsEntries(m_forwarder.getCs().size());

  const ForwarderCounters& counters = m_forwarder.getCounters();
  status->setNInInterests(counters.getInInterest());
  status->setNOutInterests(counters.getOutInterest());
  status->setNInDatas(counters.getInData());
  status->setNOutDatas(counters.getOutData());

  return status;
}

} // namespace nfd

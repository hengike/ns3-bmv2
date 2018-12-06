/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Stanford University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stephen Ibanez <sibanez@stanford.edu>
 */

#ifndef P4_QUEUE_DISC_H
#define P4_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/p4-pipeline.h"
#include <array>
#include <string>

namespace ns3 {


/**
 * \ingroup traffic-control
 *
 * The P4 qdisc is configured by a P4 program. It contains qdisc classes
 * which actually perform the queueing and scheduling. This qdisc is
 * intended to be the root qdisc that simply runs the user's P4 program
 * and then passes the modified packet to the appropriate qdisc class
 * (or drops the packet if the P4 program says to do so).
 */
class P4QueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief P4QueueDisc constructor
   */
  P4QueueDisc ();

  virtual ~P4QueueDisc();

  /// Get the JSON source file
  std::string GetJsonFile (void) const;

  /// Set the JSON source file
  void SetJsonFile (std::string jsonFile);

  /// Get the CLI commands file 
  std::string GetCommandsFile (void) const;

  /// Set the CLI commands file
  void SetCommandsFile (std::string commandsFile);

  static constexpr const char* P4_DROP = "P4 drop";      //!< P4 program said to drop packet before enqueue

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);

  /**
   * \brief The function to execute when a timer event is triggered
   */
  void RunTimerEvent (void);

  /**
   * \brief Map a double in the range [0, GetMaxSize()] to an integer
   *  in the range [0, 2^m_qSizeBits - 1].
   *  \param size a numeric (double) size value to convert
   */
  uint32_t MapSize (double size);

  /**
   * \brief Initialize the queue disc parameters.
   *
   * Note: if the link bandwidth changes in the course of the
   * simulation, the bandwidth-dependent parameters do not change.
   * This should be fixed, but it would require some extra parameters,
   * and didn't seem worth the trouble...
   */
  virtual void InitializeParams (void);
  /**
   * \brief Compute the average queue size
   * \param nQueued number of queued packets
   * \param m simulated number of packets arrival during idle period
   * \param qAvg average queue size
   * \param qW queue weight given to cur q size sample
   * \returns new average queue size
   */
  double Estimator (uint32_t nQueued, uint32_t m, double qAvg, double qW);

  static const uint64_t DQCOUNT_INVALID = std::numeric_limits<uint64_t>::max(); //!< Invalid dqCount value

  // ** Variables supplied by user 
  std::string m_jsonFile;      //!< The bmv2 JSON file (generated by the p4c-bm backend)
  std::string m_commandsFile;  //!< The CLI commands file
  uint32_t m_qSizeBits;        //!< Number of bits to use to represent range of values for queue/pkt size (up to 32 bits)
  uint32_t m_meanPktSize;      //!< Avg pkt size
  Time m_linkDelay;            //!< Link delay
  DataRate m_linkBandwidth;    //!< Link bandwidth
  double m_qW;                 //!< Queue weight given to cur queue size sample
  uint32_t m_dqThreshold;      //!< Minimum queue size in bytes before dequeue rate is measured
  Time m_timeReference;        //!< Desired time between timer event triggers

  // ** Variables maintained by the queue disc
  SimpleP4Pipe *m_p4Pipe;            //!< The P4 pipeline
  uint32_t m_idle;                   //!< 0/1 idle status
  double m_ptc;                      //!< packet time constant in packets/second
  Time m_idleTime;                   //!< Start of current idle period
  TracedValue<double> m_qAvg;        //!< Average queue length
  TracedValue<double> m_avgDqRate;   //!< Time averaged dequeue rate
  double m_dqStart;                  //!< Start timestamp of current measurement cycle
  uint64_t m_dqCount;                //!< Number of bytes departed since current measurement cycle starts
  bool m_inMeasurement;              //!< Indicates whether we are in a measurement cycle
  TracedValue<int64_t> m_qLatency;   //!< Instantaneous queue latency (ns)
  EventId m_timerEvent;              //!< The timer event ID
  static Ptr<Packet> default_packet; //!< default packet to use for timer events

  TracedValue<uint32_t> m_p4Var1; //!< 1st traced P4 variable
  TracedValue<uint32_t> m_p4Var2; //!< 2nd traced P4 variable
  TracedValue<uint32_t> m_p4Var3; //!< 3rd traced P4 variable
  TracedValue<uint32_t> m_p4Var4; //!< 4th traced P4 variable

};

} // namespace ns3

#endif /* P4_QUEUE_DISC_H */

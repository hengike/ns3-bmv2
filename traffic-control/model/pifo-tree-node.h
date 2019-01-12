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
 * Author:  Stpehen Ibanez <sibanez@stanford.edu>
 */

#ifndef PIFO_TREE_NODE_H
#define PIFO_TREE_NODE_H

#include "ns3/queue-disc.h"
#include "ns3/enq-pipeline.h"
#include "ns3/deq-pipeline.h"

namespace ns3 {

/**
 * \brief The functor used to compare PIFO entries based on priority.
 */
template<typename T>
struct PifoEntryComp {
  bool operator()(const T &lhs, const T &rhs) const {
    return lhs.GetPriority() >= rhs.GetPriority();
  }
};

/**
 * \brief Template Pifo implementation
 */
// TODO(sibanez): need to add trace sources to this to track occupancy?
// TODO(sibanez): use std::vector or std::deque?
template<typename T,
         typename Sequence = std::vector<T>,
         typename Compare = PifoEntryComp<T> >
class Pifo : public std::priority_queue<T,Sequence,Compare> {

public:
  Pifo() : m_lastPopTime(0) {}

  void
  dequeue ()
    {
      pop(); // invoke priority_queue pop() method
      m_lastPopTime = Simulator::Now ().GetNanoSeconds ();
    }

  /// Get the last time the dequeue() method was invoked
  int64_t
  lastPopTime ()
    {
      return m_lastPopTime;
    }

private:
  int64_t m_lastPopTime;
}

/**
 * \ingroup traffic-control
 *
 * The object stored within a PIFO.
 */
class PifoEntry {
public:
  /**
   * \brief PifoEntry constructor
   *
   */
  PifoEntry ();

  /**
   * \brief Get the size of this entry (i.e. pkt_len) 
   */
  uint32_t GetSize (void);

  /**
   * \brief Get the priority of this entry (i.e. rank)
   */
  uint32_t GetPriority (void);

  /// The queue disc item, which is only valid for PIFOs in a leaf node
  Ptr<QueueDiscItem> item;
  /// node_id of the child node (index into a node's m_children vector), only valid in non-leaf nodes
  uint8_t node_id;
  /// pifo_id within the child node (index into the child node's m_pifos vector), only valid in non-leaf nodes
  uint8_t pifo_id;
  /// rank of this entry
  uint32_t rank;
  /// time at which this entry can be transmitted
  int64_t tx_time;
  /// time at which this entry can be transmitted (relative to previous pkt in PIFO)
  uint32_t tx_delta;
  /// length of the corresponding packet
  uint32_t pkt_len;
};

/**
 * \ingroup traffic-control
 *
 * The node object used in a PIFO tree queue disc.
 */
class PifoTreeNode : public Object {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief PifoTreeNode constructor
   *
   */
  PifoTreeNode ();

  virtual ~PifoTreeNode();

  /**
   * \brief Initialize the enqueue logic for this node
   */
  bool AddEnqLogic (std::string enqJson, std::string enqCommands);

  /**
   * \brief Initialize the dequeue logic for this node
   */
  bool AddDeqLogic (std::string deqJson, std::string deqCommands);

  /**
   * \brief Add the specified number of PIFOs to this node
   */
  bool AddPifos (int numPifos);

  /**
   * \brief Set the parent of this node to be the speicifed node
   */
  bool AddParent (Ptr<PifoTreeNode> parent);

  /**
   * \brief Add the specified node as a child for this node
   */
  bool AddChild (Ptr<PifoTreeNode> child);

  /**
   * \brief Check the configuration of this node
   */
  bool CheckConfig (void);

  /**
   * \brief Lookup the local ID of the node given the global node ID
   */
  uint8_t GetLocalNodeID (uint32_t global_node_id);

  /**
   * \brief Initialize the enqueue metadata
   */
  void InitEnqMeta (std_enq_meta_t& std_enq_meta);

  /**
   * \brief Initialize the dequeue metadata
   */
  void InitDeqMeta (std_deq_meta_t& std_deq_meta);

  /**
   * \brief Enqueue an entry into the specified PIFO. Used for leaf nodes.
   * \param item the queue disc item to enqueue
   * \param sched_meta various metadata fields used for the enqueue logic
   * \returns bool indicating enqueue was successful
   */
  bool Enqueue (Ptr<QueueDiscItem> item, sched_meta_t sched_meta);

  /**
   * \brief Enqueue an entry into the specified PIFO. This is the enqueue method used for non-leaf nodes.
   * \param child_node_gid the global ID of the child node, used to lookup the local ID during enqueue 
   * \param child_pifo_id the ID of the PIFO within the child node (index into the child node's m_pifos vector)
   * \param sched_meta various metadata fields used for the enqueue logic
   * \returns bool indicating enqueue was successful
   */
  bool Enqueue (uint32_t child_node_gid, uint8_t child_pifo_id, sched_meta_t sched_meta);

  /**
   * \brief Perform the next enqueue operation.
   * \param enq_delay how long to wait before attempting to perform the next enqueue operation
   * \param pifo_id local ID of the pifo that was just enqueued into
   * \param sched_meta scheduling metadata to use for next enqueue operation
   */
  bool EnqueueNext (uint32_t enq_delay, uint8_t pifo_id, sched_meta_t sched_meta);

  /**
   * \brief Dequeue from this node. Use dequeue logic to decide which PIFO to
   *   remove from. Mainly used for the root node.
   * \returns Pointer to queue disc item
   */
  Ptr<QueueDiscItem> Dequeue (void);

  /**
   * \brief Dequeue the head element from the specified pifo. The difference b/w this method and the
   *   DequeuePifo method is that if pifo_id is invalid then it will invoke Dequeue ()
   * \param pifo_id the ID of the PIFO to remove from
   * \returns Pointer to queue disc item
   */
  Ptr<QueueDiscItem> Dequeue (uint8_t pifo_id);

  /**
   * \brief Dequeue the head element from the specified pifo.
   * \param pifo_id the ID of the PIFO to remove from
   * \returns Pointer to queue disc item
   */
  Ptr<QueueDiscItem> DequeuePifo (uint8_t pifo_id);

private:
  EnqP4Pipe* m_enqPipe;
  DeqP4Pipe* m_deqPipe;

  static uint32_t nodeID;

  uint32_t m_globalID;
  bool m_isLeaf;
  Ptr<PifoTreeNode> m_parent;
  std::vector<Ptr<PifoTreeNode>> m_children;
  std::vector<Pifo<PifoEntry>> m_pifos;

  /// map global node IDs to local node IDs
  std::map<uint32_t, uint8_t> m_global2Local; 

  // P4 trace vars
  TracedValue<uint32_t> m_enqP4Var1;
  TracedValue<uint32_t> m_enqP4Var2;
  TracedValue<uint32_t> m_enqP4Var3;
  TracedValue<uint32_t> m_enqP4Var4;
  TracedValue<uint32_t> m_deqP4Var1;
  TracedValue<uint32_t> m_deqP4Var2;
  TracedValue<uint32_t> m_deqP4Var3;
  TracedValue<uint32_t> m_deqP4Var4;

  // Statistics
  TracedValue<uint32_t> m_nPackets;

};

} // namespace ns3

#endif /* PIFO_TREE_NODE_H */
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#include "prio-queue-disc-manip.h"

#include <algorithm>
#include <iterator>

#include "ns3/log.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/socket.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("PrioQueueDiscManip");

NS_OBJECT_ENSURE_REGISTERED(PrioQueueDiscManip);

ATTRIBUTE_HELPER_CPP(Priomap);

std::ostream &operator<<(std::ostream &os, const Priomap &priomap) {
    std::copy(priomap.begin(), priomap.end() - 1,
              std::ostream_iterator<uint16_t>(os, " "));
    os << priomap.back();
    return os;
}

std::istream &operator>>(std::istream &is, Priomap &priomap) {
    for (int i = 0; i < 16; i++) {
        if (!(is >> priomap[i])) {
            NS_FATAL_ERROR("Incomplete priomap specification ("
                           << i << " values provided, 16 required)");
        }
    }
    return is;
}

TypeId PrioQueueDiscManip::GetTypeId(void) {
    static TypeId tid =
        TypeId("ns3::PrioQueueDiscManip")
            .SetParent<QueueDisc>()
            .SetGroupName("TrafficControl")
            .AddConstructor<PrioQueueDiscManip>()
            .AddAttribute(
                "Priomap", "The priority to band mapping.",
                PriomapValue(Priomap{
                    {0, 1, 2, 2, 1, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}
    }),
                MakePriomapAccessor(&PrioQueueDiscManip::m_prio2band),
                MakePriomapChecker());
    return tid;
}

PrioQueueDiscManip::PrioQueueDiscManip()
    : QueueDisc(QueueDiscSizePolicy::MULTIPLE_QUEUES) {
    NS_LOG_FUNCTION(this);
}

PrioQueueDiscManip::~PrioQueueDiscManip() { NS_LOG_FUNCTION(this); }

void PrioQueueDiscManip::SetBandForPriority(uint8_t prio, uint16_t band) {
    NS_LOG_FUNCTION(this << prio << band);

    NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");

    m_prio2band[prio] = band;
}

uint16_t PrioQueueDiscManip::GetBandForPriority(uint8_t prio) const {
    NS_LOG_FUNCTION(this << prio);

    NS_ASSERT_MSG(prio < 16, "Priority must be a value between 0 and 15");

    return m_prio2band[prio];
}

bool PrioQueueDiscManip::DoEnqueue(Ptr<QueueDiscItem> item) {
    NS_LOG_FUNCTION(this << item);

    uint32_t band = m_prio2band[0];

    int32_t ret = Classify(item);

    if (ret == PacketFilter::PF_NO_MATCH) {
        NS_LOG_DEBUG(
            "No filter has been able to classify this packet, using priomap.");

        SocketPriorityTag priorityTag;
        if (item->GetPacket()->PeekPacketTag(priorityTag)) {
            band = m_prio2band[priorityTag.GetPriority() & 0x0f];
        }
    } else {
        NS_LOG_DEBUG("Packet filters returned " << ret);

        if (ret >= 0 && static_cast<uint32_t>(ret) < GetNQueueDiscClasses()) {
            band = ret;
        }
    }

    NS_ASSERT_MSG(band < GetNQueueDiscClasses(), "Selected band out of range");
    bool retval = GetQueueDiscClass(band)->GetQueueDisc()->Enqueue(item);

    // If Queue::Enqueue fails, QueueDisc::Drop is called by the child queue
    // disc because QueueDisc::AddQueueDiscClass sets the drop callback

    NS_LOG_LOGIC("Number packets band "
                 << band << ": "
                 << GetQueueDiscClass(band)->GetQueueDisc()->GetNPackets());

    return retval;
}

Ptr<QueueDiscItem> PrioQueueDiscManip::DoDequeue(void) {
    NS_LOG_FUNCTION(this);

    Ptr<QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++) {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Dequeue()) != 0) {
            NS_LOG_LOGIC("Popped from band " << i << ": " << item);
            NS_LOG_LOGIC(
                "Number packets band "
                << i << ": "
                << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

Ptr<const QueueDiscItem> PrioQueueDiscManip::DoPeek(void) {
    NS_LOG_FUNCTION(this);

    Ptr<const QueueDiscItem> item;

    for (uint32_t i = 0; i < GetNQueueDiscClasses(); i++) {
        if ((item = GetQueueDiscClass(i)->GetQueueDisc()->Peek()) != 0) {
            NS_LOG_LOGIC("Peeked from band " << i << ": " << item);
            NS_LOG_LOGIC(
                "Number packets band "
                << i << ": "
                << GetQueueDiscClass(i)->GetQueueDisc()->GetNPackets());
            return item;
        }
    }

    NS_LOG_LOGIC("Queue empty");
    return item;
}

bool PrioQueueDiscManip::CheckConfig(void) {
    NS_LOG_FUNCTION(this);
    if (GetNInternalQueues() > 0) {
        NS_LOG_ERROR("PrioQueueDiscManip cannot have internal queues");
        return false;
    }

    if (GetNQueueDiscClasses() == 0) {
        // create 3 fifo queue discs
        ObjectFactory factory;
        factory.SetTypeId("ns3::FifoQueueDisc");
        for (uint8_t i = 0; i < 2; i++) {
            Ptr<QueueDisc> qd = factory.Create<QueueDisc>();
            qd->SetMaxSize(this->GetMaxSize());
            qd->Initialize();
            Ptr<QueueDiscClass> c = CreateObject<QueueDiscClass>();
            c->SetQueueDisc(qd);
            AddQueueDiscClass(c);
        }
    }

    if (GetNQueueDiscClasses() < 2) {
        NS_LOG_ERROR("PrioQueueDiscManip needs at least 2 classes");
        return false;
    }

    return true;
}

void PrioQueueDiscManip::InitializeParams(void) { NS_LOG_FUNCTION(this); }

}  // namespace ns3

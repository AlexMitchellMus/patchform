#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>
#include "pffft.h"
#include <cmath>
#include <algorithm>

class BinCombine final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("BinCombine", "bincombine", true);

public:
    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    BinCombine(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams) {
        addInputPort("a", AudioPort::PortType::Data);
        addInputPort("b", AudioPort::PortType::Data);
    }

    ~BinCombine() override {
        if (savedA) context->makeDataPersistent(savedA, false, nodeID);
        if (savedB) context->makeDataPersistent(savedB, false, nodeID);
    }

    void processAudio(const float*, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const auto& aEvents = inputPortBuffers[0]->getEvents();
        const auto& bEvents = inputPortBuffers[1]->getEvents();

        for (const auto* ev : aEvents) {
            if (ev->getTagHash() == hash("complexbins") && ev->data != savedA) {
                if (savedA)
                    context->makeDataPersistent(savedA, false, nodeID);
                savedA = ev->data;
                context->makeDataPersistent(savedA, true, nodeID);
            }
        }

        for (const auto* ev : bEvents) {
            if (ev->getTagHash() == hash("complexbins") && ev->data != savedB) {
                if (savedB)
                    context->makeDataPersistent(savedB, false, nodeID);
                savedB = ev->data;
                context->makeDataPersistent(savedB, true, nodeID);
            }
        }

        if (!savedA || !savedB) return;

        DataAtom* head = nullptr;
        DataAtom* tail = nullptr;

        const DataAtom* a = savedA;
        const DataAtom* b = savedB;

        for (size_t i = 0; i < FREQ_BINS; ++i) {
            const auto* aList = a ? a->data.list : nullptr;
            const auto* bList = b ? b->data.list : nullptr;

            float reA = (aList && aList->type == DataAtom::DataType::Float) ? aList->data.atom : 0.0f;
            float imA = (aList && aList->next && aList->next->type == DataAtom::DataType::Float) ? aList->next->data.atom : 0.0f;

            float reB = (bList && bList->type == DataAtom::DataType::Float) ? bList->data.atom : 0.0f;
            float imB = (bList && bList->next && bList->next->type == DataAtom::DataType::Float) ? bList->next->data.atom : 0.0f;

            float re = 0.5f * (reA + reB);
            float im = 0.5f * (imA + imB);

            DataAtom* outer = context->eventPool.allocateDataAtom();
            outer->type = DataAtom::DataType::List;

            DataAtom* realAtom = context->eventPool.allocateDataAtom();
            realAtom->type = DataAtom::DataType::Float;
            realAtom->data.atom = re;

            DataAtom* imagAtom = context->eventPool.allocateDataAtom();
            imagAtom->type = DataAtom::DataType::Float;
            imagAtom->data.atom = im;
            realAtom->next = imagAtom;
            imagAtom->next = nullptr;

            outer->data.list = realAtom;
            outer->next = nullptr;

            if (!head) {
                head = tail = outer;
            } else {
                tail->next = outer;
                tail = outer;
            }

            if (a) a = a->next;
            if (b) b = b->next;
        }

        auto* ev = context->eventPool.getFreeEvent();
        ev->data = head;
        ev->numAtoms = FREQ_BINS;
        ev->setTagHashcode(hash("complexbins"));
        outputPortBuffers[0]->addEvent(ev);
    }

private:
    DataAtom* savedA = nullptr;
    DataAtom* savedB = nullptr;
};

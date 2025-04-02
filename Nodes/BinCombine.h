#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>
#include <cmath>
#include <algorithm>

class BinCombine final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("BinCombine", "bincombine", true);

public:
    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    BinCombine(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("a", AudioPort::PortType::Data); // primary input
        addInputPort("b", AudioPort::PortType::Data); // character input

        modeParam = addParameter<StringParameter>("mode", objParams.value("mode", "formant"));
        modeHash.store(hash(modeParam->getValue()));

        modeParam->informNodeOfChange = [this]() {
            modeHash.store(hash(modeParam->getValue()));
        };
    }

    void processAudio(const float*, float*, const unsigned long, std::vector<MidiMessage>&) override {
        const auto& eventsA = inputPortBuffers[0]->getEvents();
        const auto& eventsB = inputPortBuffers[1]->getEvents();

        // Clear previous bins - this is important!
        std::fill(binReal.begin(), binReal.end(), 0.0f);
        std::fill(binImag.begin(), binImag.end(), 0.0f);
        std::fill(binRealFromB.begin(), binRealFromB.end(), 0.0f);
        std::fill(binImagFromB.begin(), binImagFromB.end(), 0.0f);

        // Capture bins from input A
        bool hasA = false;
        for (const auto* ev : eventsA) {
            if (ev->getTagHash() != hash("complexbins") || ev->numAtoms < FREQ_BINS)
                continue;
            for (size_t i = 0; i < FREQ_BINS; ++i) {
                const DataAtom* outer = ev->getAtom(i);
                if (!outer || outer->type != DataAtom::DataType::List || !outer->data.list) continue;
                const DataAtom* realAtom = outer->data.list;
                const DataAtom* imagAtom = realAtom ? realAtom->next : nullptr;
                if (!realAtom || !imagAtom) continue;
                binReal[i] = realAtom->data.atom;
                binImag[i] = imagAtom->data.atom;
            }
            hasA = true;
            break;
        }

        // Capture bins from input B
        bool hasB = false;
        for (const auto* ev : eventsB) {
            if (ev->getTagHash() != hash("complexbins") || ev->numAtoms < FREQ_BINS)
                continue;
            for (size_t i = 0; i < FREQ_BINS; ++i) {
                const DataAtom* outer = ev->getAtom(i);
                if (!outer || outer->type != DataAtom::DataType::List || !outer->data.list) continue;
                const DataAtom* realAtom = outer->data.list;
                const DataAtom* imagAtom = realAtom ? realAtom->next : nullptr;
                if (!realAtom || !imagAtom) continue;
                binRealFromB[i] = realAtom->data.atom;
                binImagFromB[i] = imagAtom->data.atom;
            }
            hasB = true;
            break;
        }

        // If we don't have valid data, don't output anything
        if (!hasA || !hasB) return;

        auto* outEvent = context->eventPool.getFreeEvent();
        if (!outEvent) return;

        DataAtom* head = nullptr;
        DataAtom* tail = nullptr;

        auto m = modeHash.load();

        // Pre-analyze B's spectrum for spectral envelope
        std::array<float, FREQ_BINS> bMagnitudes{};
        for (size_t i = 0; i < FREQ_BINS; ++i) {
            bMagnitudes[i] = std::sqrt(binRealFromB[i] * binRealFromB[i] + binImagFromB[i] * binImagFromB[i]);
        }

        // Find B's peak frequencies (up to 5)
        struct Peak { size_t bin; float magnitude; };
        std::array<Peak, 5> peaks{};
        for (size_t i = 1; i < FREQ_BINS - 1; ++i) {
            if (bMagnitudes[i] > bMagnitudes[i-1] && bMagnitudes[i] > bMagnitudes[i+1]) {
                // Found a local peak
                for (size_t p = 0; p < peaks.size(); ++p) {
                    if (bMagnitudes[i] > peaks[p].magnitude) {
                        // Shift existing peaks down
                        for (size_t j = peaks.size() - 1; j > p; --j) {
                            peaks[j] = peaks[j-1];
                        }
                        peaks[p] = {i, bMagnitudes[i]};
                        break;
                    }
                }
            }
        }

        // Process the spectral data according to current mode
        for (size_t i = 0; i < FREQ_BINS; ++i) {
            float re = 0.0f, im = 0.0f;

            // Input A (primary)
            float rA = binReal[i];
            float iA = binImag[i];

            // Input B (character)
            float rB = binRealFromB[i];
            float iB = binImagFromB[i];

            // Get magnitudes and phases
            float magA = std::sqrt(rA * rA + iA * iA);
            float phaseA = std::atan2(iA, rA);

            float magB = std::sqrt(rB * rB + iB * iB);
            float phaseB = std::atan2(iB, rB);

            // Apply different processing modes
            switch (m) {
            case hash("formant"):
                // EXTREMELY DIFFERENT MODE 1: Formant transfer
                {
                    // Calculate smoothed spectral envelope from B
                    float smoothEnvB = 0.0f;
                    float totalWeight = 0.0f;

                    // Use Gaussian-like weighting for smoother envelope
                    for (size_t j = 0; j < FREQ_BINS; ++j) {
                        float distance = std::abs((float)i - (float)j);
                        float weight = std::exp(-distance * distance / (2.0f * 100.0f)); // sigma = 10
                        smoothEnvB += bMagnitudes[j] * weight;
                        totalWeight += weight;
                    }
                    smoothEnvB /= totalWeight;

                    // Apply strong formant shaping
                    float newMag = magA * (1.0f + smoothEnvB * 8.0f);

                    // Keep A's phase
                    re = newMag * std::cos(phaseA);
                    im = newMag * std::sin(phaseA);
                }
                break;

            case hash("resonator"):
                // EXTREMELY DIFFERENT MODE 2: Selective resonance at B's peak frequencies
                {
                    // Default to original A
                    float newMag = magA;
                    float newPhase = phaseA;

                    // Check if this bin is near any of B's peaks
                    for (const auto& peak : peaks) {
                        if (peak.magnitude < 0.01f) continue; // Skip unused peak slots

                        float distance = std::abs((float)i - (float)peak.bin);
                        if (distance < 20.0f) { // Wider resonance bands
                            // Create resonance with strong boost and phase alignment
                            float resonanceStrength = std::exp(-distance * distance / (2.0f * 25.0f)); // sigma = 5
                            newMag = magA * (1.0f + 15.0f * resonanceStrength);

                            // Slightly pull phase toward B's phase at resonant points
                            float phasePull = 0.3f * resonanceStrength;
                            newPhase = phaseA * (1.0f - phasePull) + phaseB * phasePull;
                            break; // Only apply the strongest resonance
                        }
                    }

                    // Output with resonance applied
                    re = newMag * std::cos(newPhase);
                    im = newMag * std::sin(newPhase);
                }
                break;
            case hash("vocoder"):
                {
                    static constexpr size_t NUM_BANDS = 20;
                    static constexpr size_t BINS_PER_BAND = FREQ_BINS / NUM_BANDS;

                    std::array<float, NUM_BANDS> bandEnvB{};

                    // Step 1: Compute average envelope per band from B (modulator)
                    for (size_t band = 0; band < NUM_BANDS; ++band) {
                        float sum = 0.0f;
                        size_t start = band * BINS_PER_BAND;
                        size_t end = (band + 1) * BINS_PER_BAND;
                        for (size_t j = start; j < end; ++j) {
                            float mag = std::sqrt(binRealFromB[j] * binRealFromB[j] + binImagFromB[j] * binImagFromB[j]);
                            sum += mag;
                        }
                        bandEnvB[band] = sum / static_cast<float>(BINS_PER_BAND);
                    }

                    // Step 2: Apply modulator's envelope to carrier's phase
                    size_t bandIndex = i / BINS_PER_BAND;
                    if (bandIndex >= NUM_BANDS) bandIndex = NUM_BANDS - 1;

                    float env = bandEnvB[bandIndex];

                    // Classic vocoder: magnitude from B, phase from A
                    re = env * std::cos(phaseA);
                    im = env * std::sin(phaseA);

                    // Optional: enhance articulation by emphasizing mid bands
                    if (bandIndex < 2 || bandIndex > NUM_BANDS - 2) {
                        // Attenuate very low/high bands
                        float factor = 0.5f;
                        re *= factor;
                        im *= factor;
                    }
                }
                break;
            default:
                // Fallback to original signal A
                re = rA;
                im = iA;
                break;
            }

            // Create output structure
            DataAtom* outer = context->eventPool.allocateDataAtom();
            outer->type = DataAtom::DataType::List;

            DataAtom* reOut = context->eventPool.allocateDataAtom();
            reOut->type = DataAtom::DataType::Float;
            reOut->data.atom = re;

            DataAtom* imOut = context->eventPool.allocateDataAtom();
            imOut->type = DataAtom::DataType::Float;
            imOut->data.atom = im;

            reOut->next = imOut;
            outer->data.list = reOut;
            outer->next = nullptr;

            if (!head) head = tail = outer;
            else { tail->next = outer; tail = outer; }
        }

        outEvent->data = head;
        outEvent->numAtoms = FREQ_BINS;
        outEvent->setTagHashcode(hash("complexbins"));
        outEvent->setTimeStamp(0);
        outputPortBuffers[0]->addEvent(outEvent);
    }

private:
    std::array<float, FREQ_BINS> binReal{};
    std::array<float, FREQ_BINS> binImag{};
    std::array<float, FREQ_BINS> binRealFromB{};
    std::array<float, FREQ_BINS> binImagFromB{};

    StringParameter* modeParam = nullptr;
    std::atomic<uint32_t> modeHash;
};
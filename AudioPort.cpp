//
// Created by alexw on 28/12/2024.
//

#include "AudioPort.h"

AudioPort::AudioPort(AudioNode* parent, size_t size)
    : node(parent)
{
    if (size > 0) {
        audioBuffer.resize(size, 0.0f);
    }
}

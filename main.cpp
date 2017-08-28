#include <assert.h>
#include <chrono>
#include <memory>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include "ableton/Link.hpp"
#include "wiringPi.h"

namespace {

const double defaultTempo = 120.0;
const double defaultQuantum = 4.0;
constexpr std::chrono::microseconds updateInterval(2000);
constexpr std::chrono::microseconds pulseLength(100);
const uint32_t pulseWiringPiGpioPin = 0;

bool isRunning = false;

constexpr std::chrono::microseconds timeDiff(const struct timespec& startTime, const struct timespec& endTime) {
    return std::chrono::microseconds((endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_nsec - startTime.tv_nsec) / 1000);
}

constexpr std::chrono::microseconds toMicroSeconds(const struct timespec& inTime) {
    return std::chrono::microseconds(inTime.tv_sec * 1000000 + inTime.tv_nsec / 1000);
}

void setupGpio() {
    wiringPiSetup();
    pinMode(0, OUTPUT);
    digitalWrite(pulseWiringPiGpioPin, LOW);
}

void startTransport(ableton::Link& link) {
    auto timeline = link.captureAudioTimeline();
    
    struct timespec hostTimeSpec;
    clock_gettime(CLOCK_MONOTONIC, &hostTimeSpec);
    const auto hostTime = toMicroSeconds(hostTimeSpec);

    timeline.requestBeatAtTime(0, hostTime, defaultQuantum);
    link.commitAudioTimeline(timeline);
}

void onPhaseChange(ableton::Link& link, const double phase) {
    digitalWrite(pulseWiringPiGpioPin0, HIGH);
    usleep(static_cast<useconds_t>(pulseLength.count()));
    digitalWrite(pulseWiringPiGpioPin, LOW);
}

} // namespace

void threadFunction(ableton::Link* link, const std::chrono::microseconds interval) {
    assert(link);
    if (!link) {
        return;
    }

    startTransport(*link);

    struct timespec startTime;
    struct timespec endTime;
    double lastPhase = -1.0;

    while (isRunning) {
        clock_gettime(CLOCK_MONOTONIC, &startTime);
        
        {
            auto timeline = link->captureAudioTimeline();
            const auto hostTime = toMicroSeconds(startTime);
            
            if (timeline.beatAtTime(hostTime, defaultQuantum) >= 0.0) {
                const auto phase = timeline.phaseAtTime(hostTime, defaultQuantum);
                
                if (static_cast<int32_t>(phase) != static_cast<int32_t>(lastPhase)) {
                    onPhaseChange(*link, phase);
                    lastPhase = phase;
                }
            }

            link->commitAudioTimeline(timeline);    
        }

        clock_gettime(CLOCK_MONOTONIC, &endTime);
        const auto diff = timeDiff(startTime, endTime);
        if (diff < interval) {
            usleep(static_cast<useconds_t>(interval.count() - diff.count()));
        }
    }
}

int main(int arch, char** argv) {
    setupGpio();

    struct sched_param schedParam;

    schedParam.sched_priority = 10;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &schedParam);

    ableton::Link link(defaultTempo);
    link.enable(true);

    isRunning = true;
    std::unique_ptr<std::thread> thread(new std::thread(threadFunction, &link, updateInterval));

    schedParam.sched_priority = 99;
    if (pthread_setschedparam(thread->native_handle(), SCHED_FIFO, &schedParam) != 0) {
        fprintf(stderr, "Unable to set thread priority\n");
    }
    
    sleep(10);
    isRunning = false;
    thread->join();
}

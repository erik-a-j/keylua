#ifndef EVENT_PIPELINE_H
#define EVENT_PIPELINE_H

#include <string>
#include <string_view>
#include <atomic>

struct input_event;
class DeviceGrabber;
class VirtualDevice;

class EventPipeline {
public:
    explicit EventPipeline(DeviceGrabber& dev, VirtualDevice& vdev);

    EventPipeline(EventPipeline&& other) = delete;
    EventPipeline& operator=(EventPipeline&&) = delete;
    EventPipeline(const EventPipeline&) = delete;
    EventPipeline& operator=(const EventPipeline&) = delete;

    bool run(std::atomic<bool>& stop);
    //void request_stop();

    explicit operator bool() const { return m_errbuf.empty(); }
    std::string_view errmsg() const { return m_errbuf; }
private:
    bool process_event(const ::input_event& ev);

    DeviceGrabber& m_dev;
    VirtualDevice& m_vdev;
    std::string m_errbuf;
};

#endif /* #ifndef EVENT_PIPELINE_H */

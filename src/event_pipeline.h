#ifndef EVENT_PIPELINE_H
#define EVENT_PIPELINE_H

class DeviceGrabber;
class VirtualDevice;

class EventPipeline {
public:
    explicit EventPipeline(DeviceGrabber& dev, VirtualDevice& vdev);

    EventPipeline(EventPipeline&& other) = delete;
    EventPipeline& operator=(EventPipeline&&) = delete;
    EventPipeline(const EventPipeline&) = delete;
    EventPipeline& operator=(const EventPipeline&) = delete;

    ~EventPipeline();

    void run();

private:
    DeviceGrabber& m_dev;
    VirtualDevice& m_vdev;
};

#endif /* #ifndef EVENT_PIPELINE_H */

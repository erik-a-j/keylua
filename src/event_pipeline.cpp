
#include "event_pipeline.h"
#include "device_grabber.h"
#include "virtual_device.h"
#include <sys/epoll.h>

EventPipeline::EventPipeline(DeviceGrabber& dev, VirtualDevice& vdev)
    : m_dev{dev}, m_vdev{vdev}
{}

EventPipeline::~EventPipeline()
{
    m_vdev.close();
    m_dev.close();
}

void EventPipeline::run()
{

}



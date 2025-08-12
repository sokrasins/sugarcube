https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/maximizing-battery-lifetime-in-cellular-iot-an-analysis-of-edrx-psm-and-as-rai

https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/ltem-vs-nbiot-field-test-how-distance-affects-power-consumption

power-saving modes

- DRX
  - Discontinuous reception: coordinated periods of reception
  - cDRX
  - iDRX
  - eDRX
- PSM
  - Power Saving Mode: deep sleep modes where radio is not receiving
- AS-RAI
  - Release Assistant Indication for Access Stratum
  - User Equipment (UE) can tell network that it has no more uplink data, and requests to be releases (RRC release)
  - Faster transition to RRC idle
  - Skips period of monitoring paging messages

- PSM and eDRX can coexist, but it's not always optimal
  - If UE does not expect to receive DL (downlink) data and only needs to send UL data rarely (every other day), switching modem off between UL messages is most beneficial
  - UE not expecting DL data, but sends UL or TAUs in intervals every hour or once a day, then PSM is most beneficial
  - UE expects to receive DL data, then UE needs to monitor RX paging, and eDRX is more beneficial. Set eDRX to max latency the application can accommodate


Questions:
- What's the paging window for?
- Sim card power consumption: support for suspend/resume feature? Good for power consumption.
- Device design - What's the slowest polling speed appropriate for low-detection?
  - Or does the device expect a downlink at the correct threshold?
- RAI support depends on the network. How prevalent is this?
- Network-chosen parameters have a huge effect on UE battery life. Need to test configuration in service area.






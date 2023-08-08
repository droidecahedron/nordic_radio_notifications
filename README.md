# Hardware
`nRF52840DK`

# Background
[Very good blog on the MSPL timeslot interface by Daniel V](https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/software/posts/updating-to-the-mpsl-timeslot-interface) that led to a lot of this knowledge.
Let's say we want to "estimate" how long connection events and intervals are between two devices.

Sometimes simply toggling a pin when a notification begins and ends doesn't give us the clear picture on what's actually happening on the radio level. 
The reason for this is because for instance if you toggle the pin on before sending a notification, then toggle it off after the notification completes via callback, the notification complete callback only executes when the device actually hears back. So this doesn't really show you much as far as what's happening on the radio.

Thus, we can use [Radio Notifications](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/doc/radio_notification.html) to toggle a pin in a lightweight IRQ on either edge. This lets us paint a clearer picture of when the notification actually started and stop described above.

In this case, the QDEC IRQn is hijacked as it's not being used. Some peripherals share IRQs! ([Link](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fmemory.html&cp=5_0_0_3_1_3&anchor=topic))

# Screenshot
(Example of code addition running on a 2 peripheral notifying the central configuration)

![image](https://github.com/droidecahedron/nordic_radio_notifications/assets/63935881/2d3e0c12-3825-4ee4-b533-c66db071ca58)


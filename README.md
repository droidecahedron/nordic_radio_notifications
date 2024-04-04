# Hardware
`nRF52840`

# Software
`SDK <v2.5.0`

## **[RADIO NOTIFICATION API REMOVED IN MPSL IN LATEST](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/CHANGELOG.html#changes)**
> If you are on 2.6 and onward, you can use this: https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/bluetooth/connection_event_trigger/README.html

# Background

[Very good blog on the MSPL timeslot interface by Daniel V](https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/software/posts/updating-to-the-mpsl-timeslot-interface) that led to a lot of this knowledge.
Let's say we want to "estimate" how long connection events and intervals are between two devices.

Sometimes simply toggling a pin when a notification begins and ends doesn't give us the clear picture on what's actually happening on the radio level. 
The reason for this is because for instance if you toggle the pin on before sending a notification, then toggle it off after the notification completes via callback, the notification complete callback only executes when the device actually hears back. So this doesn't really show you much as far as what's happening on the radio.

Thus, we can use [Radio Notifications](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.5.0/nrfxlib/mpsl/doc/radio_notification.html) to toggle a pin in a lightweight IRQ on either edge. This lets us paint a clearer picture of when the notification actually started and stop described above.

In this case, the QDEC IRQn is hijacked as it's not being used. Some peripherals share IRQs! ([Link](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fmemory.html&cp=5_0_0_3_1_3&anchor=topic))

As you test and observe the radio activity, you can take some actions to optimize your connection for throughput/rate/etc.

Or maybe you want an interrupt to trigger when the radio is going to turn on. or off. From [MPSL doc page](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/README.html) : 
> Radio notifications. It provides a configurable interrupt, usable before and/or after radio activity.

# Screenshot
(Example of code addition running on 3 peripherals notifying the central configuration)

![image](https://github.com/droidecahedron/nordic_radio_notifications/assets/63935881/65c34e3a-b37c-4188-aad7-d43a706cefd4)

*Missed connection events due to sub optimal network parameters chosen.*

![image](https://github.com/droidecahedron/nordic_radio_notifications/assets/63935881/bb0ebb95-020b-469a-abde-9caf34391080)
![image](https://github.com/droidecahedron/nordic_radio_notifications/assets/63935881/4114a020-5d48-40a0-88c6-62354897e4fb)


*No missed CE with optimal network parameters, good usage of the radio*



# Notes

- you could use `k_sched_lock()` and `k_sched_unlock()` within main before bluetooth init and it would work, but it is better to use system work queue. The calls have to be from cooperative thread.

## Optimizing connection
### [Good blog on this](https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/software/posts/building-a-bluetooth-application-on-nrf-connect-sdk-part-3-optimizing-the-connection)

_For instance, modifying event length fxn and connection interval from [sample](https://github.com/nrfconnect/sdk-nrf/blob/main/samples/bluetooth/llpm)_
```c
//peripheral
static int set_custom_event_length(uint32_t event_len_us)
{
	int err;
	struct net_buf *buf;

	sdc_hci_cmd_vs_event_length_set_t *cmd_event_len_set;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_EVENT_LENGTH_SET, sizeof(*cmd_event_len_set));
	if (!buf)
	{
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	cmd_event_len_set = net_buf_add(buf, sizeof(*cmd_event_len_set));
	cmd_event_len_set->event_length_us = event_len_us;

	err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_EVENT_LENGTH_SET, buf, NULL);
	if (err)
	{
		printk("Set event length failed (err %d)\n", err);
		return err;
	}

	printk("event length configured\n");
	return 0;
}

//central
static int change_connection_interval(uint16_t interval_us)
{
	int err;
	struct net_buf *buf;

	sdc_hci_cmd_vs_conn_update_t *cmd_conn_update;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE,
				sizeof(*cmd_conn_update));
	if (!buf) {
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	uint16_t conn_handle;

	err = bt_hci_get_conn_handle(default_conn, &conn_handle);
	if (err) {
		printk("Failed obtaining conn_handle (err %d)\n", err);
		return err;
	}

	cmd_conn_update = net_buf_add(buf, sizeof(*cmd_conn_update));
	cmd_conn_update->conn_handle         = conn_handle;
	cmd_conn_update->conn_interval_us    = interval_us;
	cmd_conn_update->conn_latency        = 0;
	cmd_conn_update->supervision_timeout = 300;

	err = bt_hci_cmd_send_sync(SDC_HCI_OPCODE_CMD_VS_CONN_UPDATE, buf, NULL);
	if (err) {
		printk("Update connection parameters failed (err %d)\n", err);
		return err;
	}

	return 0;
}
```

## IRQ Connect
This repo utilizes the following:
https://github.com/droidecahedron/nordic_radio_notifications/blob/c5c0810ce86d65d57fd5918db04d743780155ad9/main.c#L33-L34

If at some point you want to use `IRQ_DIRECT_CONNECT`, be advised of the following:
> `IRQ_DIRECT_CONNECT` will not allow the scheduler to run, it effectively bypasses the kernels ability run its scheduling point that might otherwise happen after an ISR returns.

## From the [Radio Notification doc page](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/doc/radio_notification.html)
[TechDocs link](https://docs.nordicsemi.com/bundle/ncs-2.5.0/page/nrfxlib/mpsl/doc/radio_notification.html)
> Copypasta in case the link dies again...

The radio notification is a configurable feature that enables ACTIVE and INACTIVE (nACTIVE) signals from the MPSL to the application notifying it when timeslot events are active. The application can configure how much in advance the ACTIVE signal occurs.

The radio notification signals are sent right before or at the end of an MPSL timeslot activity. The timeslot activity may be requested by the application or another user of MPSL. To ensure that the radio notification signals behave in a consistent way, the radio notification must always be configured when the MPSL is in an idle state with no active users. Therefore, it is recommended to configure the radio notification signals directly after the MPSL has been enabled.

If it is enabled, the ACTIVE signal is sent before the timeslot events starts. Similarly, if the nACTIVE signal is enabled, it is sent at the end of the timeslot event. These signals can be used by the application developer to synchronize the application logic with the timeslot activity. For example, if the application is using the timeslot for radio activity, the ACTIVE signal can be used to switch off external devices to manage peak current drawn during periods when the radio is ON, or to trigger sensor data collection for transmission during the upcoming event.

As both ACTIVE and nACTIVE use the same software interrupt, it is up to the application to manage them. If both ACTIVE and nACTIVE are configured ON by the application, there will always be an ACTIVE signal before an nACTIVE signal.

When there is sufficient time between timeslot events, both the ACTIVE and nACTIVE notification signals will be present at each event. When there is not sufficient time between the events, the ACTIVE and nACTIVE notification signals will be skipped. There will still be an ACTIVE signal before the first event and an nACTIVE signal after the last event.

![image](https://github.com/droidecahedron/nordic_radio_notifications/assets/63935881/a91b0021-6e01-4eb5-bc01-5d57db554fa3)



t_distance is important to note, as well as whether we want notifications for active and nactive. You can repurpose the mpsl api call to suit your requirements.
https://github.com/droidecahedron/nordic_radio_notifications/blob/f9417fa625dc21417dca27c9c04f39e969c3cf0e/main.c#L23-L25

The list of what is available is in `<ncs ver>/nrfxlib/mpsl/include/mpsl_radio_notification.h`
```c
/** @brief Guaranteed time for application to process radio inactive notification. */
#define MPSL_RADIO_NOTIFICATION_INACTIVE_GUARANTEED_TIME_US  (62)

/** @brief Radio notification distances. */
enum MPSL_RADIO_NOTIFICATION_DISTANCES
{
  MPSL_RADIO_NOTIFICATION_DISTANCE_NONE = 0, /**< The event does not have a notification. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_200US,    /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_420US,    /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_800US,    /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_1740US,   /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_2680US,   /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_3620US,   /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_4560US,   /**< The distance from the active notification to start of radio activity. */
  MPSL_RADIO_NOTIFICATION_DISTANCE_5500US    /**< The distance from the active notification to start of radio activity. */
};

/** @brief Radio notification types. */
enum MPSL_RADIO_NOTIFICATION_TYPES
{
    MPSL_RADIO_NOTIFICATION_TYPE_NONE = 0,        /**< The event does not have a radio notification signal. */
    MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_ACTIVE,   /**< Using interrupt for notification when the radio will be enabled. */
    MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_INACTIVE, /**< Using interrupt for notification when the radio has been disabled. */
    MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH,     /**< Using interrupt for notification both when the radio will be enabled and disabled. */
};
```

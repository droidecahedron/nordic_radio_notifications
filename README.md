# Hardware
`nRF52840`

# Background
[Very good blog on the MSPL timeslot interface by Daniel V](https://devzone.nordicsemi.com/guides/nrf-connect-sdk-guides/b/software/posts/updating-to-the-mpsl-timeslot-interface) that led to a lot of this knowledge.
Let's say we want to "estimate" how long connection events and intervals are between two devices.

Sometimes simply toggling a pin when a notification begins and ends doesn't give us the clear picture on what's actually happening on the radio level. 
The reason for this is because for instance if you toggle the pin on before sending a notification, then toggle it off after the notification completes via callback, the notification complete callback only executes when the device actually hears back. So this doesn't really show you much as far as what's happening on the radio.

Thus, we can use [Radio Notifications](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrfxlib/mpsl/doc/radio_notification.html) to toggle a pin in a lightweight IRQ on either edge. This lets us paint a clearer picture of when the notification actually started and stop described above.

In this case, the QDEC IRQn is hijacked as it's not being used. Some peripherals share IRQs! ([Link](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fmemory.html&cp=5_0_0_3_1_3&anchor=topic))

As you test and observe the radio activity, you can take some actions to optimize your connection for throughput/rate/etc.

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
static int set_short_event_length(uint32_t event_len_us)
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
static int vs_change_connection_interval(uint16_t interval_us)
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

If at some point you need to use `IRQ_DIRECT_CONNECT`, be advised of the following:
> `IRQ_DIRECT_CONNECT` will not allow the scheduler to run, it effectively bypasses the kernels ability run its scheduling point that might otherwise happen after an ISR returns.

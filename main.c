/*Note: This isn't a full main.c, just the additions you need to add it to your code.*/
#include <mpsl_radio_notification.h>

//! hijacking qdec irqn. note some peripherals share irqs. (https://infocenter.nordicsemi.com/index.jsp?topic=%2Fps_nrf52840%2Fmemory.html&cp=5_0_0_3_1_3&anchor=topic)
#define RADIO_NOTI_IRQN QDEC_IRQn
#define RADIO_NOTI_IRQN_NODELABEL qdec
#define RADIO_NOTI_IRQ_PRIO 4 // in the middle.

//! pin to toggle
#define TEST_PIN 16 // p0 pin 16. note p1 pin0 is 32.

//! WorkQ item.
//You need to call the mspl api from a cooperative thread.
//the work thread is a cooperative thread, so calling the fxn as a work item works.
static struct k_work radio_noti_work;
static void radio_noti_work_fn(struct k_work *work)
{
	mpsl_radio_notification_cfg_set(MPSL_RADIO_NOTIFICATION_TYPE_INT_ON_BOTH, // on both events                                                  
	MPSL_RADIO_NOTIFICATION_DISTANCE_200US, // lowest possible
	RADIO_NOTI_IRQN);
}

//! Radio notification callback
static void radio_notify_cb(const void *context)
{
	/*note: Not using devicetree to make sure this irq is as fast as possible!*/
	nrf_gpio_pin_toggle(TEST_PIN);
}


void main(void)
{
  IRQ_CONNECT(DT_IRQN(DT_NODELABEL(RADIO_NOTI_IRQN_NODELABEL)), RADIO_NOTI_IRQ_PRIO,
                radio_notify_cb, NULL, 0);
    irq_enable(DT_IRQN(DT_NODELABEL(RADIO_NOTI_IRQN_NODELABEL)));

	k_work_init(&radio_noti_work, radio_noti_work_fn);
	k_work_submit(&radio_noti_work);
		
	//init the pin
	nrf_gpio_cfg_output(TEST_PIN);
	nrf_gpio_pin_clear(TEST_PIN); // start clear

  /*
   The rest of your code, particularly bluetooth inits. You want to do above before bluetooth inits.
    */
}

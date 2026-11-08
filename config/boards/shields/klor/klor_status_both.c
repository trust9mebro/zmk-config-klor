/*
 * KLOR status screen for the left (central) OLED.
 *
 * Same as ZMK's built-in screen (output status top-left, layer bottom-left),
 * plus one line showing the battery of BOTH halves:
 *
 *     L⚡82%  R 41%
 *
 * The right half's level comes from the peripheral over the split BLE link
 * (CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING). ZMK reports 0 when the
 * peripheral disconnects, which is shown as "R --".
 *
 * Written against ZMK v0.3.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <lvgl.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/central.h>

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/usb.h>
#include <zmk/events/usb_conn_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
#include <zmk/display/widgets/output_status.h>
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
#include <zmk/display/widgets/layer_status.h>
static struct zmk_widget_layer_status layer_status_widget;
#endif

/* ---- both-halves battery line ------------------------------------------ */

struct klor_batt_state {
    uint8_t left;
    uint8_t right; /* 0 = peripheral disconnected / not reported yet */
    bool usb;
};

static lv_obj_t *batt_label;

static void klor_batt_update_cb(struct klor_batt_state s) {
    if (batt_label == NULL) {
        return;
    }

    char right[6];
    if (s.right == 0) {
        strcpy(right, "--");
    } else {
        snprintf(right, sizeof(right), "%u%%", s.right);
    }

    char text[32];
    snprintf(text, sizeof(text), "L%s%u%%  R %s", s.usb ? LV_SYMBOL_CHARGE : " ", s.left, right);
    lv_label_set_text(batt_label, text);
}

static struct klor_batt_state klor_batt_get_state(const zmk_event_t *eh) {
    struct klor_batt_state s = {0};

    const struct zmk_battery_state_changed *ev =
        (eh != NULL) ? as_zmk_battery_state_changed(eh) : NULL;
    s.left = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge();

#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
    uint8_t level = 0;
    if (zmk_split_central_get_peripheral_battery_level(0, &level) == 0) {
        s.right = level;
    }
#endif

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    s.usb = zmk_usb_is_powered();
#endif

    return s;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_klor_batt, struct klor_batt_state, klor_batt_update_cb,
                            klor_batt_get_state)

ZMK_SUBSCRIPTION(widget_klor_batt, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING)
ZMK_SUBSCRIPTION(widget_klor_batt, zmk_peripheral_battery_state_changed);
#endif
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_klor_batt, zmk_usb_conn_state_changed);
#endif

/* ---- screen ------------------------------------------------------------- */

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
    zmk_widget_output_status_init(&output_status_widget, screen);
    lv_obj_align(zmk_widget_output_status_obj(&output_status_widget), LV_ALIGN_TOP_LEFT, 0, 0);
#endif

    batt_label = lv_label_create(screen);
    lv_obj_align(batt_label, LV_ALIGN_LEFT_MID, 0, 0);
    widget_klor_batt_init();

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
    zmk_widget_layer_status_init(&layer_status_widget, screen);
    lv_obj_set_style_text_font(zmk_widget_layer_status_obj(&layer_status_widget),
                               lv_theme_get_font_small(screen), LV_PART_MAIN);
    lv_obj_align(zmk_widget_layer_status_obj(&layer_status_widget), LV_ALIGN_BOTTOM_LEFT, 0, 0);
#endif

    return screen;
}

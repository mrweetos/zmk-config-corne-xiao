/*
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/display.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>

#include "battery_status.h"

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
    #define SOURCE_OFFSET 1
#else
    #define SOURCE_OFFSET 0
#endif

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct battery_state {
    uint8_t source;
    uint8_t level;
    bool usb_present;
};

// Canvas dependencies removed; using native LV_OBJ wrappers mapped to ZMK_DISPLAY UI loop.

static void set_battery_symbol(lv_obj_t *widget, struct battery_state state) {
    lv_obj_t *battery_container = lv_obj_get_child(widget, state.source * 2);
    lv_obj_t *label = lv_obj_get_child(widget, state.source * 2 + 1);

    lv_obj_t *fill = lv_obj_get_child(battery_container, 0);

    if (state.level <= 10 || state.usb_present) {
        lv_obj_set_height(fill, 5);
        if (state.usb_present) {
            lv_obj_set_style_bg_opa(fill, LV_OPA_TRANSP, 0);
        } else {
            lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
        }
    } else if (state.level <= 30) {
        lv_obj_set_height(fill, 4);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    } else if (state.level <= 50) {
        lv_obj_set_height(fill, 3);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    } else if (state.level <= 70) {
        lv_obj_set_height(fill, 2);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    } else if (state.level <= 90) {
        lv_obj_set_height(fill, 1);
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_height(fill, 5); // Fully charged
        lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    }

    lv_label_set_text_fmt(label, "%4u%%", state.level);
    
    if (state.level > 0 || state.usb_present) {
        lv_obj_clear_flag(battery_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(battery_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
}

void battery_status_update_cb(struct battery_state state) {
    struct zmk_widget_dongle_battery_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_symbol(widget->obj, state); }
}

static struct battery_state peripheral_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_peripheral_battery_state_changed *ev = as_zmk_peripheral_battery_state_changed(eh);
    return (struct battery_state){
        .source = ev->source + SOURCE_OFFSET,
        .level = ev->state_of_charge,
    };
}

static struct battery_state central_battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);
    return (struct battery_state) {
        .source = 0,
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

static struct battery_state battery_status_get_state(const zmk_event_t *eh) { 
    if (as_zmk_peripheral_battery_state_changed(eh) != NULL) {
        return peripheral_battery_status_get_state(eh);
    } else {
        return central_battery_status_get_state(eh);
    }
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_dongle_battery_status, struct battery_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_peripheral_battery_state_changed);

#if IS_ENABLED(CONFIG_ZMK_DONGLE_DISPLAY_DONGLE_BATTERY)
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_dongle_battery_status, zmk_usb_conn_state_changed);
#endif 
#endif 
#endif 

int zmk_widget_dongle_battery_status_init(struct zmk_widget_dongle_battery_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_remove_style_all(widget->obj);

    lv_obj_set_size(widget->obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT + SOURCE_OFFSET; i++) {
        // Create an empty border for the battery casing bypassing canvas crashes
        lv_obj_t *battery_container = lv_obj_create(widget->obj);
        lv_obj_remove_style_all(battery_container);
        lv_obj_set_size(battery_container, 5, 8);
        lv_obj_set_style_border_color(battery_container, lv_color_white(), 0);
        lv_obj_set_style_border_width(battery_container, 1, 0);
        lv_obj_set_style_bg_opa(battery_container, LV_OPA_TRANSP, 0);

        // Nested filling level
        lv_obj_t *battery_fill = lv_obj_create(battery_container);
        lv_obj_remove_style_all(battery_fill);
        lv_obj_set_width(battery_fill, 3);
        lv_obj_set_style_bg_color(battery_fill, lv_color_white(), 0);
        lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, 0);
        lv_obj_align(battery_fill, LV_ALIGN_BOTTOM_MID, 0, -1);

        lv_obj_t *battery_label = lv_label_create(widget->obj);

        lv_obj_align(battery_container, LV_ALIGN_TOP_RIGHT, 0, i * 10);
        lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -7, i * 10);
        
        lv_obj_add_flag(battery_container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(battery_label, LV_OBJ_FLAG_HIDDEN);
    }

    sys_slist_append(&widgets, &widget->node);
    widget_dongle_battery_status_init();

    return 0;
}

lv_obj_t *zmk_widget_dongle_battery_status_obj(struct zmk_widget_dongle_battery_status *widget) {
    return widget->obj;
}

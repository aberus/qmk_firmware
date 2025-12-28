#include "lamparray.h"
#include "hid_lamparray.h"
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

uint16_t _lastLampID = 0;
bool hid_lamparray_auto_mode = true;

static const LAMP_INFO_IN s_lampAttributes[] =
{
        //X      Y      Z          Key      LampPurposes
        { 7460,  23000, 4700+3200, KC_NO,   2 }, // LampPurposeAccent
        { 14500, 20000, 4700,      KC_LCTL, 1 },
        { 32500, 20000, 4700,      KC_C,    1 },
        { 52500, 20000, 4700,      KC_V,    1 },
        { 60000, 23000, 4700+3200, KC_NO,   2 }, // LampPurposeAccent
};

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{

    switch(*ReportID) {
        case hid_lamparray_attributes_report_id: {
            hid_lamparray_attributes_report_t report = {};
            //report.report_id = *ReportID;
            report.lamp_count = RGBLED_NUM;
            report.bounding_box.x = 67000; // 67mm
            report.bounding_box.y = 34000;  // 34mm
            report.bounding_box.z = 13000;   // 13mm
            report.array_kind = 1; // LampArrayKind Keyboard
            report.min_update_interval = 4000;  // 4ms
            *ReportSize = sizeof(report);

            _lastLampID = 0;

            memcpy(ReportData, &report, *ReportSize);

            break;
        }

        case hid_lamparray_attributes_response_report_id: {
            const LAMP_INFO_IN *p = &s_lampAttributes[_lastLampID];

            hid_lamparray_attributes_response_report_t report = {};
            //report.report_id = *ReportID;
            report.lamp_id = _lastLampID;
            report.position.x = p->position_x;
            report.position.y = p->position_y;
            report.position.z = p->position_z;
            report.update_latency = 4000;
            report.lamp_purposes = p->lamp_purposes;
            report.level_counts.red = UINT8_MAX;
            report.level_counts.green = UINT8_MAX;
            report.level_counts.blue = UINT8_MAX;
            report.level_counts.intensity = 1;
            report.is_programmable = 1;
            report.input_binding = p->lamp_key;

            _lastLampID++;
            if (_lastLampID  >= RGBLED_NUM) {
                _lastLampID = 0;
            }

            *ReportSize = sizeof(report);
            memcpy(ReportData, &report, *ReportSize);

            break;
        }
    }

	return false;
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize) {

    switch (ReportID) {
        case hid_lamparray_attributes_request_report_id: {
            hid_lamparray_attributes_request_report_t *report = (hid_lamparray_attributes_request_report_t *)ReportData;
            //report.report_id = *ReportID;
            _lastLampID = report->lamp_id;
            if (_lastLampID  >= RGBLED_NUM) {
                _lastLampID = 0;
            }
            break;
        }
        case hid_lamparray_multi_update_report_id: {
            if (hid_lamparray_auto_mode) {
                return;
            }

            hid_lamparray_multi_update_report_t *report = (hid_lamparray_multi_update_report_t *)ReportData;
            for (int i = 0; i < report->lamp_count; i++) {
                if (report->lamp_ids[i] < RGBLED_NUM && report->lamp_ids[i] >= 0) {
                    rgblight_setrgb_at(report->colors[i].red, report->colors[i].green, report->colors[i].blue, report->lamp_ids[i]);
                }
            }

            break;
        }
        case hid_lamparray_range_update_report_id: {
            if (hid_lamparray_auto_mode) {
                return;
            }

            hid_lamparray_range_update_report_t *report = (hid_lamparray_range_update_report_t *)ReportData;
            for (int i = report->lamp_id_start; i <= report->lamp_id_end && i < RGBLED_NUM; i++) {
                rgblight_setrgb_at(report->color.red, report->color.green, report->color.blue, i);
            }

            break;
        }
        case hid_lamparray_control_report_id: {
            hid_lamparray_control_report_t *report = (hid_lamparray_control_report_t *)ReportData;
            if (hid_lamparray_auto_mode && report->autonomous_mode == 0) {
                // rgblight_disable_noeeprom()
                hid_lamparray_auto_mode = false;
            }
            else if (!hid_lamparray_auto_mode && report->autonomous_mode == 1) {
                rgblight_enable_noeeprom();
                // rgblight_setrgb(RGB_RED);
                hid_lamparray_auto_mode = true;
            }
            break;
        }
    }
}

#include "cfgmod.h"

#include <stdio.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"

#define TAG "cfg_module"

#define CFGMOD_NVS_NAMESPACE "cfg"

static void log_hex(const char *label, const uint8_t *buf, size_t len)
{
    if (!buf || len == 0) {
        ESP_LOGI(TAG, "%s: (empty)", label);
        return;
    }

    char line[128];
    size_t pos = 0;

    ESP_LOGI(TAG, "%s: len=%u", label, (unsigned)len);
    for (size_t i = 0; i < len; i++) {
        int written = snprintf(&line[pos], sizeof(line) - pos, "%02X ", buf[i]);
        if (written <= 0) break;
        pos += (size_t)written;

        if ((i % 16) == 15 || i == len - 1) {
            ESP_LOGI(TAG, "%s", line);
            pos = 0;
        }
    }
}

/*
	comms
*/

// Payload format:
//  byte0 = opcode (0=GET, 1=SET)
//  byte1 = key_len (1..CFGMOD_MAX_KEY_LEN)
//  bytes[2..] = key (not null-terminated)
//  remaining bytes = data (SET only)
esp_err_t cfgmod_handle_usb_comm(const uint8_t *data,
                                 size_t len,
                                 uint8_t *out,
                                 size_t *out_len,
                                 size_t out_max)
{
    if (!data || !out || !out_len || out_max == 0) {
        ESP_LOGE(TAG, "Wrong data");
        return ESP_ERR_INVALID_ARG;
    }
    *out_len = 0;

    if (len < sizeof(cfgmod_wire_header_t)) {
        ESP_LOGE(TAG, "Invalid size (size < sizeof(cfgmod_wire_header_t))");
        return ESP_ERR_INVALID_SIZE;
    }

    cfgmod_wire_header_t hdr;
    memcpy(&hdr, data, sizeof(hdr));

    if (hdr.payload_len > (len - sizeof(hdr))) {
        ESP_LOGE(TAG, "Invalid size (hdr.payload_len > (len - sizeof(hdr)))");
        return ESP_ERR_INVALID_SIZE;
    }

    const uint8_t *payload = data + sizeof(hdr);
    size_t payload_len = hdr.payload_len;

    if (payload_len < 2) {
        ESP_LOGE(TAG, "Invalid size (payload_len < 2)");
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t opcode = payload[0];
    uint8_t key_len = payload[1];

    if (key_len == 0 || key_len > CFGMOD_MAX_KEY_LEN) return ESP_ERR_INVALID_ARG;
    if (payload_len < (size_t)(2 + key_len)) return ESP_ERR_INVALID_SIZE;

    char key[CFGMOD_MAX_KEY_LEN + 1] = {0};
    memcpy(key, &payload[2], key_len);
    key[key_len] = '\0';

    const uint8_t *data_in = &payload[2 + key_len];
    size_t data_in_len = payload_len - (2 + key_len);

    // Build response header
    cfgmod_wire_header_t rsp = {
        .type = CFGMOD_MSG_RESPONSE,
        .kind = hdr.kind,
        .seq = hdr.seq,
        .payload_len = 0
    };

    // Response payload: [esp_err_t status (4 bytes)] + data (GET only)
    const size_t status_size = sizeof(esp_err_t);
    if (out_max < sizeof(rsp) + status_size) return ESP_ERR_NO_MEM;

    esp_err_t status = ESP_OK;
    uint8_t *out_payload = out + sizeof(rsp);
    size_t out_payload_max = out_max - sizeof(rsp);
    memset(out_payload, 0, out_payload_max);

	// log
	ESP_LOGI(TAG, "RX COMM");
    log_hex("RX", data, len);

	// message is get. answer reading from storage
    if (opcode == 0) {
		ESP_LOGI(TAG, "Received GET message");

        size_t read_len = out_payload_max - status_size;
        status = cfgmod_read_storage((cfgmod_kind_t)hdr.kind, key, out_payload + status_size, &read_len);
        memcpy(out_payload, &status, status_size);

        if (status == ESP_OK) {
            rsp.payload_len = (uint16_t)(status_size + read_len);
        } else {
            rsp.payload_len = (uint16_t)status_size;
        }
	// message is set. answer after storing
    } else if (opcode == 1) {
		ESP_LOGI(TAG, "Received SET message");
        status = cfgmod_write_storage((cfgmod_kind_t)hdr.kind, key, data_in, data_in_len);
        memcpy(out_payload, &status, status_size);
        rsp.payload_len = (uint16_t)status_size;
    } else {
        status = ESP_ERR_INVALID_ARG;
        memcpy(out_payload, &status, status_size);
        rsp.payload_len = (uint16_t)status_size;
    }

    memcpy(out, &rsp, sizeof(rsp));
    *out_len = sizeof(rsp) + rsp.payload_len;

    ESP_LOGI(TAG, "Response length: %d, last byte: 0x%02X", *out_len, out[*out_len - 1]);

	// log
    ESP_LOGI(TAG, "TX COMM");
    log_hex("TX", out, *out_len);

    return ESP_OK;
}

/*
	storage
*/

// Build a short NVS key from kind and key name.
static esp_err_t cfgmod_build_key(cfgmod_kind_t kind, const char *key, char *out_key, size_t out_len)
{
	if (out_key == NULL || out_len == 0 || key == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (strlen(key) > CFGMOD_MAX_KEY_LEN) {
		return ESP_ERR_INVALID_ARG;
	}

	int written = snprintf(out_key, out_len, "k%d_%s", (int)kind, key);
	if (written < 0 || (size_t)written >= out_len) {
		return ESP_ERR_INVALID_ARG;
	}

	return ESP_OK;
}

static bool s_init = false;
bool is_init(void) { return s_init; }

// Initialize NVS for cfg storage use.
esp_err_t cfg_init(void)
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		esp_err_t erase_err = nvs_flash_erase();
		if (erase_err != ESP_OK) {
			return erase_err;
		}
		err = nvs_flash_init();
	}

    if (err == ESP_OK) {
        s_init = true;

        // Build "hello" field with ASCII "Hello world"
        static const char hello_msg[] = "Hello world";
        cfgmod_write_storage(CFGMOD_KIND_SYSTEM, "hello", hello_msg, sizeof(hello_msg) - 1);
    }

	return err;
}

// Deinitialize cfg module (placeholder for future cleanup).
esp_err_t cfg_deinit(void)
{
	return ESP_OK;
}

// Read a blob from NVS for the given kind/key.
esp_err_t cfgmod_read_storage(cfgmod_kind_t kind,
							  const char *key,
							  void *out_buf,
							  size_t *inout_len)
{
	if (out_buf == NULL || inout_len == NULL || *inout_len == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	char nvs_key[16] = {0};
	esp_err_t err = cfgmod_build_key(kind, key, nvs_key, sizeof(nvs_key));
	if (err != ESP_OK) {
		return err;
	}

	nvs_handle_t handle;
	err = nvs_open(CFGMOD_NVS_NAMESPACE, NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return err;
	}

	err = nvs_get_blob(handle, nvs_key, out_buf, inout_len);
	nvs_close(handle);
	return err;
}

// Write a blob to NVS for the given kind/key.
esp_err_t cfgmod_write_storage(cfgmod_kind_t kind,
							   const char *key,
							   const void *data,
							   size_t len)
{
	if (data == NULL || len == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	char nvs_key[16] = {0};
	esp_err_t err = cfgmod_build_key(kind, key, nvs_key, sizeof(nvs_key));
	if (err != ESP_OK) {
		return err;
	}

	nvs_handle_t handle;
	err = nvs_open(CFGMOD_NVS_NAMESPACE, NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		return err;
	}

	err = nvs_set_blob(handle, nvs_key, data, len);
	if (err == ESP_OK) {
		err = nvs_commit(handle);
	}
	nvs_close(handle);
	return err;
}

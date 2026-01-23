#include "cfg_layouts.h"

#include <stdbool.h>
#include <string.h>

#include "cfgmod.h"
#include "cfg_storage_keys.h"

static cfg_layout_t s_layout;
static bool s_layout_initialized = false;

// Ensure the layout cache is initialized to defaults.
esp_err_t cfg_layout_init(void)
{
	if (!s_layout_initialized) {
		memset(&s_layout, 0, sizeof(s_layout));
		s_layout.key_count = CFG_LAYOUT_MAX_KEYS;
		s_layout_initialized = true;
	}
	return ESP_OK;
}

// Return a copy of the current layout.
esp_err_t cfg_layout_get(cfg_layout_t *out_layout)
{
	if (out_layout == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!s_layout_initialized) {
		cfg_layout_init();
	}

	memcpy(out_layout, &s_layout, sizeof(cfg_layout_t));
	return ESP_OK;
}

// Replace the current layout with the provided one.
esp_err_t cfg_layout_set(const cfg_layout_t *layout)
{
	if (layout == NULL || layout->key_count > CFG_LAYOUT_MAX_KEYS) {
		return ESP_ERR_INVALID_ARG;
	}

	if (!s_layout_initialized) {
		cfg_layout_init();
	}

	memcpy(&s_layout, layout, sizeof(cfg_layout_t));
	return ESP_OK;
}

// Load the layout from NVS storage into memory.
esp_err_t cfg_layout_load_from_storage(void)
{
	if (!s_layout_initialized) {
		cfg_layout_init();
	}

	size_t len = sizeof(cfg_layout_t);
	esp_err_t err = cfgmod_read_storage(CFGMOD_KIND_LAYOUT, CFG_ST_LAYOUT1, &s_layout, &len);
	if (err != ESP_OK) {
		return err;
	}

	if (len != sizeof(cfg_layout_t)) {
		return ESP_ERR_INVALID_SIZE;
	}

	return ESP_OK;
}

// Persist the current in-memory layout to NVS.
esp_err_t cfg_layout_save_to_storage(void)
{
	if (!s_layout_initialized) {
		cfg_layout_init();
	}

	return cfgmod_write_storage(CFGMOD_KIND_LAYOUT, CFG_ST_LAYOUT1, &s_layout, sizeof(cfg_layout_t));
}

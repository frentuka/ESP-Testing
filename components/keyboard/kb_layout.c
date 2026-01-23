#include "kb_layout.h"
#include "class/hid/hid.h"

static inline bool kb_matrix_bit_get(const uint8_t *matrix, uint8_t row, uint8_t col)
{
    if (row >= KB_MATRIX_ROW_COUNT || col >= KB_MATRIX_COL_COUNT) {
        return false;
    }

    size_t bit_index = (row * KB_MATRIX_COL_COUNT) + col;
    return (matrix[bit_index >> 3] & (uint8_t)(1U << (bit_index & 7U))) != 0;
}

static inline bool kb_layout_is_fn_key(uint8_t row, uint8_t col)
{
    return (row == KB_FN1_ROW && col == KB_FN1_COL) ||
           (row == KB_FN2_ROW && col == KB_FN2_COL);
}

uint8_t kb_layout_get_active_layer(const uint8_t *matrix)
{
    if (kb_matrix_bit_get(matrix, KB_FN2_ROW, KB_FN2_COL)) {
        return KB_LAYER_FN2;
    }
    if (kb_matrix_bit_get(matrix, KB_FN1_ROW, KB_FN1_COL)) {
        return KB_LAYER_FN1;
    }
    return KB_LAYER_BASE;
}

uint8_t kb_layout_get_keycode(uint8_t row, uint8_t col, uint8_t layer)
{
    if (row >= KB_MATRIX_ROW_COUNT || col >= KB_MATRIX_COL_COUNT || kb_layout_is_fn_key(row, col)) {
        return HID_KEY_NONE;
    }

    if (layer >= KB_LAYER_COUNT) {
        layer = KB_LAYER_BASE;
    }

    uint8_t kc = keymaps[layer][row][col];
    if (kc == KB_KEY_TRANSPARENT) {
        kc = keymaps[KB_LAYER_BASE][row][col];
    }

    return kc;
}